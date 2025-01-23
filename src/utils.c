#include "../include/utils.h"
#include "../include/base64.h"

#define GUID "258EAFA5-E914-47DA-95CA-C5AB0DC85B11"
#define RESPONSE_SIZE 1024

char *ws_createAcceptToken(char *key)
{
    char *token = malloc(64);
    strcpy(token, key);
    strcat(token, GUID);

    unsigned char sha_hash[SHA_DIGEST_LENGTH];
    SHA1((unsigned char *)token, strlen(token), sha_hash);

    size_t output_length;
    char *accept_token = base64_encode(sha_hash, SHA_DIGEST_LENGTH, &output_length);
    char *response = ws_generateHTTPResponse(accept_token);

    free(token);
    free(accept_token);

    return response;
}

char *ws_generateHTTPResponse(char *key)
{
    char *response = malloc(RESPONSE_SIZE);

    snprintf(response, RESPONSE_SIZE,
             "HTTP/1.1 101 Switching Protocols\r\n"
             "Upgrade: websocket\r\n"
             "Connection: Upgrade\r\n"
             "Sec-WebSocket-Accept: %s\r\n\r\n",
             key);

    return response;
}
