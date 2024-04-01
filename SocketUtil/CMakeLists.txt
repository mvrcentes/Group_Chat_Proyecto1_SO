cmake_minimum_required(VERSION 3.27)
project(SocketUtil C)

# Establece el estándar de C en 11
set(CMAKE_C_STANDARD 11)

# Incluye el directorio actual
include_directories(.)

# Define la ruta del directorio de instalación de ProtobufC
list(APPEND CMAKE_PREFIX_PATH "/Users/mvrcentes/Downloads/protobuf-3.15.0")

# Encuentra y configura la biblioteca protobuf-c
find_package(ProtobufC REQUIRED)

# Incluye los directorios de inclusión de protobuf-c
include_directories(${ProtobufC_INCLUDE_DIRS})

# Agrega la biblioteca protobuf-c
target_link_libraries(SocketUtil ${ProtobufC_LIBRARIES})


# Lista de archivos fuente para tu biblioteca
set(SOURCE_FILES
#        socketutil.c
#        socketutil.h
        chat.pb.cc
        chat.pb.h
)

# Agrega la biblioteca
add_library(SocketUtil ${SOURCE_FILES})

# Enlaza la biblioteca de Protocol Buffers en C
target_link_libraries(SocketUtil ${ProtobufC_LIBRARIES})
