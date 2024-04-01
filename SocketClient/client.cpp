#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <pthread.h>
#include <algorithm>
#include <sstream>
#include <vector>
#include <errno.h> 
#include <netdb.h>  
#include <arpa/inet.h> 
#include "chat.pb.h"

// Colores a usar en prints
# define DEFAULT "\033[0m"
# define MAGENTA "\033[35m"      
# define RED     "\033[31m"    
# define BLUE    "\033[34m"      
# define GREEN   "\033[32m"      
# define YELLOW  "\033[33m"      
# define BOLDBLACK   "\033[1m\033[30m"          
 
using namespace std;
using namespace chat;

#define MAX_BUFFER 8192 // Buffer maximo para los mensajes

pthread_t listen_client;
pthread_t options_client;
string lastUser;
string lastMessage;

// Opciones de solicitudes que el cliente puede hacer al servidor
enum ClientOpt {
    SYNC = 1,
    CONNECTED_USERS = 2,
    STATUS = 3,
    BROADCAST_C =4,
    DM = 5,
    ACKNOWLEDGE = 6
};

// Opciones de respuestas del servidor al cliente
enum ServerOpt {
    BROADCAST_S = 1,
    MESSAGE = 2,
    ERROR = 3,
    RESPONSE = 4,
    C_USERS_RESPONSE = 5,
    CHANGE_STATUS = 6,
    BROADCAST_RESPONSE = 7,
    DM_RESPONSE = 8
};


