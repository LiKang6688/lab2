#include "../include//WebServer.h"

//process request
void accept_request(int client)
{
 	char buf[BUFF_SIZE];
 	int numchars;
 	char method[255];
 	char url[255];
 	char path[512];
 	size_t i, j;
 	struct stat st;
 	int cgi = 0;

 	char *query_string = NULL;
 	memset(buf, 0, sizeof(buf));
 	numchars = get_line(client, buf, sizeof(buf));
 	 

 	//get method type
 	i = 0; j = 0;
 	while (!ISspace(buf[j]) && (i < sizeof(method) - 1))
 	{
 		method[i] = buf[j];
 		i++; j++;
 	}
 	method[i] = '\0';

 	if(strcasecmp(method, "GET") && strcasecmp(method, "HEAD"))
 	{
 		unimplemented(client);
  		return;
 	}

 	//get URL
 	i = 0;
 	while (ISspace(buf[j]) && (j < sizeof(buf)))
 		j++;
 	while (!ISspace(buf[j]) && (i < sizeof(url) - 1) && (j < sizeof(buf)))
 	{
 		url[i] = buf[j];
 		i++; 
 		j++;
 	}
 	url[i] = '\0';

 	//GET and HEAD method
 	//They are similar, GET method has extra operation of sending required data
 	if(strcasecmp(method, "GET") == 0 || strcasecmp(method, "HEAD") == 0)
 	{
 		query_string = url;
 		while ((*query_string != '?') && (*query_string != '\0'))
  		query_string++;

 		if(*query_string == '?')
 		{
  			cgi = 1;
  			*query_string = '\0';
   			query_string++;
    	}
 	}

 	sprintf(path, "../www%s", url);
 	if (path[strlen(path) - 1] == '/')
 	{
		strcat(path, "index.html");
		if(URL_validation(path) == -1)
    		{
      			ERR_EXIT("Too long URL");
      			close(client);
      			return;
    		}
	}
 	if (stat(path, &st) == -1) {
  		while ((numchars > 0) && strcmp("\n", buf))  /* read & discard headers */
   			numchars = get_line(client, buf, sizeof(buf));
   		//response code 404
  		not_found(client);
 	}else{
 		if ((st.st_mode & S_IFMT) == S_IFDIR)
   		{
			strcat(path, "/index.html");
			if(URL_validation(path) == -1)
      			{
        			ERR_EXIT("Too long URL");
        			close(client);
        			return;
      			}	
		}
	   		if(!(S_ISREG(st.st_mode)) || !(S_IRUSR & st.st_mode))
		{
			//response code 403
			forbidden(client);
			return;
		}
  		if ((st.st_mode & S_IXUSR) || (st.st_mode & S_IXGRP) || (st.st_mode & S_IXOTH))
   			cgi = 1;
  		if (!cgi)
  		{	
  			//execute GET method
  			if(strcasecmp(method, "GET") == 0) 
   				get_method(client, path);
   			//execute HEAD method
   			if(strcasecmp(method, "HEAD") == 0)
   				head_method(client, path);
  		}
  		else
   			execute_cgi(client, path, method, query_string);
 	}

 	close(client);
}

int URL_validation(char *path)
{
	int result = -1;
  	char buf[4096];
  	realpath(path, buf);
  	if(strlen(buf) < MAX_URL_LEN)
    		result = 1;
  		return result;
}

//handle bad request condition
void bad_request(int client)
{
 	char buf[BUFF_SIZE];

 	sprintf(buf, "HTTP/1.0 400 BAD REQUEST\r\n");
 	send(client, buf, sizeof(buf), 0);
 	sprintf(buf, "Content-type: text/html\r\n");
 	send(client, buf, sizeof(buf), 0);
 	sprintf(buf, "\r\n");
 	send(client, buf, sizeof(buf), 0);
 	sprintf(buf, "<P>Your browser sent a bad request, ");
 	send(client, buf, sizeof(buf), 0);
 	sprintf(buf, "such as a post without a Content-Length.\r\n");
 	send(client, buf, sizeof(buf), 0);
}

