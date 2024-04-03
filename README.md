# SisChat

**SisChat** es un sistema de mensajería que permite a los usuarios comunicarse a través de un servidor. Con opciones tanto para conexión local como en la nube, SisChat ofrece una plataforma flexible para chatear en tiempo real.

## 🌐 Servidor

### IP

- **Localhost**: `127.0.0.1`
- **Cloudhost**: `20.55.64.147`

### Puerto

- **PORT**: `8000`

## 💻 Compilación y Ejecución

### Servidor

Compila y ejecuta el servidor SisChat siguiendo estos pasos:

#### Requisitos 

tener protoc versión 3.15 

1. **Compilación del Servidor**

    Utiliza el siguiente comando para compilar el servidor:

    ```bash
    clang++ -std=c++11 -stdlib=libc++ -o server server.cpp sistos.pb.cc -pthread -I/usr/local/include -L/usr/local/lib -lprotobuf -lpthread
    ```

2. **Ejecución del Servidor**

    Inicia el servidor especificando el puerto de escucha (`8000` en este ejemplo):

    ```bash
    ./server 8000
    ```

### Cliente

Conecta con el servidor SisChat como cliente:

1. **Compilación del Cliente**

    Compila el cliente con el siguiente comando:

    ```bash
    clang++ -std=c++11 -stdlib=libc++ -o client client.cpp sistos.pb.cc -pthread -I/usr/local/include -L/usr/local/lib -lprotobuf -lpthread
    ```

2. **Ejecución del Cliente**

    Ejecuta el cliente proporcionando tu nombre de usuario, la IP del servidor y el puerto:

    ```bash
    ./client <username> <servidor> <PORT>
    ```

    Por ejemplo:

    ```bash
    ./client Alice 127.0.0.1 8000
    ```
