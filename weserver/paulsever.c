
//This structure holds socket address information for many types of sockets
struct sockaddr{
	unsigned short  	sa_family;  // address family, AF_xxx
	char 				sa_data[14];// 14 bytes of protocol address
}

struct sockaddr_in{
	short int 			sin_family;// Address family
	unsigned short int 	sin_port;// Port number
	struct in_addr		sin_addr;// Internet address
	unsigned char 		sin_zero[8];// Same size as struct sockaddr
}