/*
 * Metodo para mandar un mensaje de error al cliente y salir del programa
 * parametros: char *msg - message to show
*/
void error(const char *msg)
{
    perror(msg);
    exit(0);
}
/*
 * Thread para escuchar las respuestas del server, mientras que el socket este
 * abierto, el cliente escuchara cada respuesta y mostrara al cliente informacion
 * especifica dependiendo de la opcion del servidor
 * 
*/
void *listen_thread(void *params){
    int socketFd = *(int *)params;
    char buffer[MAX_BUFFER];
    string msgSerialized;
    ClientPetition clientMessage;
    ClientPetition clientAcknowledge;
    ServerResponse serverMessage;

    cout << "Thread for hearing responses of server created" << endl;

    loop: while (1)
    {
        int bytes_received = recv(socketFd, buffer, MAX_BUFFER, 0);
        // reception and parse for server messages
        if (bytes_received > 0){
            serverMessage.ParseFromString(buffer);
            
            if(serverMessage.option() == ServerOpt::BROADCAST_S){
                if(serverMessage.broadcast().has_username()){
                    cout << "De : " << BLUE << serverMessage.broadcast().username() << DEFAULT << GREEN << ". Para: Todos" << DEFAULT << endl;
                    cout << "\t" << serverMessage.broadcast().message().c_str() << endl;
                } else if(serverMessage.broadcast().has_userid()){
                    cout << "De : " << BLUE << serverMessage.broadcast().userid() << DEFAULT << GREEN << ". Para: Todos" << DEFAULT << endl;
                    cout << "\t" << serverMessage.broadcast().message().c_str() << endl;
                } else {
                    cout << RED << "No username or userid sent by server" << DEFAULT << endl;
                }

            }
            else if (serverMessage.option() == ServerOpt::BROADCAST_RESPONSE)
            {
                cout << BOLDBLACK << "Server response: " << serverMessage.broadcastresponse().messagestatus() << DEFAULT << endl;
                cout << "De mi, para " <<  DEFAULT << YELLOW << " Todos: " << DEFAULT << endl;
                cout << "\t" << lastMessage << endl;
               
            } else if (serverMessage.option() == ServerOpt::CHANGE_STATUS)
            {
                cout << BOLDBLACK << "Server change status correctly" << DEFAULT << endl;
    
            } 
            else if(serverMessage.option() == ServerOpt::DM_RESPONSE){
                if(serverMessage.directmessageresponse().messagestatus() == "ENVIADO" || serverMessage.directmessageresponse().has_messagestatus()){
                    cout << GREEN << "Mensaje enviado exitosamente!" << DEFAULT << endl;
                    cout << "De mi, para " << BLUE << lastUser << DEFAULT << YELLOW << " (Privado): " << DEFAULT << endl;
                    cout << "\t" << lastMessage << endl;
                }else{
                    cout << RED << "No se pudo enviar el mensaje." << endl;
                }
            } else if (serverMessage.option() == ServerOpt::MESSAGE)
            {
                if(serverMessage.message().has_username()){
                    cout << "De: " << BLUE << 
                    serverMessage.message().username() << DEFAULT << YELLOW << " (Privado): " << DEFAULT << endl;
                    cout << "\t" << serverMessage.message().message().c_str() << endl;
                } else {
                    cout << "De: " << BLUE << serverMessage.message().userid() << DEFAULT << YELLOW << " (Privado):" << DEFAULT << endl;
                    cout << "\t" << serverMessage.message().message().c_str() << endl;
                }
            } else if (serverMessage.option() == ServerOpt::C_USERS_RESPONSE)
            {
                
                if((serverMessage.connecteduserresponse().connectedusers_size()) != 0){
                    cout << BOLDBLACK << "Los usuarios conectados al chat son: " << DEFAULT << endl;
                    cout << GREEN <<"Conectados: " << DEFAULT << serverMessage.connecteduserresponse().connectedusers_size() << endl;
                    for (int i = 0; i < serverMessage.connecteduserresponse().connectedusers_size(); i++) {
                        ConnectedUser tmpUser = serverMessage.connecteduserresponse().connectedusers(i);
                        cout << BLUE << "USERNAME: " << DEFAULT << tmpUser.username() << endl;
                        if(tmpUser.has_userid())
                             cout << BLUE <<"ID: " << DEFAULT << tmpUser.userid() << endl;
                        if(tmpUser.has_status())    
                            cout << BLUE<< "STATUS: " << DEFAULT << tmpUser.status() << endl;
                        if(tmpUser.has_ip())    
                            cout << BLUE << "IP: " << DEFAULT << tmpUser.ip() << endl;
                        cout << "----------------------------------" << endl;
                    }
          
                } else {
                    cout << RED << "No hay usuarios conectados al chat." << DEFAULT << endl;
                }

            } else if (serverMessage.option() == ServerOpt::ERROR && serverMessage.has_error())
            {
                cout << RED << "Server response with an error: " << serverMessage.error().errormessage() << DEFAULT << endl;
                if(serverMessage.error().errormessage() == "Failed to Synchronize" || serverMessage.error().errormessage() == "Failed to Acknowledge" )
                    exit(0);
                else
                    goto loop;
            }
            serverMessage.Clear();
        }
        else {
            pthread_cancel(options_client); //request para que el otro thread termine
            std::cout << "Saliendo del cliente." << endl;
            // close(socketFd);
            exit(0);
            pthread_exit(0);
        }
    }
    pthread_exit(0);
}
/*
 * Metodo para separar la entrada del cliente en dos
 * params: input - entrada entera
 * return: primera palabra antes de un espacio en blanco
*/
string getFirst(string input){
    istringstream inputStream(input);
    string username;
    inputStream >> username;
    return username;
}
/*
 * Metodo para separar la entrada del cliente en dos
 * params: input - entrada entera del cliente, toErease - primera palabra antes de un espacio en blanco
 * return: segunda palabra, primera palabra despues de un espacio en blanco
*/
string getMessage(string input, string toErase) {
    size_t pos = input.find(toErase);

    if (pos != string::npos) 
        input.erase (pos, toErase.length() + 1);

    return input;
}

