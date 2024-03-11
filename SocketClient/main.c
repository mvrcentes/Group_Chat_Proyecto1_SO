#include "socketutil.h"

int main() {
    int socketFD = createTCPIpv4Socket(); //file descriptor
    struct sockaddr_in *address = createIPv4Address("142.250.188.46", 80);

    int result = connect(socketFD, address, sizeof (*address));

    if (result == 0) {
        printf("Connected to the server\n");
    }

    char* message;
    message = "GET \\ HTTP/1.1\r\nHost:google.com\r\n\r\n";
    send(socketFD, message, strlen(message), 0);

    char buffer[1024];
    recv(socketFD, buffer, 1024, 0);

    printf("Response was %s\n", buffer);

    return 0;
}


