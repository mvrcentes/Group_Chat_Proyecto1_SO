#include <stdbool.h>
#include <unistd.h>
#include "socketutil.h"

int main() {

    int serverSockerFD = createTCPIpv4Socket();
    // conexiones entrantes
    struct sockaddr_in *serverAddress = createIPv4Address("", 2000);

    int result = bind(serverSockerFD, serverAddress, sizeof(*serverAddress));
    if (result == 0)
        printf("Link correcto con el socket\n");


    // Cuantos se van a conectar
    int listenResult = listen(serverSockerFD, 10);


    struct sockaddr_in clientAddress;
    int clientAddressSize = sizeof (struct sockaddr_in);
    int clientSocketFD = accept(serverSockerFD, &clientAddress, &clientAddressSize);

    char buffer[1024];
    while (true) {
        ssize_t ammountReceived = recv(clientSocketFD, buffer, 1024, 0);

        if (ammountReceived > 0) {
            buffer[ammountReceived] = 0;
            printf("Respuesta: %s\n", buffer);
        }

        if (ammountReceived == 0)
            break;
    }

    close(clientSocketFD);
    shutdown(serverSockerFD, SHUT_RDWR);

    return 0;
}