//send entire content our on a socket
void cat(int client, FILE *resource)
{
 	char buf[BUFF_SIZE];

 	fgets(buf, sizeof(buf), resource);
 	while (!feof(resource))
 	{
  		send(client, buf, strlen(buf), 0);
  		fgets(buf, sizeof(buf), resource);
 	}
}

//handle internal server error condition
void internal_server_error(int client)
{
 	char buf[BUFF_SIZE];

 	sprintf(buf, "HTTP/1.0 500 Internal Server Error\r\n");
 	send(client, buf, strlen(buf), 0);
 	sprintf(buf, "Content-type: text/html\r\n");
 	send(client, buf, strlen(buf), 0);
 	sprintf(buf, "\r\n");
 	send(client, buf, strlen(buf), 0);
 	sprintf(buf, "<P>Error prohibited CGI execution.\r\n");
 	send(client, buf, strlen(buf), 0);
}



//Execute a CGI script. 
void execute_cgi(int client, const char *path,
                 const char *method, const char *query_string)
{
 	char buf[BUFF_SIZE];
 	int cgi_output[2];
	int cgi_input[2];
 	pid_t pid;
 	int status;
 	int i;
 	char c;
 	int numchars = 1;
 	int content_length = -1;

 	buf[0] = 'A'; 
	buf[1] = '\0';
	if (strcasecmp(method, "GET") == 0)
  	while ((numchars > 0) && strcmp("\n", buf))  /* read & discard headers */
   		numchars = get_line(client, buf, sizeof(buf));
	else   
 	{
  		numchars = get_line(client, buf, sizeof(buf));
  		while ((numchars > 0) && strcmp("\n", buf))
  		{
   			buf[15] = '\0';
   			if (strcasecmp(buf, "Content-Length:") == 0)
    				content_length = atoi(&(buf[16]));
   				numchars = get_line(client, buf, sizeof(buf));
  		}
  		if (content_length == -1) 
		{
 			bad_request(client);
   			return;
  		}
 	}

 	sprintf(buf, "HTTP/1.0 200 OK\r\n");
 	send(client, buf, strlen(buf), 0);

 	if (pipe(cgi_output) < 0) 
	{
  		internal_server_error(client);
  		return;
 	}
 	if (pipe(cgi_input) < 0) 
	{
  		internal_server_error(client);
  		return;
 	}

 	if ( (pid = fork()) < 0 ) 
	{
  		internal_server_error(client);
  		return;
 	}
 	if (pid == 0)  /* child: CGI script */
 	{
  		char meth_env[255];
  		char query_env[255];
  		char length_env[255];

  		dup2(cgi_output[1], 1);
  		dup2(cgi_input[0], 0);
  		close(cgi_output[0]);
  		close(cgi_input[1]);
  		sprintf(meth_env, "REQUEST_METHOD=%s", method);
  		putenv(meth_env);
  		if (strcasecmp(method, "GET") == 0) 
		{
   			sprintf(query_env, "QUERY_STRING=%s", query_string);
   			putenv(query_env);
  		}
  	else 
	{   
   		sprintf(length_env, "CONTENT_LENGTH=%d", content_length);
   		putenv(length_env);
  	}
  	execl(path, path, NULL);
  	exit(0);
 	} 
	else 
	{    /* parent */
  		close(cgi_output[1]);
  		close(cgi_input[0]);
  		if (strcasecmp(method, "HEAD") == 0)
   			for (i = 0; i < content_length; i++) 
			{
    				recv(client, &c, 1, 0);
    				write(cgi_input[1], &c, 1);
   			}
  	while (read(cgi_output[0], &c, 1) > 0)
   		send(client, &c, 1, 0);

  	close(cgi_output[0]);
  	close(cgi_input[1]);
  	waitpid(pid, &status, 0);
 	}
}

