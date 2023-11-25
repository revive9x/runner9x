#include "ws.h"
#include "rlog.h"

#include <stdio.h>
#include <stdlib.h>
#include <winsock.h>
#include <windows.h>
#include <shlwapi.h>
#include <strsafe.h>

#define HOST "192.168.1.9"
#define PORT 27015

bool kill_sub(PROCESS_INFORMATION *pi) {
	int res = TerminateProcess(pi->hProcess, 1);

	if(res) {
		rlog("Child process terminated.");
	} else {
		rlog("Could not terminate child process.");
	}

	CloseHandle(pi->hProcess);
	CloseHandle(pi->hThread);
	return res;
}

int main(void) {
	rlog("Starting r9x-dbg client..");

	char* temp_directory = getenv("TEMP");

	printf("\nServer host: %s\n", HOST);
	printf("Server port: %i\n", PORT);
	printf("Temp directory: %s\n\n", temp_directory);

	if(!socket_startup()) {
		rlog("Could not start Winsock.");
		return EXIT_FAILURE;
	}

	SOCKET sock;
	if(!socket_connect(&sock, HOST, PORT)) {
		rlog("Could not connect to r9x-dbg host..");
		return EXIT_FAILURE;
	}
	
	rlog("Sending hello message...");
	
	socket_send(&sock, "HELLO");

	char *res = socket_recv(&sock);
	if(strcmp(res, "ACK") != 0) {
		rlog("Unknown response from host. Disconnecting.");
		return EXIT_FAILURE;
	}
	free(res);
	
	// Process managment
	STARTUPINFO si;
	PROCESS_INFORMATION pi;
	ZeroMemory(&si, sizeof(si));
	si.cb = sizeof(si);
	ZeroMemory(&pi, sizeof(pi));
	bool kill = false;

	char *key = NULL;
	char *val = NULL;

	while(1) {
		rlog("Waiting for commands..");
		char *cmd = socket_recv(&sock);

		if(cmd == NULL) {
			rlog("Connection to server lost.");
			if(kill) {
				kill_sub(&pi);	
			}
			return -1;
		}
		
		rlog("Received command from host: '%s'", cmd);
		
		// Check if command has an argument
		if(strchr(cmd, ' ') != NULL) {
			key = strtok(cmd, " ");
			val = strtok(NULL, " ");
			
			// Run a new binary
			if(strcmp(key, "RUN") == 0) {
				int bytes = atoi(val);

				if(bytes == 0) {
					socket_send(&sock, "NACK");
					continue;
				}

				// Kill previous process
				if(kill) {
					if(!kill_sub(&pi)) {
						socket_send(&sock, "NACK");
					} 
					
					kill = false;
				}
					
				rlog("Receiving file. size=%i bytes", bytes);

				char* target_path = malloc((strlen(target_path) + strlen(temp_directory) + 1) * sizeof(char));
				PathCombineA(target_path, temp_directory, "r9x-dbg.exe");
				
				rlog("Writing file to target '%s'", target_path);
				
				if(PathFileExistsA(target_path)) {
					rlog("Removed old file..");
					DeleteFileA(target_path);
				}

				// Ready to receive files
				socket_send(&sock, "ACK");	
				
				int read = 0;
				FILE *fp = fopen(target_path, "ab+");
				char *buffer = malloc(4096 * sizeof(char));

				while(read != bytes) {
					int got = recv(sock, buffer, 4096, 0);
					read += got;
					fwrite(buffer, got, 1, fp); 
				}

				rlog("Received file.");
				fclose(fp);

				rlog("Attempting to execute received program.");
				ZeroMemory(&si, sizeof(si));
				si.cb = sizeof(si);
				ZeroMemory(&pi, sizeof(pi));
				
				// ah yes - the better 'execve()'
				if(CreateProcess(NULL, target_path, NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi)) {
					kill = true;
				} else {
					rlog("Could not spawn process. Maybe the .exe file is corrupted: Error: (%d)", GetLastError());
				}

			}

		// Command without argument
		} else {
			if(strcmp(cmd, "SUBKILL") == 0) {
				if(!kill) {
					socket_send(&sock, "NACK");
					continue;
				}

				if(kill_sub(&pi)) {
					socket_send(&sock, "ACK");
				} else {
					socket_send(&sock, "NACK");
				}

				kill = false;
			}
		}
		
		// 'cmd' is no longer needed, free it
		free(cmd);
	}


	return EXIT_SUCCESS;

}
