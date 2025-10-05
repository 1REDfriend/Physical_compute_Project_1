// HondaECU_IT.c (Windows)
#define _WINSOCK_DEPRECATED_NO_WARNINGS
#include <winsock2.h>
#include <ws2tcpip.h>
#include <stdio.h>
#include <stdlib.h>

#pragma comment(lib, "ws2_32.lib")

int main(int argc, char** argv) {
    const char* ip = (argc > 1) ? argv[1] : "192.168.4.1";
    int port = (argc > 2) ? atoi(argv[2]) : 3333;

    WSADATA wsa;
    if (WSAStartup(MAKEWORD(2,2), &wsa) != 0) { fprintf(stderr, "WSAStartup failed\n"); return 1; }

    SOCKET s = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (s == INVALID_SOCKET) { fprintf(stderr, "socket failed\n"); WSACleanup(); return 1; }

    struct sockaddr_in addr; addr.sin_family = AF_INET; addr.sin_port = htons((u_short)port);
    addr.sin_addr.s_addr = inet_addr(ip);

    if (connect(s, (struct sockaddr*)&addr, sizeof(addr)) == SOCKET_ERROR) {
        fprintf(stderr, "connect failed: %d\n", WSAGetLastError());
        closesocket(s); WSACleanup(); return 1;
    }

    printf("connected to %s:%d\n", ip, port);

    // set 3s recv timeout so we don't block forever
    DWORD tv = 3000;
    setsockopt(s, SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv, sizeof(tv));

    char buf[1024];
    for (;;) {
        int n = recv(s, buf, sizeof(buf)-1, 0);
        if (n > 0) {
            buf[n] = 0;
            fputs(buf, stdout);
            fflush(stdout);
        } else if (n == 0) {
            fprintf(stderr, "\nserver closed\n");
            break;
        } else {
            int err = WSAGetLastError();
            if (err == WSAETIMEDOUT) {
                fprintf(stderr, "[no data yet]\n");
                continue;
            } else {
                fprintf(stderr, "recv error: %d\n", err);
                break;
            }
        }
    }

    closesocket(s);
    WSACleanup();
    return 0;
}
