#ifdef _WIN32
#include <winsock2.h>
#define SOCKET_ERR INVALID_SOCKET
#else // *nix
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#define SOCKET_ERR -1
#endif

#include "risa.h"
#include "socket.h"


void stopServer(rv32iHart *cpu) {
#ifdef _WIN32
    shutdown(cpu->gdbFields.socketFd, SD_BOTH);
    closesocket(cpu->gdbFields.socketFd);
    WSACleanup();
#else // *nix
    shutdown(cpu->gdbFields.socketFd, SHUT_RDWR);
    close(cpu->gdbFields.socketFd);
#endif
}

int startServer(rv32iHart *cpu){
#ifdef _WIN32
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 0), &wsaData) != 0) {
        return -1;
    }
#endif
    struct sockaddr_in serverAddr, client;
    struct in_addr localhostaddr;
    cpu->gdbFields.socketFd = socket(AF_INET, SOCK_STREAM, 0);
    if (cpu->gdbFields.socketFd == SOCKET_ERR) {
        printf("[rISA]: Error. Socket creation for GDB failed.\n");
        return -1;
    }
    
#ifdef _WIN32
    localhostaddr.S_un.S_addr = htonl(INADDR_ANY);
#else
    localhostaddr.s_addr = htonl(INADDR_ANY);
#endif
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr = localhostaddr;
    serverAddr.sin_port = htons(cpu->gdbFields.serverPort);

    if ((bind(cpu->gdbFields.socketFd, (struct sockaddr*)&serverAddr, sizeof(serverAddr))) != 0) {
        printf("[rISA]: Error. Socket binding for GDB failed.\n");
        stopServer(cpu);
        return -1;
    }

    if ((listen(cpu->gdbFields.socketFd, 5)) != 0) {
        printf("[rISA]: Error. Socket server listening for GDB failed.\n");
        stopServer(cpu);
        return -1;
    }

    // Connect to client
    int len = sizeof(client);
    printf("\n[rISA]: GDB server listening on port %hu.\n", cpu->gdbFields.serverPort);
    cpu->gdbFields.connectFd = accept(cpu->gdbFields.socketFd, (struct sockaddr*)&client, &len);
    if (cpu->gdbFields.connectFd < 0) {
        printf("[rISA]: Error. Socket server accept for GDB failed.\n");
        stopServer(cpu);
        return -1;
    }

    return 0;
}

// TODO: Clean this up?
int readSocket(int clientSocket, char *packet, size_t len) {
    const int res = recv(clientSocket, packet, len, 0);

    switch (res)
    {
        case SOCKET_ERR:
            return READ_SOCKET_ERR_RECV;
        case 0:
            return READ_SOCKET_SHUTDOWN;
        default:
            len = res;
            break;
    }

    return READ_SOCKET_OK;
}
int writeSocket(int clientSocket, const char *packet, size_t len) {
    if (clientSocket <= 0)
        return -1;

    // TODO: POSIX? - Port to windows if so...
    if (send(clientSocket, packet, len, 0) == -1) {
        perror("send():");
    }

    return 0;
}