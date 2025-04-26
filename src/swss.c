#include "../include/swss.h"
#include "../include/utils.h"
#include <endian.h>
#include <sys/types.h>

static const char *protocol_error = "\x03\xea"; // 1002
static ws_callbacks_t *g_callbacks;

void ws_exit()
{
    printf("Exiting...\n");
    free(g_callbacks);
    exit(0);
}

int ws_handshake(int sockfd)
{
    char buf[1024], *key;
    recv(sockfd, buf, 1024, 0);

    key = strstr(buf, "Sec-WebSocket-Key:") + 19;
    key = strtok(key, "\r\n");

    if (key == NULL)
    {
        return -1;
    }

    char *response = ws_createAcceptToken(key);

    if ((send(sockfd, response, strlen(response), 0)) == -1)
    {
        perror("send");
        return -1;
    }

    free(response);
    // TODO: FREE THE KEY WITHOUT SEGFAULT
    return 0;
}

int ws_send_response(int client_fd, u_int8_t opcode, u_int8_t *payload,
                     u_int64_t payload_len, u_int8_t mask)
{
    if (client_fd < 0)
    {
        return -1;
    }

    size_t frame_header_size = 2;
    u_int8_t payload_len_specifier = 0;

    if (payload_len == NULL || payload_len <= 125)
    {
        payload_len_specifier = payload_len;
    }
    else if (payload_len < 65536)
    {
        payload_len_specifier = 126;
        frame_header_size += 2;
    }
    else
    {
        payload_len_specifier = 127;
        frame_header_size += 8;
    }

    u_int8_t *frame = malloc(frame_header_size + payload_len);
    if (!frame)
    {
        return -1;
    }

    // 0x80 -> 1000 0000, fin rsrv1 rsrv2 rsrv3 opcode(4 bit)
    frame[0] = 0x80 | opcode;

    // if mask is enabled then first bit of second byte is set to 1 and rest is
    // payload length specifier
    if (mask)
    {
        frame[1] = 0x80;
    }
    else
    {
        frame[1] = 0x00;
    }

    frame[1] |= payload_len_specifier;

    if (payload_len_specifier == 126)
    {
        // host order to network order (little endian to big endian) for length of 2
        // bytes
        size_t len = htons(payload_len);
        memcpy(frame + 2, &len, 2);
    }
    else if (payload_len_specifier == 127)
    {
        // host order to network order (little endian to big endian) for length of
        // 64 bytes
        size_t len = htobe64(payload_len);
        memcpy(frame + 2, &len, 8);
    }

    if (mask)
    {
        u_int8_t mask_key[4];
        mask_key[0] = rand() % 256;
        mask_key[1] = rand() % 256;
        mask_key[2] = rand() % 256;
        mask_key[3] = rand() % 256;

        memcpy(frame + frame_header_size, mask_key, 4);
        frame_header_size += 4;
    }

    // Copy payload at correct offset
    if (payload != NULL && payload_len > 0)
    {
        memcpy(frame + frame_header_size, payload, payload_len);
    }

    size_t total_size = frame_header_size + payload_len;
    ssize_t bytes_sent = send(client_fd, frame, total_size, 0);
    free(frame);

    if (bytes_sent == -1)
    {
        return -1;
    }

    return 0;
}

