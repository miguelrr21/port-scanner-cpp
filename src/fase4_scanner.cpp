// fase1_single_port.cpp
// Objetivo: comprobar si UN puerto de una IP esta abierto, usando Winsock (Windows).
// Uso: fase1_single_port.exe <ip> <puerto>




#include <iostream>
#include <string>
#include <winsock2.h>   // Funciones principales de Winsock (socket, connect...)
#include <ws2tcpip.h>   // Funciones auxiliares modernas (inet_pton, etc.)
//V2
#include <chrono>
//V3
#include <thread>
#include <mutex>
#include <vector>

//"Controlador de escritura compartido"
std::mutex mutex_salida;

// Le decimos al linker que enlace la libreria de Winsock.
// En Linux esto no hace falta, pero en Windows si no lo pones, el programa no compila.
#pragma comment(lib, "ws2_32.lib")

//v3 creo una funcion que me permite conectar mediante una ip y un puerto
void scanner(std::string ip, int port){
// 3. Crear el socket
    // socket(domain, type, protocolo)
    // AF_INET      -> vamos a usar direcciones IPv4
    // SOCK_STREAM  -> queremos un socket orientado a conexion (esto es lo que implica TCP)
    // IPPROTO_TCP  -> protocolo TCP explicitamente
     SOCKET sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (sock == INVALID_SOCKET) {
         std::cerr << "Error al crear el socket. Codigo: " << WSAGetLastError() << std::endl;
         return;
        }

    

        // 5. Preparar la direccion de destino 
        // sockaddr_in es una estructura que representa "IP + puerto" en el formato
        // que entiende la API de sockets (no es un simple string).
        sockaddr_in destino{};
        destino.sin_family = AF_INET;
        destino.sin_port = htons(port);
        // htons = "host to network short": convierte el numero de puerto al formato
        // de bytes que espera la red (big-endian), independientemente de como lo
        // guarde internamente tu procesador. Es obligatorio hacer esta conversion.

        // inet_pton convierte el texto de la IP ("192.168.1.1") a su forma binaria.
        if (inet_pton(AF_INET, ip.c_str(), &destino.sin_addr) <= 0) {
            std::cerr << "IP invalida: " << ip << std::endl;
            closesocket(sock);
            return;
        }

        // 6. Intentar conectar
        // Aqui es donde realmente ocurre el "three-way handshake" que hablamos:
        // el sistema operativo manda el SYN, espera SYN-ACK, manda ACK.
        // connect() nos devuelve 0 si se completo con exito.
        
        char buffer[1024];
        
        auto inicio = std::chrono::steady_clock::now();
        int resultadoConnect = connect(sock, (sockaddr*)&destino, sizeof(destino));
        auto fin = std::chrono::steady_clock::now();
        
        {//v3 mutex, el "candado"
            std::lock_guard<std::mutex> candado(mutex_salida);

            std::cout << "Puerto " << port << ": " << std::chrono::duration_cast<std::chrono::milliseconds>(fin - inicio).count() << " ms" << std::endl;
            if (resultadoConnect == 0) {

                //v4
                DWORD timeout = 1000; // en milisegundos
                setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, (const char*)&timeout, sizeof(timeout));
                int n_bytes = recv(sock, buffer, sizeof(buffer) -1 ,0);
                if (n_bytes > 0){
                    buffer[n_bytes] = '\0';
                    std::cout << "Puerto " << port << " en " << ip << " -> ABIERTO con mensaje: " << buffer<< std::endl;

                }else
                    std::cout << "Puerto " << port << " en " << ip << " -> ABIERTO" << std::endl;

            } else {
                // Si falla, distinguimos (a grandes rasgos) el motivo usando el codigo de error.
                int error = WSAGetLastError();
                if (error == WSAETIMEDOUT) 
                    std::cout << "Puerto " << port << " en " << ip << " -> FILTRADO (timeout, sin respuesta)" << std::endl;
                else 
                    std::cout << "Puerto " << port << " en " << ip << " -> CERRADO (RST recibido o rechazo inmediato)" << std::endl;
                
            }
        }

        // Cerramos el socket y liberamos Winsock. Importante hacerlo siempre,
        // incluso en los caminos de error de arriba (si no, se "filtran" recursos).
        closesocket(sock);
}


int main(int argc, char* argv[]) {

    // 1. Leer argumentos
    // argv[0] es el nombre del programa, argv[1] la IP, argv[2] el puerto.
    if (argc != 3) {
        std::cerr << "Uso: " << argv[0] << " <ip> <puerto>" << std::endl;
        return 1;
    }
    std::string ip = argv[1];
    std::string rango = argv[2];
    size_t pos = rango.find("-");
    if (pos == std::string::npos){
        std::cout << "Error: sintaxis argumento rango de puertos erronea 'x-y' " << std::endl;
        return 1;
    }

    int portIni = std::stoi(rango.substr(0, pos));
    int portFin = std::stoi(rango.substr(pos+1));

    //int port = std::stoi(argv[2]); // convierte el texto "80" al numero 80

    // 2. Inicializar Winsock 
    // En Windows, antes de crear ningun socket, hay que "arrancar" la libreria.
    // Esto no existe en Linux: alli el sistema operativo ya tiene los sockets listos siempre.
    WSADATA wsaData;
    int resultadoInit = WSAStartup(MAKEWORD(2, 2), &wsaData);
    // MAKEWORD(2,2) pide la version 2.2 de Winsock, la estandar desde hace mas de 20 anios.
    if (resultadoInit != 0) {
        std::cerr << "Error al inicializar Winsock. Codigo: " << resultadoInit << std::endl;
        return 1;
    }


    //v3, creo un vector donde almacenar los hilos
    std::vector<std::thread> hilos_puertos;


    //v3 creo los hilos y los almaceno en el vector
    for (int port = portIni ; port <= portFin ; port++){
        std::thread puerto(scanner,ip,port);
        hilos_puertos.push_back(std::move(puerto));
    }

    //v3 
    for(auto& puerto : hilos_puertos){
        puerto.join();
    }

    WSACleanup();
    

    return 0;
}
