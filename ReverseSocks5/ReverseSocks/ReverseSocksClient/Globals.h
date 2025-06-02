#pragma once
#include<winsock2.h>
#include <vector>
#include<Windows.h>
#include <list>
#include<WS2tcpip.h>
#include<mutex>
#include <vector>
#include<future>
#pragma comment(lib,"WS2_32")
using namespace std;
extern char NewConnexion;
extern char ConnexionData;
extern unsigned char bnr[4838];
extern char* granted;

extern std::vector<std::future<void>> fu;

extern SOCKET s;
extern mutex m; // for main socket
extern mutex mc;//for list

struct connexion_details {

    long long unsigned int id;
    char d[2000];//host port
    int sizep;


};

struct socket_details {

    long long unsigned int id;
    SOCKET sc;

};
extern list<socket_details*> clients;
