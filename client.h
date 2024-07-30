#include<iostream>
#include<netdb.h>
#include<stdlib.h>
#include<cstring>
#include<string>
#include<sys/socket.h>
#include<unistd.h>
#include<arpa/inet.h>
#include<pthread.h>
#include<csignal>
#include<array>
#include<fstream>
#include<sstream>
#define MAX_MSG_LEN 1024
#define MAX 80
#define SA struct sockaddr

using namespace std;

class ChatClient {

public:
int cfd;
string username, serv_host_;
int serv_port_;
static void* send(void* arg);
void *receive(void *arg);
void handle_messenger_(int fd, ChatClient cli);
static void signal_handler(int signal_num);
void read_config_(string config_file_path);
private :
static ChatClient* instance;
};
