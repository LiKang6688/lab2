#include <stdio.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <ctype.h>
#include <strings.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <stdlib.h>

#define ISspace(x) isspace((int)(x))
#define BUFF_SIZE 1024
#define MAX_URL_LEN 2083
#define ERR_EXIT(m) \
    do { \
        perror(m); \
        exit(EXIT_FAILURE); \
    } while (0)
#define SERVER_STRING "Server: Stone Chen Web Server\r\n"

void accept_request(int);
void bad_request(int);
void cat(int, FILE *);
void internal_server_error(int);
void execute_cgi(int, const char *, const char *, const char *);
int get_line(int, char *, int);
void headers(int, const char *);
void not_found(int);
void forbidden(int);
void get_method(int, const char *);
void head_method(int, const char *);
int startup(u_short *);
void unimplemented(int);
int URL_validation(char *);