/*
 * metodo para mandar la solicitud al servidor
 * params: message - mensaje serializado, int - socket
*/
void sendBySocket(string message, int sockfd){
    char cstr[message.size() + 1];
    strcpy(cstr, message.c_str());
    // send to socket
    send(sockfd, cstr, strlen(cstr), 0 );
}
/*
 * Metodo para mandar una solicitud de broadcast al servidor
 * una variable ClientPetition se llena con con la opcion BROADCAST REQUEST
 * el BroadcastRequest se llena con el mensaje que el usuario quiere enviar
 * finalmente se envia por el socket al servidor
 * params: int - socket, string - message user wants to send
*/
void broadCast(char buffer[], int sockfd, string message){
    string binary;
    ClientPetition clientMessage;
    ServerResponse serverResponseMsg;
    BroadcastRequest *brdRequest = new BroadcastRequest();
    clientMessage.set_option(ClientOpt::BROADCAST_C);
    brdRequest->set_message(message);
    clientMessage.set_allocated_broadcast(brdRequest);
    clientMessage.SerializeToString(&binary);
    sendBySocket(binary, sockfd);
    cout << BOLDBLACK << "Sending broadcast request to server" << DEFAULT << endl;
    cout << endl;
}
/*
 * Metodo para mandar una solicitud de mensaje directo al servidor 
 * una variable ClientPetition se llena con la opcion DIRECT MESSAGE REQUEST,
 * tambien con el id del usuario de quien esta enviando el mensaje en este caso
 * el user id es el mismo que el socket
 * el DirectMessageRequest se llena con el mensaje que el usuario quiere enviar,
 * y el el username del destinatario a quien se quiere que el server le envie el mensaje
 * finalmente, se envia por el socket al servidor
 * params: int - socket, string - message user wants to send, recipient_username - username of destinatiry
*/
void directMS(int sockfd, string message, string recipient_username, int id){
    string binary;
    ClientPetition clientMessage;
    DirectMessageRequest *directMsRequest = new DirectMessageRequest();
    directMsRequest->set_message(message);
    if (id != 0) {
        directMsRequest->set_userid(id);
    } else {
        directMsRequest->set_username(recipient_username);
    }

    clientMessage.set_option(ClientOpt::DM);
    clientMessage.set_userid(sockfd);
    clientMessage.set_allocated_directmessage(directMsRequest);
    clientMessage.SerializeToString(&binary);
    sendBySocket(binary, sockfd);
    cout << BOLDBLACK << "Enviado DM a:" << DEFAULT << BLUE << recipient_username.c_str() << DEFAULT << endl;
    cout << endl;
}
/*
 * Metodo para enviar una solicitud de cambio de estatus al servidor
 * una variable ClientPetition se llena con la opcion CHANGE STATUS REQUEST
 * el ChangeStatusRequest se llena con el nuevo estatus del cliente
 * params: status - newStatus, socket - socket int
*/
void changeStatus(string status, int sockfd){
    string binary;
    ClientPetition clientMessage;
    ServerResponse serverResponseMsg;
    ChangeStatusRequest *statusRequest = new ChangeStatusRequest();
    clientMessage.set_option(ClientOpt::STATUS);
    statusRequest->set_status(status);
    clientMessage.set_allocated_changestatus(statusRequest);
    clientMessage.SerializeToString(&binary);
    sendBySocket(binary, sockfd);
    cout << BOLDBLACK << "Sending change status request to server" << DEFAULT << endl;
    cout << endl;
}

