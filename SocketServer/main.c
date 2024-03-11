#include "socketutil.h"

int main() {

    int serverSockerFD = createTCPIpv4Socket();
    // conexiones entrantes
    struct sockaddr_in *serverAddress = createIPv4Address("", 2000);

    int result = bind(serverSockerFD, serverAddress, sizeof(*serverAddress));

    if (result == 0)
        printf("socket was bound\n");

    // Cuantos se van a conectar
    int listenResult = listen(serverSockerFD, 10);

    struct sockaddr_in clientAddress ;
    int clientAddressSize = sizeof (struct sockaddr_in);
    int clientSocketFD = accept(serverSockerFD, &clientAddress, &clientAddressSize);



    return 0;
}
