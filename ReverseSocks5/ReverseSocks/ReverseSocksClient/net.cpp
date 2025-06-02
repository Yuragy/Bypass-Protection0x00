#include"Globals.h"
#include <iostream>
using namespace std;

void new_connection(char* buff, long long int id,int size);
void ExitPr() {


    cout << "Connection to server was lost !\nExit...";
    ExitProcess(1);
}
void connectback(char *ip,unsigned short port) {

    SOCKADDR_IN inf;
    memset(&inf, 0, sizeof(inf));
    inf.sin_family = AF_INET;
    inf.sin_port = htons(port);
    inet_pton(AF_INET, ip, &inf.sin_addr.s_addr);

    while (1) {
        s = socket(AF_INET, SOCK_STREAM, 0);
        if (connect(s, (sockaddr*)&inf, sizeof(inf)) != SOCKET_ERROR) { break; }
        Sleep(3000);
        closesocket(s);


    }

}
void clean_connection(list<socket_details*>::iterator tmp) {
    socket_details* sd = *tmp;
    closesocket(sd->sc);
    delete* tmp;
    clients.erase(tmp);




}

void parse_packet(char* buff, int len, long long unsigned int id) {

    mc.lock();
    list<socket_details*>::iterator tmp;

    for (tmp = clients.begin(); tmp != clients.end(); tmp++) {
        if ((*tmp)->id == id) {
            send((*tmp)->sc, buff, len, 0);
            if (len == 0) { closesocket((*tmp)->sc); }
            mc.unlock(); return;
        }
    }
    mc.unlock();


}
void clean_connection(socket_details* sd) {
    mc.lock();
    list<socket_details*>::iterator tmp;
    for (tmp = clients.begin(); tmp != clients.end(); tmp++) {
        if ((*tmp)->id == sd->id) {
            clean_connection(tmp);
            break;
        }
    }
    mc.unlock();


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


int send_data(char* buff, int len, long long unsigned int id) {

    m.lock();
    int res = 0;
    if (sendall(s, (char*)&len, sizeof(len)) == SOCKET_ERROR) { res = -1; }
    if (sendall(s, (char*)&id, sizeof(id)) == SOCKET_ERROR) { res = -1; }
    if (sendall(s, buff, len) == SOCKET_ERROR) { res = -1; }
    m.unlock();
    return res;
}
int recvit(SOCKET s, char* buffer, int count) {

    int tot = 0, rest, r;
    rest = count;
    while (tot < count) {
        r = recv(s, &buffer[tot], rest, 0);
        if (r == SOCKET_ERROR) { cout << "error 4 : " << GetLastError() << "\n"; return -1; }
        tot += r;
        rest = count - tot;


    }


    return 0;
}
int recvp(char* buffer, int buff_len) {

    int packet_len;
    long long unsigned int id;
    char order;


    if (recvit(s, (char*)&packet_len, sizeof(packet_len)) == SOCKET_ERROR) { cout << "error 1" << GetLastError() << "\n"; ExitPr(); } //recv packet_len
    if (recvit(s, (char*)&id, sizeof(id)) == SOCKET_ERROR) { cout << "error 2\n"; ExitPr(); }
    if (recvit(s, (char*)&order, 1) == SOCKET_ERROR) { cout << "error 3\n"; ExitPr(); }

    if (packet_len > buff_len) { cout << "invalid packet\n"; return 2; }

    if (recvit(s, buffer, packet_len) == SOCKET_ERROR) { ExitPr(); }


    if (order == NewConnexion) { new_connection(buffer, id,packet_len); }
    else if (order == ConnexionData) { parse_packet(buffer, packet_len, id); }
    else { cout << "unk !\n"; }
    return 0;



}


