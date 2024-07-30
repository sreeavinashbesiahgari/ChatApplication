#include<iostream>
#include<netdb.h>
#include<netinet/in.h>
#include<cstdlib>
#include<cstring>
#include<sys/socket.h>
#include<sys/types.h>
#include<unistd.h>
#include<sys/select.h>
#include<map>
#include<csignal>
#include<fstream>
#include <arpa/inet.h>
#include <vector>
#include <dirent.h>
#include<unordered_map>
#include <algorithm>
#define MAX_MESSAGE_LEN 1024
#define max_cli 10
#define SA struct sockaddr
using namespace std;

namespace server {
void handle_client(int fd) ;
std::unordered_map<std::string, int> COMS{
        {"CHAT",0},
        {"LOGIN",1},
        {"LOGOUT",2},
        {"EXIT",3},
        {"UNKNOWN", -1}
    };
    
enum CMDS{
    CHAT, LOGIN, LOGOUT, EXIT_C_, UNKNOWN
};

struct mssg{
    string to, from , mesg;
};

map<int, string> fd_to_unames_;
int clients_fds[max_cli] , PORT_ , serverfd;
bool is_logged_in(int fd) {
    for (const auto& [key, value] : fd_to_unames_) {
        if (key == fd && key != -1) {
            return true;
        }
    }
    return false;
}


void send_mesg_(int fd, mssg mesg){
    string sendstr = mesg.from + " >> " + mesg.mesg ;
    int bytes_sent = send(fd, sendstr.c_str(), sendstr.size(), 0);
    if (bytes_sent == -1) {
        cerr << "Error: Failed to send message to client" << endl;
    }
}
mssg Handle_Message(string buffer) {
    mssg msgdecoded;
    if (buffer.size() < 6) {
        return msgdecoded;
    }
    if (buffer[5] == '@') {
        size_t pos = buffer.find(' ', 6);
        if (pos == string::npos) {
            return msgdecoded;
        }
        msgdecoded.to = buffer.substr(6, pos - 6);
        msgdecoded.mesg = buffer.substr(pos + 1);
    } else {
        msgdecoded.to = "all";
        msgdecoded.mesg = buffer.substr(5);
    }
    return msgdecoded;
}
void process_client(int client_fd);

string Exec_chat(int senderfd, const string& buffer) {
    auto sender_it = fd_to_unames_.find(senderfd);
    if (sender_it == fd_to_unames_.end()) {
        return "Error: user for file descriptor '" + to_string(senderfd) + "' not found.";
    }
    string sender = sender_it->second;

    mssg mesg = std::move(Handle_Message(buffer));
    mesg.from = sender;
    if (mesg.mesg.empty() || mesg.to.empty()) {
        return "Error:  CHAT command.";
    }
    if (mesg.to == "all") {
        for (auto & [client_fd, client_name] : fd_to_unames_) {
            if (client_fd != senderfd) {
                send_mesg_(client_fd, mesg);
            }
        }
        return "broadcasted '" + sender + "'!";
    }
    for (auto& [client_fd, client_name] : fd_to_unames_) {
        if (client_name == mesg.to) {
            send_mesg_(client_fd, mesg);
            return "Message sent from '" + sender + "' to '" + mesg.to + "'!";
        }
    }
    return "Error: user '" + mesg.to + "' not found.";
}


CMDS Exec_Cmd(int fd, const string& command) {
    const string commands[] = {"exit", "login", "logout", "chat"};
    const CMDS command_types[] = {EXIT_C_, LOGIN, LOGOUT, CHAT};
    for (size_t i = 0; i < sizeof(commands)/sizeof(commands[0]); ++i) {
        if (command.find(commands[i]) == 0) {
            return command_types[i];
        }
    }
    return UNKNOWN;
}

void r_config_(const string &conf_file_)
{
    ifstream con_f(conf_file_);
    if (!con_f.is_open())
    {
        cerr << "Error: Could not open configuration file " << conf_file_ << endl;
        exit(EXIT_FAILURE);
    }

    string line;
    bool port_found = false;
    while (getline(con_f, line))
    {
        if (line.find("port:") == 0)
        {
            string port_string = line.substr(5);
            if (!port_string.empty())
            {
                try
                {
                    PORT_ = stoi(port_string);
                    port_found = true;
                }
                catch (const std::invalid_argument &e)
                {
                    cout << "Invalid port number: " << port_string << endl;
                    exit(1);
                }
                catch (const std::out_of_range &e)
                {
                    cout << "Port number out of range: " << port_string << endl;
                    exit(1);
                }
            }
        }
        else
        {
            cout << "Invalid line in config file: " << line << endl;
            exit(1);
        }
    }
    con_f.close();

    if (!port_found)
    {
        cout << "Port number not found in config file." << endl;
        exit(1);
    }
}
void handle_exit_command(int fd)  {
           if (fd_to_unames_.count(fd) > 0) {
            fd_to_unames_.erase(fd);
             }
    if (close(fd) == -1) {
        cout << " Error closing the file descriptor...... " << endl;
    }
    fd = 0;
}
void signal_handler(int signal_num){

    mssg responsemsg{"server", "server down!"};
    for (int fd : clients_fds) {
        if (fd > 0) {
            send_mesg_(fd, responsemsg);
            close(fd);
        }
    }
    close(serverfd);
    cout << "\rserver closed!" << endl;
    exit(signal_num);
}
};