// reads one control frame (exit, ping, pong)
// OR
// reads one message frame, possibly interspersed with any number of control frames (if fragmented)
int read_frame(int sock_fd)
{
    if (sock_fd < 0)
    {
        return -1;
    }

    printf("\nReading frame\n");
    u_int8_t original_opcode = 0; // for fragmented messages
    u_int8_t *final_payload = NULL;
    u_int64_t final_payload_len = 0;
    ssize_t recv_result;

    while (1)
    {
        u_int8_t buf[2];
        u_int64_t received_len = 0;
        while (received_len != 2)
        {
            recv_result = recv(sock_fd, buf + received_len, 2 - received_len, 0);
            if (recv_result <= 0)
            {
                free(final_payload);
                return -1;
            }
            received_len += recv_result;
        }

        u_int8_t fin = (buf[0] & 0x80) >> 7;
        u_int8_t rsv = buf[0] & 0x70;
        u_int8_t opcode = buf[0] & 0x0F;

        printf("FIN: %d\n", fin);

        u_int8_t mask = (buf[1] & 0x80) >> 7;
        printf("Mask: %d\n", mask);

        switch (opcode)
        {
        case 0x0:
            printf("Continuation Frame\n");
            break;
        case 0x1:
            printf("Text Frame\n");
            break;
        case 0x2:
            printf("Binary Frame\n");
            break;
        case 0x8:
            printf("Close Frame\n");
            break;
        case 0x9:
            printf("Ping Frame\n");
            break;
        case 0xA:
            printf("Pong Frame\n");
            break;
        }

        u_int64_t payload_len = buf[1] & 0x7F;

        printf("Payload Length: %lu\n", payload_len);

        if (opcode >= 0x8 && payload_len > 125)
        {
            printf("Invalid Extended Payload Frame\n");
            free(final_payload);
            return -1;
        }
        if (payload_len == 126)
        {
            u_int64_t received_len = 0;
            while (received_len != 2)
            {
                recv_result = recv(sock_fd, ((char *)&payload_len) + received_len, 2 - received_len, 0);
                if (recv_result <= 0)
                {
                    free(final_payload);
                    return -1;
                }
                received_len += recv_result;
            }
            payload_len = be64toh(payload_len);
            payload_len = payload_len >> (6*8);
            printf("2-Extended Payload Length: %lu\n", payload_len);
        }
        else if (payload_len == 127)
        {
            u_int64_t received_len = 0;
            while (received_len != 8)
            {
                recv_result = recv(sock_fd, ((char *)&payload_len) + received_len, 8 - received_len, 0);
                if (recv_result <= 0)
                {
                    free(final_payload);
                    return -1;
                }
                received_len += recv_result;
            }
            payload_len = be64toh(payload_len);
            printf("8-Extended Payload Length: %lu\n", payload_len);
        }

        u_int8_t mask_key[4];
        if (mask == 1)
        {
            u_int64_t received_len = 0;
            while (received_len != 4)
            {
                recv_result = recv(sock_fd, mask_key + received_len, 4 - received_len, 0);
                if (recv_result <= 0)
                {
                    free(final_payload);
                    return -1;
                }
                received_len += recv_result;
            }
            printf("Mask Key: %d %d %d %d\n", mask_key[0], mask_key[1], mask_key[2],
                   mask_key[3]);
        }

        u_int8_t *payload = NULL;
        if (payload_len > 0)
        {
            payload = malloc(payload_len);
            if (!payload)
            {
                free(final_payload);
                return -1;
            }

            u_int64_t received_len = 0;
            while (received_len != payload_len)
            {
                recv_result = recv(sock_fd, payload + received_len, payload_len - received_len, 0);
                if (recv_result <= 0)
                {
                    free(payload);
                    free(final_payload);
                    return -1;
                }
                received_len += recv_result;
            }

            if (mask == 1)
            {
                for (int i = 0; i < payload_len; i++)
                {
                    payload[i] = payload[i] ^ mask_key[i % 4];
                }
            }

            if (opcode <= 0x2)
            {
                u_int8_t *new_final_payload = realloc(final_payload, final_payload_len + payload_len);
                if (!new_final_payload)
                {
                    free(payload);
                    free(final_payload);
                    return -1;
                }
                final_payload = new_final_payload;

                memcpy(final_payload + final_payload_len, payload, payload_len);
                final_payload_len += payload_len;
            }
        }

        // reserved / future (not supported) opcodes
        if ((3 <= opcode && opcode <= 7) || opcode > 10)
        {
            ws_send_response(sock_fd, 0x8, protocol_error, 2, 0);
            free(payload);
            free(final_payload);
            return -1;
        }
        // this does not support any protocol extensions
        if (rsv != 0)
        {
            ws_send_response(sock_fd, 0x8, protocol_error, 2, 0);
            free(payload);
            free(final_payload);
            return -1;
        }

        switch (opcode)
        {
        case 0x0:
            if (original_opcode == 0)
            {
                // continuing, but never got a non-fin start?
                ws_send_response(sock_fd, 0x8, protocol_error, 2, 0);
                free(payload);
                free(final_payload);
                return -1;
            }
            // fall through
        case 0x1:
        case 0x2:
            if (opcode != 0)
            {
                if (original_opcode != 0)
                {
                    // tried to start a new message while a fragmented one was in-progress
                    ws_send_response(sock_fd, 0x8, protocol_error, 2, 0);
                    free(payload);
                    free(final_payload);
                    return -1;
                }
                original_opcode = opcode;
            }
            if (fin != 1)
            {
                free(payload);
                continue;
            }
            break;
        case 0x8:
        case 0x9:
        case 0xA:
            if (fin != 1 || (opcode == 0x8 && payload_len == 1))
            {
                ws_send_response(sock_fd, 0x8, protocol_error, 2, 0);
                free(payload);
                free(final_payload);
                return -1;
            }
            break;
        }

        switch (opcode)
        {
        case 0x0:
            if (original_opcode == 0)
            {
                // finished a continue, but never got a non-fin start?
                ws_send_response(sock_fd, 0x8, protocol_error, 2, 0);
                free(payload);
                free(final_payload);
                return -1;
            }
            // fall through
        case 0x1:
        case 0x2:
            g_callbacks->on_message(sock_fd, (original_opcode == 0x1) ? 1 : 0,
                (final_payload_len > 0 && final_payload != NULL) ? (const char *)final_payload : "\0",
                (final_payload_len > 0) ? final_payload_len : 0
            );
            free(payload);
            free(final_payload);
            return 1;
            break;
        case 0x8:
            u_int16_t reason = 1000;
            if(payload_len != 0 && payload != NULL)
            {
                reason = be16toh((((u_int16_t)payload[1]) << 8) | payload[0]);
            }
            switch (reason)
            {
            case 1000 ... 1003:
            case 1007 ... 1011:
            case 3000:
            case 3999:
            case 4000:
            case 4999:
                ws_send_response(sock_fd, 0x8,
                    (payload_len != 0 && payload != NULL) ? (const char *)payload : "\x03\xe8", // 1000
                2, 0);
                break;
            default:
                ws_send_response(sock_fd, 0x8, protocol_error, 2, 0);
                break;
            }
            free(payload);
            free(final_payload);
            return -1;
        case 0x9:
            ws_send_response(sock_fd, 0xA,
                (payload_len > 0 && payload != NULL) ? payload : NULL,
                (payload_len > 0) ? payload_len : 0,
            0);
            // fall through
        case 0xA:
            free(payload);
            if (original_opcode == 0)
            {
                free(final_payload);
                return 1;
            }
            break;
        }
    }

    // should never be hit, will have to explicitly return after a fin above
    return 1;
}

