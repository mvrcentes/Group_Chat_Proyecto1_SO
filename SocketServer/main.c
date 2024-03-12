#include <stdbool.h>
#include <unistd.h>
#include "socketutil.h"

struct AcceptedSocket {
    int acceptedSocketFD;
    struct sockaddr_in address;
    int error;
    bool acceptedSuccessfully;
};

struct AcceptedSocket * acceptIncomingConnection(int serverSockerFD);

int main() {

    int serverSockerFD = createTCPIpv4Socket();
    // conexiones entrantes
    struct sockaddr_in *serverAddress = createIPv4Address("", 2000);

    int result = bind(serverSockerFD, serverAddress, sizeof(*serverAddress));
    if (result == 0)
        printf("Link correcto con el socket\n");


    // Cuantos se van a conectar
    int listenResult = listen(serverSockerFD, 10);


    struct AcceptedSocket* clientSocket = acceptIncomingConnection(serverSockerFD);

    char buffer[1024];
    while (true) {
        ssize_t ammountReceived = recv(clientSocket->acceptedSocketFD, buffer, 1024, 0);

        if (ammountReceived > 0) {
            buffer[ammountReceived] = 0;
            printf("Respuesta: %s\n", buffer);
        }

        if (ammountReceived == 0)
            break;
    }

    close(clientSocket->acceptedSocketFD);
    shutdown(serverSockerFD, SHUT_RDWR);

    return 0;
}

struct AcceptedSocket * acceptIncomingConnection(int serverSockerFD) {
    struct sockaddr_in clientAddress;
    int clientAddressSize = sizeof (struct sockaddr_in);
    int clientSocketFD = accept(serverSockerFD, &clientAddress, &clientAddressSize);

    struct AcceptedSocket* acceptedSocket = malloc(sizeof (struct AcceptedSocket));
    acceptedSocket->address = clientAddress;
    acceptedSocket->acceptedSocketFD = clientSocketFD;
    acceptedSocket->acceptedSuccessfully = clientSocketFD > 0;

    if (!acceptedSocket->acceptedSuccessfully) {
        acceptedSocket->error = clientSocketFD;
    }

    return acceptedSocket;
}