//get new line from a socket and terminate with \n
int get_line(int sock, char *buf, int size)
{
 	int i = 0;
 	char c = '\0';
 	int n;

 	while ((i < size - 1) && (c != '\n'))
 	{
  		n = recv(sock, &c, 1, 0); 		
  		if (n > 0)
  		{
   			if (c == '\r')
   			{
    			n = recv(sock, &c, 1, MSG_PEEK);
       			if ((n > 0) && (c == '\n'))
     				recv(sock, &c, 1, 0);
    			else
     				c = '\n';
   			}
   			buf[i] = c;
   			i++;
  		}
  		else
   			c = '\n';
 	}
 	buf[i] = '\0';
 
 	return(i);
}

//return http header information
//refers to 200 response code
void headers(int client, const char *filename)
{
 	char buf[BUFF_SIZE];
 	(void)filename;  

 	strcpy(buf, "HTTP/1.0 200 OK\r\n");
 	send(client, buf, strlen(buf), 0);
 	strcpy(buf, SERVER_STRING);
 	send(client, buf, strlen(buf), 0);
 	sprintf(buf, "Content-Type: text/html\r\n");
 	send(client, buf, strlen(buf), 0);
 	strcpy(buf, "\r\n");
 	send(client, buf, strlen(buf), 0);
}

//handle not found condition
void not_found(int client)
{
 	char buf[BUFF_SIZE];

 	sprintf(buf, "HTTP/1.0 404 NOT FOUND\r\n");
 	send(client, buf, strlen(buf), 0);
 	sprintf(buf, SERVER_STRING);
 	send(client, buf, strlen(buf), 0);
 	sprintf(buf, "Content-Type: text/html\r\n");
 	send(client, buf, strlen(buf), 0);
 	sprintf(buf, "\r\n");
 	send(client, buf, strlen(buf), 0);
 	sprintf(buf, "<HTML><TITLE>Not Found</TITLE>\r\n");
 	send(client, buf, strlen(buf), 0);
 	sprintf(buf, "<BODY><P>The server could not fulfill\r\n");
 	send(client, buf, strlen(buf), 0);
 	sprintf(buf, "your request because the resource specified\r\n");
 	send(client, buf, strlen(buf), 0);
 	sprintf(buf, "is unavailable or nonexistent.\r\n");
 	send(client, buf, strlen(buf), 0);
 	sprintf(buf, "</BODY></HTML>\r\n");
 	send(client, buf, strlen(buf), 0);
}

//handle forbidden condition
void forbidden(int client)
{
	char buf[BUFF_SIZE];

 	sprintf(buf, "HTTP/1.0 403 FORBIDDEN\r\n");
 	send(client, buf, strlen(buf), 0);
 	sprintf(buf, SERVER_STRING);
 	send(client, buf, strlen(buf), 0);
 	sprintf(buf, "Content-Type: text/html\r\n");
 	send(client, buf, strlen(buf), 0);
 	sprintf(buf, "\r\n");
 	send(client, buf, strlen(buf), 0);
 	sprintf(buf, "<HTML><TITLE>FORBIDDEN</TITLE>\r\n");
 	send(client, buf, strlen(buf), 0);
 	sprintf(buf, "<BODY><P>You do not have permission to acess / on this server\r\n");
 	send(client, buf, strlen(buf), 0);
 	sprintf(buf, "</BODY></HTML>\r\n");
 	send(client, buf, strlen(buf), 0);
}

//GET method
void get_method(int client, const char *filename)
{
 	FILE *resource = NULL;
 	int numchars = 1;
 	char buf[BUFF_SIZE];

 	buf[0] = 'A'; 
 	buf[1] = '\0';
 	while ((numchars > 0) && strcmp("\n", buf))  /* read & discard headers */
  		numchars = get_line(client, buf, sizeof(buf));

 	resource = fopen(filename, "r");  
 	if (resource == NULL)
  		not_found(client);
 	else
 	{
  		headers(client, filename);
  		cat(client, resource);
 	}
 	fclose(resource);
}