/*
 * Metodo para enviar una solicitud de ver todos los usuarios conectados al servidor
 * una variable ClientPetition se llena con la opcion CONNECTED USER
 * el connectedUserRequest se llena con el id 0 que significa que 
 * la opcion sera la de ver todos los nombres de los usuarios conectados al servidor
 * params: int socket
*/
void connectedUsers(char buffer[], int socketfd){
    string binary;
    ClientPetition ClientPetition;
    connectedUserRequest *usersRequest = new connectedUserRequest();
    ClientPetition.set_option(ClientOpt::CONNECTED_USERS);
    usersRequest->set_userid(0);
    ClientPetition.set_allocated_connectedusers(usersRequest);
    ClientPetition.SerializeToString(&binary);
    sendBySocket(binary, socketfd);
    cout << BOLDBLACK << "Sending connected user Request to server" << DEFAULT << endl;
    cout << endl;
}
/*
 * Metodo para enviar una solicitud de ver info de usuario al servidor
 * una variable ClientPetition se llena con la opcion CONNECTED USER
 * el connectedUserRequest se llena con el id del usuario que se desea ver la info
 * params: int socketfd, username - nombre del usuario seleccionado
*/
void requestUserIfo(int socketfd, string username){
    string binary;
    ClientPetition ClientPetition;
    connectedUserRequest *usersRequest = new connectedUserRequest();
    ClientPetition.set_option(ClientOpt::CONNECTED_USERS);
    usersRequest->set_username(username);
    ClientPetition.set_allocated_connectedusers(usersRequest);
    ClientPetition.SerializeToString(&binary);
    sendBySocket(binary, socketfd);
    cout << BOLDBLACK << "Sending connected user Request to server" << DEFAULT << endl;
    cout << endl;
}
/*
 * Thread para mandar solicitudes al servidor mientras el socket este abierto
 * la forma que el usuario selecciona una opcion es al escribirla como primera palabra
 * antes de su entrada completa
 * 
*/
void *options_thread(void *args)
{
    string input;
    char buffer[MAX_BUFFER];
    int socketFd = *(int *)args;
    int status;
    string newStatus;
    string message, directMessage, recipient_username;
    int idDestinatary;
    printf("Thread for sending requests to server created\n");
    std::cout << R"(

* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * **
     ---Info--                                                           *
     1. Mandar un mensaje en el chat general: 'everyone <yourmessage>'   *
     2. Cambiar estado: 'status <newstatus>'                             *
     3. Ver usuarios conectados 'users'                                  *
     4. Ver informacion de un usuario: '<username>'                      *
     5. Para mandar un mensaje privado: '<username> <yourmessage>'       *
     6. Ver todos los comandos: 'help'                                   *
     7. Salir: 'exit'                                                    *
                                                                         *
* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * **
)" << '\n';
    while (1)
    {

        getline(cin, input);
        string action = getFirst(input);
        string message = getMessage(input, action);
        if (action == "everyone"){ // Mandar un mensaje a todos, chat general
            lastMessage = message;
            broadCast(buffer, socketFd, message);
            sleep(3);
        } else if (action == "status"){ // Cambiar usuario en el cliente
            changeStatus(message, socketFd);
            sleep(3);
        } else if (action == "exit"){ // salir
            memset(&buffer[0], 0, sizeof(buffer)); //clear buffer
            send(socketFd, NULL, 0, 0); //send the message to notify the server
            pthread_cancel(listen_client); //request to ask the listen thread to finish 
               std::cout << R"(Gracias por usar el chat!
   
                         
)"              << '\n';
            close(socketFd);
            pthread_exit(0);
            sleep(2);
        } else if (action == "users"){ //Solicitud para ver todos los usuarios conectados al servidor
            connectedUsers(buffer, socketFd);
            sleep(5);
        } else if (action == "id"){
            string userId = getFirst(message);
            int id = stoi(userId);
            string newMessage = getMessage(message, userId);
            lastUser = userId;
            lastMessage = newMessage;
            directMS(socketFd, newMessage, "", id);

        } else if (action == "help" || action == ""){ // Menu de ayuda o comandos
            std::cout << R"(
* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * **
     ---Info--                                                           *
     1. Mandar un mensaje en el chat general: 'everyone <yourmessage>'   *
     2. Cambiar estado: 'status <newstatus>'                             *
     3. Ver usuarios conectados 'users'                                  *
     4. Ver informacion de un usuario: '<username>'                      *
     5. Para mandar un mensaje privado: '<username> <yourmessage>'       *
     6. Ver todos los comandos: 'help'                                   *
     7. Salir: 'exit'                                                    *
                                                                         *
* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * **
)"          << '\n';
        } else {
            if(message == ""){
                requestUserIfo(socketFd, action);
            } else {
                lastUser = action;
                lastMessage = message;
                directMS(socketFd, message, action, 0);
                sleep(3);
            }
        }
        
    }
    
}


