#include <iostream>
#include<WinSock2.h>
#include<Windows.h>
#include <list>
#include<WS2tcpip.h>
#include<mutex>
#include"Header.h"
#include<string>

#pragma comment(lib,"WS2_32")

using namespace std;
#define ERROR_SOCKS_SERVER -90
int port_listen;
mutex m;

mutex mc;

SOCKET client;

char NewConnexion = 0x55;
char ConnexionData = 0x70;
char* reply = (char*)"\x05\x00";
long long unsigned int Gid = 0;


struct socket_details {
    SOCKET s;
    long long unsigned int id;
};

list<socket_details*> clients;

void close(int error) {

    cout << "Closing ... Error: " << error;
    exit(-1);

}

int sendall(SOCKET s, char* buf, int len)
{
    int total = 0;        // how many bytes we've sent
    int bytesleft = len; // how many we have left to send
    int n = 0;

    while (total < len) {
        n = send(s, buf + total, bytesleft, 0);
        if (n == -1) { break; }
        total += n;
        bytesleft -= n;
    }

    len = total; // return number actually sent here

    return n == -1 ? -1 : 0; // return -1 on failure, 0 on success
}

void sendpacket(char* buffer, int buff_len, SOCKET s, long long unsigned int id, char* order) {
    mc.lock();
    sendall(s, (char*)&buff_len, 4);
    sendall(s, (char*)&id, sizeof(id));
    sendall(s, order, 1);
    sendall(s, buffer, buff_len);
    mc.unlock();


    return;
}
void clean_connection(list<socket_details*>::iterator tmp) {
    socket_details* sd = *tmp;
    closesocket(sd->s);
    delete* tmp;
    clients.erase(tmp);
    return;

}
void clean_connection(socket_details* sd) {
    m.lock();
    list<socket_details*>::iterator tmp;
    for (tmp = clients.begin(); tmp != clients.end(); tmp++) {
        if ((*tmp)->id == sd->id) {
            clean_connection(tmp);
            break;
        }
    }
    m.unlock();


}
void _stdcall socks_connection(socket_details* sc) {

    char buff[8000];
    SOCKET s = sc->s;
    int r;
    if (recv(s, buff, 50, 0) == SOCKET_ERROR) { goto cleanup; }
    send(s, reply, 2, 0);
    r = recv(s, buff, 2000, 0);
    if (r == SOCKET_ERROR) { goto cleanup; }

    if (buff[0] != 0x05 || (buff[1] != 0x01 && buff[1] != 0x03)) { goto cleanup; }
    sendpacket(buff, r, client, sc->id, &NewConnexion);
    while (1) {
        r = recv(s, buff, sizeof(buff), 0);
        if (r <= 0) { sendpacket(buff, 0, client, sc->id, &ConnexionData); break; }
        else { sendpacket(buff, r, client, sc->id, &ConnexionData); }
    }
cleanup:
    clean_connection(sc); return;


}

void _stdcall socks_server() {
    HANDLE th;
    SOCKADDR_IN inf;
    memset(&inf, 0, sizeof(inf));
    inet_pton(AF_INET, "127.0.0.1", &inf.sin_addr.s_addr);
    inf.sin_port = htons(1080);
    inf.sin_family = AF_INET;

    SOCKET s = socket(AF_INET, SOCK_STREAM, 0);
    if (s == SOCKET_ERROR) { close(ERROR_SOCKS_SERVER); }
    if (bind(s, (sockaddr*)&inf, sizeof(inf))) { close(ERROR_SOCKS_SERVER); }
    if (listen(s, 100)) { close(ERROR_SOCKS_SERVER); }
    cout << "Socks 5 Server Started 127.0.0.1:1080\n";
    //accept socks connections
    while (1) {
        SOCKET client_socks = accept(s, NULL, NULL);
        socket_details* clinf = new socket_details;
        clinf->id = Gid;
        clinf->s = client_socks;
        m.lock();
        clients.push_front(clinf);
        Gid++;
        m.unlock();
        //start thread
        th = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)socks_connection, clinf, 0, 0);
        if (th) { CloseHandle(th); }


    }

}

int ServerStart() {
    SOCKADDR_IN inf;
    memset(&inf, 0, sizeof(inf));
    inet_pton(AF_INET, "0.0.0.0", &inf.sin_addr.s_addr);
    inf.sin_port = htons(port_listen);
    inf.sin_family = AF_INET;

    SOCKET s = socket(AF_INET, SOCK_STREAM, 0);
    if (bind(s, (sockaddr*)&inf, sizeof(inf))) { return -1; }
    if (listen(s, 100)) { return -1; }
    return s;

}


void handle_packet(char* p, int len, long long unsigned int id) {

    m.lock();
    list<socket_details*>::iterator tmp;
    for (tmp = clients.begin(); tmp != clients.end(); tmp++) {
        if ((*tmp)->id == id) {
            if (send((*tmp)->s, p, len, 0) == SOCKET_ERROR || len==0) { clean_connection(tmp);}
            break;
        }
    }
    m.unlock();
    return;


}

int recvit(SOCKET s, char* buffer, int count) {

    int tot = 0, rest, r;
    rest = count;
    while (tot < count) {
        r = recv(s, &buffer[tot], rest, 0);
        if (r == SOCKET_ERROR || r == 0) { cout << "error 4 : " << GetLastError() << "\n"; return -1; }
        tot += r;
        rest = count - tot;


    }


    return 0;
}
int recvp(char* buff, int buff_len, SOCKET s) {

    int packet_len;
    long long unsigned int id;
    char order;
    int tot = 0, rest;
    int r;

    if (recvit(s, (char*)&packet_len, sizeof(packet_len)) == SOCKET_ERROR) { return -1; }
    if (recvit(s, (char*)&id, sizeof(id)) == SOCKET_ERROR) { return -1; } 

    if (packet_len > buff_len) { return 2; }

    if (recvit(s, buff, packet_len) == SOCKET_ERROR) { return -1; }

    handle_packet(buff, packet_len, id);
    return 0;
}

int main(int argc,char *argv[])
{
    WSAData d;
    int r;
    char data[8000];
    if (WSAStartup(MAKEWORD(2, 0), &d)) {
        return -1;
    }
    cout << bnr<<"\n\n\t\t"<<"https://github.com/f3di006\n";
    if (argc < 2) { cout << "usage : ReverseSocksServer.exe port\n"; Sleep(10000); return -1; }
    string port = argv[1];
    port_listen = std::stoi(port);

    SOCKET server;
    server = ServerStart();
    cout << "Waiting for client\n";
    client = accept(server, NULL, NULL);
    cout << "Accepted client!!\n";
    //Start thread
    HANDLE th;
    th = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)socks_server, NULL, 0, 0);
    if (th) { CloseHandle(th); }
    else { cout << "Error Starting Thread ! \n"; exit(-1); }

    while (1) {

        r = recvp(data, sizeof(data), client);
        if (r == -1) { cout << "Client closed connection!\n"; break; }





    }



}

// Run program: Ctrl + F5 or Debug > Start Without Debugging menu
// Debug program: F5 or Debug > Start Debugging menu

// Tips for Getting Started: 
//   1. Use the Solution Explorer window to add/manage files
//   2. Use the Team Explorer window to connect to source control
//   3. Use the Output window to see build output and other messages
//   4. Use the Error List window to view errors
//   5. Go to Project > Add New Item to create new code files, or Project > Add Existing Item to add existing code files to the project
//   6. In the future, to open this project again, go to File > Open > Project and select the .sln file
