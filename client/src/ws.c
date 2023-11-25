#include <stdbool.h>
#include <winsock.h>
#include "rlog.h"

int socket_startup() {
	// Start winsock
	WSADATA wsa;
	if(WSAStartup(MAKEWORD(1, 1), &wsa) != 0) {
		return false;
	}
	return true;
}

bool socket_connect(SOCKET* sock, char* host, int port) {
	// Create socket
	*sock = socket(AF_INET, SOCK_STREAM, 0);
	if (*sock == INVALID_SOCKET) {
		WSACleanup();
    		return false;
	}

	// setup sockaddr struct
	struct sockaddr_in server_addr;
	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons(port);
	server_addr.sin_addr.s_addr = inet_addr(host);

	// Attempt to connect
	if (connect(*sock, (struct sockaddr*) &server_addr, sizeof(server_addr)) < 0) {
		closesocket(*sock);
    		WSACleanup();
    		return false;
	}
	return true;
}

void socket_send(SOCKET* sock, char* msg) {
	send(*sock, msg, strlen(msg), 0);
}

char* socket_recv(SOCKET* sock) {
	char *buff = malloc(sizeof(char) * 4096);

	int got = recv(*sock, buff, 4096, 0);
	if(got == 0) {
		free(buff);
		return NULL;
	}

	return buff;
}
