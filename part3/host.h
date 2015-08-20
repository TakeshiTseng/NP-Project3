#ifndef __HOST_H__
#define __HOST_H__
#include <stdio.h>


struct host {
    // xxx.xxx.xxx.xxx
    char hostname[21];
    int port;
    char filename[128];
    FILE* host_file;
    int is_connect;
	SOCKET server_socket;
};

typedef struct host host_t;

void create_host(host_t** host, char* hostname, int port, char* filename);

#endif