//HEAD method
void head_method(int client, const char *filename)
{
	FILE *resource = NULL;
 	int numchars = 1;
 	char buf[BUFF_SIZE];

 	buf[0] = 'A'; 
 	buf[1] = '\0';
 	while ((numchars > 0) && strcmp("\n", buf))  /* read & discard headers */
  		numchars = get_line(client, buf, sizeof(buf));

 	resource = fopen(filename, "r");
 	if (resource == NULL)
  		not_found(client);
 	else
 	{
  		headers(client, filename);
 	}
 	fclose(resource);
}

//start listen on a specific or dynamically allocated port
int startup(u_short *port)
{
 	int sock = 0;
 	struct sockaddr_in name;

 	sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
 	if (sock == -1)
  		ERR_EXIT("socket error");
 	memset(&name, 0, sizeof(name));
 	name.sin_family = AF_INET;
 	name.sin_port = htons(*port);
 	name.sin_addr.s_addr = htonl(INADDR_ANY);//
 	if (bind(sock, (struct sockaddr *)&name, sizeof(name)) < 0)
  		ERR_EXIT("bind error");
  	//dynamically allocate a port
 	if (*port == 0)  
 	{
  		socklen_t namelen = sizeof(name);
  		if (getsockname(sock, (struct sockaddr *)&name, &namelen) == -1)
   		ERR_EXIT("getsockname error");
  		*port = ntohs(name.sin_port);
 	}
 	if (listen(sock, SOMAXCONN) < 0)
  		ERR_EXIT("listen error");
 	return(sock);
}

// reponse code 501 method not implemented
void unimplemented(int client)
{
 	char buf[BUFF_SIZE];

 	sprintf(buf, "HTTP/1.0 501 Method Not Implemented\r\n");
 	send(client, buf, strlen(buf), 0);
 	sprintf(buf, SERVER_STRING);
 	send(client, buf, strlen(buf), 0);
 	sprintf(buf, "Content-Type: text/html\r\n");
 	send(client, buf, strlen(buf), 0);
 	sprintf(buf, "\r\n");
 	send(client, buf, strlen(buf), 0);
 	sprintf(buf, "<HTML><HEAD><TITLE>Method Not Implemented\r\n");
 	send(client, buf, strlen(buf), 0);
 	sprintf(buf, "</TITLE></HEAD>\r\n");
 	send(client, buf, strlen(buf), 0);
 	sprintf(buf, "<BODY><P>HTTP request method not supported.\r\n");
 	send(client, buf, strlen(buf), 0);
 	sprintf(buf, "</BODY></HTML>\r\n");
 	send(client, buf, strlen(buf), 0);
}

int main(int argc, char* argv[])
{
 	int server_sock = -1;
 	u_short default_port = 0;
 	int client_sock = -1;
	int status=99;
 	struct sockaddr_in client_name;
 	socklen_t client_name_len = sizeof(client_name);
 	pid_t pid;

	//Command line options	
	if (argc < 2)
	{
		server_sock = startup(&default_port);
		printf("Server starting on  port: %d\n", default_port);
	}else if (argc == 2){
	    if (!strcmp(argv[1],"-h"))
	    {
	        printf("To run server type: ./webserver -p <port number>\n");
	        exit(1);
	    }else
	        printf("Wrong input,for help type : ./webserver -h\n");
		exit(1);
	}else if(( argc == 3) && (!strcmp(argv[1],"-p"))){
		u_short dynamic_port = (u_short)strtoul(argv[2], NULL, 0);   
		server_sock = startup(&dynamic_port);
		printf("Starting server on chosen port, %s\n",argv[2]);
	}
 	while(1)
 	{		
		client_sock = accept(server_sock, (struct sockaddr *)&client_name, &client_name_len);
  		if(client_sock < 0)
  			ERR_EXIT("accept error");
  		
  			//handle mutiple request
  			//achieve fork() method
  		pid = fork();
  		if(pid == -1)
  		{
  			ERR_EXIT("fork error");
  		}
  		else if(pid == 0)
  		{
  			accept_request(client_sock);
  			close(server_sock);
			exit(0);
  		}else{   // avoid zombie
			waitpid(pid,&status,0);  			
			close(client_sock);
  		}
 	}
 	return(0);
}