void *handle_client(void *arg)
{
    int clientfd = *((int *)arg);
    free(arg);

    // handshake failed
    if (ws_handshake(clientfd) != 0)
    {
        perror("ws_handshake");
        printf("Client Disconnected\n");
        send(clientfd, "HTTP/1.1 400 Bad Request\r\n", 25, 0);
        close(clientfd);
    }

    char buf[4096];

    while (1)
    {
        int res = read_frame(clientfd);
        if (res == -1)
        {
            break;
        }
    }

    g_callbacks->on_close(clientfd);
    close(clientfd);
    return NULL;
}

void ws_init(ws_callbacks_t *callbacks) { g_callbacks = callbacks; }

// Wrapper function for sending text messages
int ws_send_txt(int client_fd, const char *message, size_t length)
{
    return ws_send_response(client_fd, 0x1, (u_int8_t *)message, length, 0);
}

// Wrapper function for sending binary payloads
int ws_send_bin(int client_fd, const u_int8_t *payload, size_t length)
{
    return ws_send_response(client_fd, 0x2, (u_int8_t *)payload, length, 0);
}

// Setup TCP server and listen for incoming connections
int ws_listen(const char *PORT)
{

    if (g_callbacks == NULL)
    {
        fprintf(stderr, "ws_init must be called before ws_listen\n");
        return -1;
    }

    struct addrinfo hints, *res, *p;
    int sockfd, clientfd, yes = 1;

    struct sockaddr_storage their_addr;
    socklen_t sin_size = sizeof(their_addr);

    memset(&hints, 0, sizeof(hints));

    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;

    if (getaddrinfo(NULL, PORT, &hints, &res) != 0)
    {
        perror("getaddrinfo");
        return -1;
    }
    for (p = res; p != NULL; p = p->ai_next)
    {
        if ((sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1)
        {
            continue;
        }

        if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1)
        {
            close(sockfd);
            continue;
        }

        if (bind(sockfd, p->ai_addr, p->ai_addrlen) == -1)
        {
            close(sockfd);
            continue;
        }

        break;
    }

    freeaddrinfo(res);

    if (p == NULL)
    {
        perror("socket");
        return -1;
    }

    if (listen(sockfd, 10) == -1)
    {
        perror("listen");
        return -1;
    }

    printf("Listening on port %s\n", PORT);

    while (1)
    {
        if ((clientfd =
                 accept(sockfd, (struct sockaddr *)&their_addr, &sin_size)) == -1)
        {
            perror("accept");
            continue;
        }

        int *client_sock = malloc(sizeof(int));
        *client_sock = clientfd;
        g_callbacks->on_open(clientfd);

        pthread_t thread_id;
        if (pthread_create(&thread_id, NULL, handle_client, (void *)client_sock) !=
            0)
        {
            perror("pthread_create");
            close(clientfd);
            free(client_sock);
            continue;
        }
        pthread_detach(thread_id);
    }

    return 0;
}
