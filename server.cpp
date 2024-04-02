#include <chrono>
#include <vector>
#include <cstring>
#include <iostream>
#include <unistd.h>
#include <pthread.h>
#include <netinet/in.h>
#include <sys/socket.h>

// ProtoBuff
#include "sistos.pb.h"


#define MAX_CLIENT_BUFFER 2048
using namespace std;


struct Client {
    int socket_fd;
    string username;
    string ip;
    string status;
};

struct ChatroomsData {
    int socket_fd;
    vector<Client> clients;
    fd_set read_fds;
    pthread_mutex_t *client_list_mutex;

    ChatroomsData(int sfd, pthread_mutex_t *clm) {
        socket_fd = sfd;
        client_list_mutex = clm;
    }
};

struct CurrentClientData {
    int socket_fd;
    ChatroomsData *chatrooms_data;
    chrono::steady_clock sc;

    CurrentClientData(int csfd, ChatroomsData *data) {
        socket_fd = csfd;
        chatrooms_data = data;
    }
};

int timeout_seconds = 120;


void bind_socket(struct sockaddr_in *server_address, int socket_fd, long port);
void *new_clients_handler(void *data);
void *client_listener(void *client_data);
void disconnect_client(ChatroomsData *chatrooms_data, int current_client_socket_fd);


int main(int argc, char *argv[]) {
    GOOGLE_PROTOBUF_VERIFY_VERSION;

    struct sockaddr_in server_address;
    int socket_fd;
    long port;

    socket_fd = socket(AF_INET, SOCK_STREAM, 0);
    port = strtol(argv[1], NULL, 0);

    if(socket_fd == -1) {
        cout << "\nNo se pudo crear el socket" << endl;
        exit(1);
    }

    bind_socket(&server_address, socket_fd, port);

    if(listen(socket_fd, 1) == -1) {
        cout << "\nUnion fallida" << endl;
        exit(1);
    }

    ChatroomsData data(socket_fd, (pthread_mutex_t *) malloc(sizeof(pthread_mutex_t)));
    pthread_mutex_init(data.client_list_mutex, NULL);

    // Thread que maneja las nuevas conexiones
    pthread_t connectionThread;

    if((pthread_create(&connectionThread, NULL, new_clients_handler, (void *)&data)) == 0) {
        cout << "\nServer listo" << endl;
    }

    FD_ZERO(&(data.read_fds));
    FD_SET(socket_fd, &(data.read_fds));

    pthread_join(connectionThread, NULL);

    pthread_mutex_destroy(data.client_list_mutex);
    free(data.client_list_mutex);

    close(socket_fd);
    pthread_exit(NULL);
}

// Enlazar el socket
void bind_socket(struct sockaddr_in *server_address, int socket_fd, long port) {
    memset(server_address, 0, sizeof(*server_address));

    server_address -> sin_family = AF_INET;
    server_address -> sin_addr.s_addr = htonl(INADDR_ANY);
    server_address -> sin_port = htons(port);

    if(::bind(socket_fd, (struct sockaddr *)server_address, sizeof(struct sockaddr_in)) == -1) {
        cout <<  "\nEnlazado de los sockets fallido" << endl;
        exit(1);
    }
}

// Agrega el client_fd a la lista y crea un hilo de manejador de cliente
void *new_clients_handler(void *data) {
    ChatroomsData *chatrooms_data = (ChatroomsData *) data;

    while(true) {
        int new_client_socket_fd = accept(chatrooms_data -> socket_fd, NULL, NULL);

        if(new_client_socket_fd > 0) {
            cout << "\nSe ha conectado un nuevo cliente: " << new_client_socket_fd << endl;

            // Obtener lock en la lista de clientes
            pthread_mutex_lock(chatrooms_data -> client_list_mutex);

            bool is_defined_already = false;

            // Verificar que no exista el file descriptor en la lista de file descriptors leidos
            for(int i = 0; i < chatrooms_data -> clients.size(); i++) {
                if(FD_ISSET(chatrooms_data -> clients[i].socket_fd, &(chatrooms_data -> read_fds)) == 0) {
                    is_defined_already = true;
                    break;
                }
            }

            // Si no estaba definido se agrega
            if (!is_defined_already) {
                Client new_client;
                new_client.socket_fd = new_client_socket_fd;
                chatrooms_data -> clients.push_back(new_client);
            }

            // Agregar un nuevo socket fd a la lista de fds leidos
            FD_SET(new_client_socket_fd, &(chatrooms_data -> read_fds));

            // Creacion de un hilo para las peticiones del cliente
            CurrentClientData client_data(new_client_socket_fd, chatrooms_data);
            pthread_t client_thread;

            if((pthread_create(&client_thread, NULL, client_listener, (void *)&client_data)) == 0) {
                cout << "\nEscuchando a cliente por socket: " << new_client_socket_fd << endl;
            } else {
                close(new_client_socket_fd);
            }

            pthread_mutex_unlock(chatrooms_data -> client_list_mutex);
        }
    }
    pthread_exit(NULL);
}

