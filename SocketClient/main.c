#include <stdbool.h>
#include <unistd.h>
#include <pthread.h>
#include "socketutil.h"

void startListeningAndPrintMessagesOnNewThread(int fd);

void listenAndPrint(int socketFD);

int main() {
    int socketFD = createTCPIpv4Socket(); //file descriptor
    struct sockaddr_in *address = createIPv4Address("127.0.0.1", 2000);

    int result = connect(socketFD, address, sizeof (*address));

    if (result == 0) {
        printf("Conectado al servidor\n");
    }

    char *line = NULL;
    size_t lineSize = 0;
    printf("Escribir mensaje...\n");

    startListeningAndPrintMessagesOnNewThread(socketFD);

    while (true) {
        ssize_t charCount = getline(&line, &lineSize, stdin);

        if (charCount > 0) {
            if (strcmp(line, "exit\n") == 0)
                break;

            ssize_t amountWasSent = send(socketFD, line, charCount, 0);
        }
    }

    close(socketFD);

    return 0;
}

void startListeningAndPrintMessagesOnNewThread(int socketFD) {
    pthread_t id;
    pthread_create(&id, NULL, (void *(*)(void *)) listenAndPrint, (void *) socketFD);
}

void listenAndPrint(int socketFD) {
    char buffer[1024];
    while (true) {
        ssize_t ammountReceived = recv(socketFD, buffer, 1024, 0);

        if (ammountReceived > 0) {
            buffer[ammountReceived] = 0;
            printf("Respuesta: %s\n", buffer);
        }

        if (ammountReceived == 0)
            break;
    }

    close(socketFD);
}


