#ifndef RV_9X_SOCK
#define RV_9X_SOCK

#include <stdbool.h>
#include <winsock.h>

int socket_startup();

bool socket_connect(SOCKET* sock, char* host, int port);
void socket_send(SOCKET* sock, char *msg);	
char* socket_recv(SOCKET* sock);	

#endif