void *client_listener(void *client_data) {
    CurrentClientData *current_client_data = (CurrentClientData *) client_data;
    ChatroomsData *chatrooms_data = (ChatroomsData *) current_client_data -> chatrooms_data;
    int current_client_socket_fd = current_client_data -> socket_fd;

    while(true) {
        char client_buffer[MAX_CLIENT_BUFFER];
        char server_buffer[MAX_CLIENT_BUFFER];
        chat::ClientPetition client_petition;
        chat::ServerResponse server_response;
        string petition;
        string response;
        auto start = current_client_data -> sc.now();
        int len_read = read(current_client_socket_fd, &client_buffer, MAX_CLIENT_BUFFER - 1);
        client_buffer[len_read] = '\0';
        auto end = current_client_data -> sc.now();

        petition = (string)client_buffer;
        client_petition.ParseFromString(petition);

        if (client_petition.option() == 0) {
            continue;
        }

        // Verificar inactividad
        auto time_span = static_cast<chrono::duration<double>>(end - start);
        if (timeout_seconds < time_span.count()) {
            for (int i = 0; i < chatrooms_data -> clients.size(); i++) {
                Client client_i = chatrooms_data -> clients[i];
                if (client_i.socket_fd == current_client_socket_fd) {
                    client_i.status = "Inactivo";
                    chatrooms_data -> clients[i] = client_i;
                }
            }
        }

        if(client_petition.option() == 7) {
            cout << "\nEl cliente se ha desconectado, socket: " << current_client_socket_fd << endl;
            disconnect_client(chatrooms_data, current_client_socket_fd);
            close(current_client_socket_fd);
            return NULL;
        } else {

            server_response.set_option(client_petition.option());

            switch (client_petition.option()) {
                case 1: {
                    // ==================================
                    // Registro de usuario
                    // ==================================
                    bool user_already_exists = false;
                    vector<Client>::iterator it = chatrooms_data -> clients.begin();

                    while (it != chatrooms_data -> clients.end()) {
                        if ((*it).username == client_petition.mutable_registration() -> username()) {
                            server_response.set_code(500);
                            server_response.set_servermessage("Ese username ya esta registrado");
                            user_already_exists = true;
                            break;
                        }

                        if ((*it).ip == client_petition.mutable_registration() -> ip()) {
                            server_response.set_code(500);
                            server_response.set_servermessage("Esa IP ya esta registrado");
                            user_already_exists = true;
                            break;
                        }

                        ++it;
                    }

                    if (user_already_exists) {
                        server_response.set_option(7);
                        server_response.set_servermessage("El usuario ya existe");
                        server_response.SerializeToString(&response);
                        strcpy(server_buffer, response.c_str());
                        write(current_client_socket_fd, server_buffer, MAX_CLIENT_BUFFER - 1);

                        cout << "\nEl cliente ha sido desconectado, socket: " << current_client_socket_fd << endl;
                        disconnect_client(chatrooms_data, current_client_socket_fd);
                        close(current_client_socket_fd);
                        return NULL;
                    } else {
                        for (int i = 0; i < chatrooms_data -> clients.size(); i++) {
                            Client client_i = chatrooms_data -> clients[i];
                            if (client_i.socket_fd == current_client_socket_fd) {
                                client_i.username = client_petition.mutable_registration() -> username();
                                client_i.ip = client_petition.mutable_registration() -> ip();
                                client_i.status = "Activo";

                                // Registration completed
                                chatrooms_data -> clients[i] = client_i;
                                server_response.set_code(200);
                            }
                        }
                    }
                    break;
                }
                case 2: {
                    // ==================================
                    // Lista de usuarios conectados
                    // ==================================
                    chat::ConnectedUsersResponse* connected_users = server_response.mutable_connectedusers();
                    for (int i = 0; i < chatrooms_data -> clients.size(); ++i) {
                        Client client_i = chatrooms_data -> clients[i];
                        chat::UserInfo* user_info = connected_users -> add_connectedusers();
                        user_info -> set_username(client_i.username);
                        user_info -> set_ip(client_i.ip);
                        user_info -> set_status(client_i.status);
                    }

                    server_response.set_code(200);
                    break;
                }
                case 3: {
                    // ==================================
                    // Cambiar estado
                    // ==================================
                    for (int i = 0; i < chatrooms_data -> clients.size(); ++i) {
                        Client client_i = chatrooms_data -> clients[i];
                        if (client_i.username == client_petition.mutable_change() -> username()) {
                            client_i.status = client_petition.mutable_change() -> status();
                            chatrooms_data -> clients[i] = client_i;
                        }
                    }
                    server_response.set_code(200);
                    break;
                }
                case 4: {
                    // ==================================
                    // Chat
                    // ==================================
                    char server_chat_buffer[MAX_CLIENT_BUFFER];
                    chat::ServerResponse server_chat_response;
                    string response_chat;
                    server_chat_response.set_option(4);

                    string sender = client_petition.mutable_messagecommunication() -> sender();
                    string receiver = client_petition.mutable_messagecommunication() -> recipient();
                    string message = client_petition.mutable_messagecommunication() -> message();

                    if (client_petition.mutable_messagecommunication() -> recipient() == "everyone") {
                        for (int i = 0; i < chatrooms_data -> clients.size(); ++i) {
                            Client client_i = chatrooms_data -> clients[i];

                            server_chat_response.mutable_messagecommunication() -> set_sender(sender);
                            server_chat_response.mutable_messagecommunication() -> set_recipient(receiver);
                            server_chat_response.mutable_messagecommunication() -> set_message(message);

                            server_chat_response.set_code(200);
                            server_chat_response.SerializeToString(&response_chat);
                            strcpy(server_chat_buffer, response_chat.c_str());
                            write(client_i.socket_fd, server_chat_buffer, MAX_CLIENT_BUFFER - 1);
                        }
                    } else {
                        for (int i = 0; i < chatrooms_data -> clients.size(); ++i) {
                            Client client_i = chatrooms_data -> clients[i];
                            if (client_i.username == client_petition.mutable_messagecommunication() -> recipient()) {
                                server_chat_response.mutable_messagecommunication() -> set_sender(sender);
                                server_chat_response.mutable_messagecommunication() -> set_recipient(receiver);
                                server_chat_response.mutable_messagecommunication() -> set_message(message);

                                server_chat_response.set_code(200);
                                server_chat_response.SerializeToString(&response_chat);
                                strcpy(server_chat_buffer, response_chat.c_str());
                                write(client_i.socket_fd, server_chat_buffer, MAX_CLIENT_BUFFER - 1);
                            }
                        }
                    }

                    server_response.set_code(200);
                    break;
                }
                case 5: {
                    // ==================================
                    // Info de usuario
                    // ==================================
                    if (client_petition.mutable_users() -> user() == "everyone") {
                        chat::ConnectedUsersResponse* connected_users = server_response.mutable_connectedusers();
                        for (int i = 0; i < chatrooms_data -> clients.size(); ++i) {
                            Client client_i = chatrooms_data -> clients[i];
                            chat::UserInfo* user_info = connected_users -> add_connectedusers();
                            user_info -> set_username(client_i.username);
                            user_info -> set_ip(client_i.ip);
                            user_info -> set_status(client_i.status);
                        }
                    } else {
                        chat::UserInfo* user_info = server_response.mutable_userinforesponse();
                        for (int i = 0; i < chatrooms_data -> clients.size(); ++i) {
                            Client client_i = chatrooms_data -> clients[i];
                            if (client_i.username == client_petition.mutable_users() -> user()) {
                                user_info -> set_username(client_i.username);
                                user_info -> set_ip(client_i.ip);
                                user_info -> set_status(client_i.status);
                            }
                        }
                    }

                    server_response.set_code(200);
                    break;
                }
                case 6: {
                    // ==================================
                    // Ayuda
                    // ==================================
                    break;
                }
                default:
                    break;
            }

            if (client_petition.option() != 0) {
                server_response.SerializeToString(&response);
                strcpy(server_buffer, response.c_str());
                write(current_client_socket_fd, server_buffer, MAX_CLIENT_BUFFER - 1);
            }
        }
    }
    pthread_exit(NULL);
}

// Removes the socket from the list of active client sockets and closes it
void disconnect_client(ChatroomsData *chatrooms_data, int current_client_socket_fd) {
    pthread_mutex_lock(chatrooms_data -> client_list_mutex);

    vector<Client>::iterator it = chatrooms_data -> clients.begin();

    while (it != chatrooms_data -> clients.end()) {
        if ((*it).socket_fd == current_client_socket_fd) {
            it = chatrooms_data -> clients.erase(it);
            close(current_client_socket_fd);
        } else {
            ++it;
        }
    }

    pthread_mutex_unlock(chatrooms_data -> client_list_mutex);
}
