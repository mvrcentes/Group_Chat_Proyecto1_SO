#include "socketutil.h"


int createTCPIpv4Socket() { return socket(AF_INET, SOCK_STREAM, 0); }

struct sockaddr_in* createIPv4Address(char *ip, int port) {
    // domain: AF_INET, type: SOCK_STREAM, protocol: 0
    // AF_INET: IPv4 Internet protocols
    // SOCK_STREAM: Provides sequenced, reliable, two-way, connection-based byte streams
    //protocol 0: default protocol for the given domain and type

    // ----------------- constructing the address to some server -----------------
    // mxtoolbox.com
    // char* ip = "142.251.111.27";
    // char* ip = "142.250.188.46";

    // no va sobre escribir la data
    struct sockaddr_in *address = malloc(sizeof(struct sockaddr_in));
    address->sin_family = AF_INET;
    address->sin_port = htons(port); // convert to network byte order

    if(strlen(ip) == 0)
        address->sin_addr.s_addr = INADDR_ANY; //cualquier conexión entrante
    else
        // convert
        inet_pton(AF_INET, ip, &address->sin_addr.s_addr);
    // ----------------- constructing the address to some server -----------------

    return address;
}
