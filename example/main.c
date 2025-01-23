#include <swss/swss.h>
#include <time.h>

#define MAX_CLIENTS 10

int client_count = 0;
int clients[MAX_CLIENTS];

void print_timestamp()
{
    time_t now = time(NULL);
    char *time_str = ctime(&now);
    time_str[strlen(time_str) - 1] = '\0'; // Remove newline
    printf("[%s] ", time_str);
}

void my_on_open(int client_fd)
{
    if (client_count < MAX_CLIENTS)
    {
        clients[client_count] = client_fd;
        client_count++;
        print_timestamp();
        printf("Client %d connected (Total clients: %d)\n", client_fd, client_count);
    }
}

void my_on_message(int client_fd, const char *message, size_t length)
{
    print_timestamp();
    printf("Broadcasting message from client %d to all other clients\n", client_fd);

    for (int j = 0; j < client_count; j++)
    {
        if (clients[j] != client_fd)
        {
            ws_send_txt(clients[j], message, length);
        }
    }
}

void my_on_close(int client_fd)
{
    for (int i = 0; i < client_count; i++)
    {
        if (clients[i] == client_fd)
        {
            // Shift remaining clients to fill the gap
            for (int j = i; j < client_count - 1; j++)
            {
                clients[j] = clients[j + 1];
            }
            client_count--;
            break;
        }
    }
    print_timestamp();
    printf("Client %d disconnected (Remaining clients: %d)\n", client_fd, client_count);
}

void my_on_error(int client_fd, int error_code)
{
    fprintf(stderr, "Error on client %d: %d\n", client_fd, error_code);
}

int main()
{
    ws_callbacks_t callbacks = {.on_open = my_on_open,
                                .on_message = my_on_message,
                                .on_close = my_on_close,
                                .on_error = my_on_error};

    ws_init(&callbacks);

    ws_listen("8080");
}
