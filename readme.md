# SWSS (Simple WebSocket Server Library)

A lightweight, high-performance WebSocket server implementation in C, fully compliant with RFC 6455. Built on Linux sockets with support for multi-threaded connections and message fragmentation.

## Features

- **Full RFC 6455 Compliance**: Implements the WebSocket protocol as specified in [RFC 6455](https://datatracker.ietf.org/doc/html/rfc6455)
- **Frame Types Support**:
  - Text frames (0x1)
  - Binary frames (0x2)
  - Close frames (0x8)
  - Ping frames (0x9)
  - Pong frames (0xA)
  - Continuation frames (0x0)
- **Robust Frame Handling**:
  - Handles fragmented messages
  - Supports both masked and unmasked frames
  - Handles variable payload lengths (7-bit, 16-bit, and 64-bit lengths)
- **Event-Driven Architecture**:
  - Connection open/close events
  - Message reception events
  - Error handling events
- **Thread-Safe**: Each client connection runs in its own thread
- **Zero Dependencies**: Only requires standard C libraries

## Installation

```bash
# Clone the repository
git clone https://github.com/yourusername/swss-lib.git
cd swss-lib

# Build the library
make

# Install (requires root privileges)
sudo make install

# Link the shared libraries
sudo ldconfig
```

## Project Structure

```
swss-lib/
├── include/
│   ├── swss.h       # Main header file
│   ├── utils.h      # Utility functions
│   └── base64.h     # Base64 encoding
├── src/
│   ├── swss.c       # Core WebSocket implementation
│   ├── utils.c      # Utility implementations
│   └── base64.c     # Base64 encoding implementation
├── example/
│   └── main.c       # Example chat server
├── Makefile
└── README.md
```

## Usage Example

```c
#include <swss/swss.h>

// Callback when client connects
void on_open(int client_fd) {
    printf("Client %d connected\n", client_fd);
}

// Callback when message is received
void on_message(int client_fd, const char *message, size_t length) {
    printf("Received message from client %d: %.*s\n", client_fd, (int)length, message);
    // Echo back to client
    ws_send_txt(client_fd, message, length);
}

// Callback when client disconnects
void on_close(int client_fd) {
    printf("Client %d disconnected\n", client_fd);
}

// Error handling callback
void on_error(int client_fd, int error_code) {
    printf("Error %d on client %d\n", error_code, client_fd);
}

int main() {
    // Initialize callback structure
    ws_callbacks_t callbacks = {
        .on_open = on_open,
        .on_message = on_message,
        .on_close = on_close,
        .on_error = on_error
    };

    // Initialize WebSocket server
    ws_init(&callbacks);

    // Start listening on port 8080
    ws_listen("8080");
    return 0;
}
```

## Building Your Application

```bash
gcc -o myapp myapp.c -lswss -lpthread
```

## Multi-Frame Support

The library handles message fragmentation automatically, allowing for:
- Large message transmission
- Streaming data
- Real-time updates
- Memory-efficient processing of large payloads

Example of how fragmentation works internally:
1. Messages larger than 65535 bytes are automatically fragmented
2. Continuation frames are properly tracked and assembled
3. Final message is delivered only when the FIN bit is received
4. Memory is managed efficiently during reassembly

### WebSocket Frame Structure

The implementation handles WebSocket frames according to RFC 6455 specification:

```
 0                   1                   2                   3
 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
+-+-+-+-+-------+-+-------------+-------------------------------+
|F|R|R|R| opcode|M| Payload len |    Extended payload length    |
|I|S|S|S|  (4)  |A|     (7)    |             (16/64)          |
|N|V|V|V|       |S|             |   (if payload len==126/127)   |
| |1|2|3|       |K|             |                               |
+-+-+-+-+-------+-+-------------+ - - - - - - - - - - - - - - - +
|     Extended payload length continued, if payload len == 127  |
+ - - - - - - - - - - - - - - - +-------------------------------+
|                               |Masking-key, if MASK set to 1  |
+-------------------------------+-------------------------------+
| Masking-key (continued)       |          Payload Data        |
+-------------------------------- - - - - - - - - - - - - - - - +
:                     Payload Data continued ...                :
+ - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - +
|                     Payload Data continued ...                |
+---------------------------------------------------------------+
```


## Thread Safety

The library is designed to be thread-safe:
- Each client connection runs in its own thread
- Global state is properly protected
- Callback functions are executed in the client's thread context
- Resources are automatically cleaned up on disconnection

## Limitations

- Currently supports Linux platforms only
- Maximum concurrent connections limited by system resources
- WebSocket client implementation not included


## Contributing

Contributions are welcome! Please feel free to submit a Pull Request.


```
  /$$$$$$$ /$$  /$$  /$$  /$$$$$$$ /$$$$$$$
 /$$_____/| $$ | $$ | $$ /$$_____//$$_____/
|  $$$$$$ | $$ | $$ | $$|  $$$$$$|  $$$$$$ 
 \____  $$| $$ | $$ | $$ \____  $$\____  $$
 /$$$$$$$/|  $$$$$/$$$$/ /$$$$$$$//$$$$$$$/
|_______/  \_____/\___/ |_______/|_______/ 

 ```                         