/*
 * Metodo para sincronizar el usuario con el servidor
 * el nombre de usuario, ip y el socket se le mandan al servidor
*/
void synchUser(struct sockaddr_in serv_addr, int sockfd, char buffer[], string ip, char *argv[]){
    int n;
    MyInfoSynchronize *clientInfo = new MyInfoSynchronize();
    clientInfo->set_username(argv[1]);
    clientInfo->set_ip(ip);
    ClientPetition clientMessage;
    clientMessage.set_option(ClientOpt::SYNC);
    clientMessage.set_userid(sockfd);
    clientMessage.set_allocated_synchronize(clientInfo);
    //Message is serialize
    string binary;
    clientMessage.SerializeToString(&binary);
    char cstr[binary.size() + 1];
    strcpy(cstr, binary.c_str());
    // send to socket
    send(sockfd, cstr, strlen(cstr), 0 );
    // listen for server response
    bzero(buffer,MAX_BUFFER);
    n = read(sockfd, buffer, 255);
    if (n < 0) 
         error("ERROR reading from socket");

    //read server response
    ServerResponse serverResponseMsg;
    serverResponseMsg.ParseFromString(buffer);
    if(serverResponseMsg.option() == ServerOpt::ERROR){
        cout << RED << "Failed to establish connection to server. Exiting." << DEFAULT << endl;
        exit(0);
    }
    cout << "Server response: " << endl;
    cout << "Option: " << serverResponseMsg.option() << endl;
    cout << "User Id: " << serverResponseMsg.myinforesponse().userid() << endl;
    
    // client response (acknowledge)
    MyInfoAcknowledge * infoAck(new MyInfoAcknowledge);
    infoAck->set_userid(sockfd);
    ClientPetition clientAcknowledge; 
    clientAcknowledge.set_option(ClientOpt::ACKNOWLEDGE);
    clientAcknowledge.set_userid(sockfd); 
    clientAcknowledge.set_allocated_acknowledge(infoAck);
    string binarya;
    clientAcknowledge.SerializeToString(&binarya);
    char cstr2[binarya.size() + 1];
    strcpy(cstr2, binarya.c_str());
    send(sockfd, cstr2, strlen(cstr2), 0);
}

int main(int argc, char *argv[])
{
    GOOGLE_PROTOBUF_VERIFY_VERSION;
    int sockfd, portno, n;
    struct sockaddr_in serv_addr;
    struct hostent *server;
    int option;
    char buffer[MAX_BUFFER];
    
    if (argc != 4) {
       fprintf(stderr, "./client [username] [host] [port]\n");
       exit(1);
    }

    portno = atoi(argv[3]);
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) 
        error("ERROR opening socket");
    server = gethostbyname(argv[2]);
    if (server == NULL) {
        fprintf(stderr,"ERROR, no such host\n");
        exit(0);
    }
    bzero((char *) &serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    bcopy((char *)server->h_addr, 
         (char *)&serv_addr.sin_addr.s_addr,
         server->h_length);
    serv_addr.sin_port = htons(portno);
    if (connect(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) 
        error("ERROR connecting");
    string ip(inet_ntoa(*((struct in_addr*) server->h_addr_list[0])));
    synchUser(serv_addr, sockfd, buffer, ip, argv);
    if (pthread_create(&listen_client, NULL, listen_thread, (void *)&sockfd) || pthread_create(&options_client, NULL, options_thread, (void *)&sockfd))
    {
        cout << RED << "Error: unable to create threads." << DEFAULT << endl;
        exit(-1);
    }
    
    pthread_join (listen_client, NULL);
    pthread_join (options_client, NULL);
    return 0;
}
