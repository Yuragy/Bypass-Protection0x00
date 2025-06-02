#pragma once
#include"Globals.h"
void parse_packet(char* buff, int len, long long unsigned int id);
int recvit(SOCKET s, char* buffer, int count);
int recvp(char* buffer, int buff_len);
int send_data(char* buff, int len, long long unsigned int id);
int sendall(SOCKET s, char* buf, int len);
void clean_connection(socket_details* sd);
void clean_connection(std::list<socket_details*>::iterator tmp);
void connectback(char* ip,unsigned short port);