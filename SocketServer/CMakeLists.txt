cmake_minimum_required(VERSION 3.23)
project(SocketServer C)

set(CMAKE_C_STANDARD 99)

set(SOCKET_UTIL_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../SocketUtil")

# Mostrar la ruta del directorio de SocketUtil
message(STATUS "La ruta del directorio de SocketUtil es: ${SOCKET_UTIL_DIR}")


# Asegurarse de que la ruta de SocketUtil exista
if(NOT EXISTS ${SOCKET_UTIL_DIR})
    message(FATAL_ERROR "La ruta de SocketUtil no es válida: ${SOCKET_UTIL_DIR}")
endif()

# Añadir la ruta de SocketUtil a los directorios de inclusión
include_directories(${SOCKET_UTIL_DIR})

# Buscar la biblioteca SocketUtil
find_library(TheLibrary SocketUtil PATHS ${SOCKET_UTIL_DIR}/cmake-build-debug)

# Verificar si se encontró la biblioteca
if(NOT TheLibrary)
    message(FATAL_ERROR "No se encontró la biblioteca SocketUtil en la ruta: ${SOCKET_UTIL_DIR}/cmake-build-debug")
endif()

# Establecer la ruta del archivo de encabezado
set(TheHeaderFile ${SOCKET_UTIL_DIR}/socketutil.h)

# Verificar si se encontró el archivo de encabezado
if(NOT EXISTS ${TheHeaderFile})
    message(FATAL_ERROR "El archivo socketutil.h no se encuentra en la ruta especificada: ${TheHeaderFile}")
endif()

# Añadir el ejecutable y especificar el archivo fuente
add_executable(SocketServer main.c)

# Enlazar la biblioteca SocketUtil
target_link_libraries(SocketServer ${TheLibrary})
