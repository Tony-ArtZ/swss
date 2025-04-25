#ifndef SWSS_H
#define SWSS_H

#include <arpa/inet.h>
#include <errno.h>
#include <netdb.h>
#include <netinet/in.h>
#include <pthread.h>
#include <signal.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

typedef struct
{
    void (*on_open)(int client_fd);
    void (*on_message)(int client_fd, int text, const char *message, size_t length);
    void (*on_close)(int client_fd);
    void (*on_error)(int client_fd, int error_code);
} ws_callbacks_t;

int ws_listen(const char *PORT);
void *handle_client(void *arg);
int ws_send_txt(int client_fd, const char *message, size_t length);
int ws_send_bin(int client_fd, const u_int8_t *payload, size_t length);
void ws_init(ws_callbacks_t *callbacks);
#define MAX_FRAME_SIZE 1024

#endif /* SWSS_H */
