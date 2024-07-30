#include "client.h"

void* ChatClient::receive(void* arg) {
    int soc_fd = *static_cast<int*>(arg);
    std::string buffer(MAX_MSG_LEN, '\0');
    
    while (true) {
        ssize_t m_len = read(soc_fd, static_cast<void*>(const_cast<char*>(buffer.data())), MAX_MSG_LEN);
        
        if (m_len <= 0) {
            close(soc_fd);
            std::cout << "\rclosed connection\n";
            std::exit(0);
        }
        
        std::cout << "\r" << buffer.substr(0, m_len) << "\n>>" << std::flush;
    }
    
    pthread_exit(nullptr);
}





void *ChatClient::send(void *arg){
    int fd = *static_cast<int*>(arg);;
    string command;
    for (;;){
        cout<<">>";
        getline(cin, command);
        write(fd, command.c_str(), command.size()*sizeof(char));
    }
    pthread_exit(NULL);
}
static void* receive_wrapper(void* arg) {
    ChatClient* cli = static_cast<ChatClient*>(arg);
    return cli->receive(arg);
}

void ChatClient::signal_handler(int signal_num){
    ChatClient cli;
    cout<<"\rclient closed!"<<endl;
    close(cli.cfd);
    exit(signal_num);
}



void ChatClient::handle_messenger_(int fd, ChatClient cli){
    pthread_t receiver_t, sender_t;
    pthread_attr_t attr;
    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);
    pthread_create(&receiver_t, &attr, receive_wrapper, (void*)&cli);
    pthread_create(&sender_t, &attr, cli.send, (void *)&fd);
    pthread_attr_destroy(&attr);
    pthread_join(receiver_t, NULL);
    pthread_join(sender_t, NULL);
}

void ChatClient::read_config_(string config_file_path){
    ifstream config_file(config_file_path.c_str());
    if(!config_file){
        cout<<"File doesn't exist: "<<config_file_path<<endl;
        exit(EXIT_FAILURE);
    }
    string configline;
    while (getline(config_file, configline)) {       
        istringstream iss(configline);
        string key, value;
        getline(iss, key, ':');
        getline(iss, value, ':');
        if (key == "servhost") {
            if (value.empty()) {
                cerr << "No host found: " << value << endl;
                exit(EXIT_FAILURE);
            }
            serv_host_ = value;
        } else if (key == "servport") {
            if (value.empty()) {
                cerr << "Invalid port: " << value << endl;
                exit(EXIT_FAILURE);
            }
            try {
                serv_port_ = std::stoi(value);
            } catch (...) {
                cerr << "Connection with Server Failed Invalid port number: " << value << endl;
                exit(EXIT_FAILURE);
            }
        } else {
            cerr << "Wrong config at line: " << configline << endl;
            exit(EXIT_FAILURE);
        }
}
}


int main(int argc, char *argv[]){
    ChatClient *client = new ChatClient(); 
    signal(SIGINT, ChatClient::signal_handler);
    
    struct sockaddr_in servaddr, cli;
    if(argc < 2){
        cout<<"USAGE: client client.conf "<<endl;
        exit(EXIT_SUCCESS);
    }
    client->read_config_(argv[1]);
    client->cfd = socket(AF_INET, SOCK_STREAM, 0);
    if (client->cfd == -1) {
        printf("socket creation failed...\n");
        exit(0);
    }
    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = inet_addr(client->serv_host_.c_str());
    servaddr.sin_port = htons(client->serv_port_);
    if(connect(client->cfd, (SA*)&servaddr, sizeof(servaddr)) != 0 ){
        printf("connection with the server failed...\n");
        exit(0);
    
    }
    client->handle_messenger_(client->cfd, *client); 
    close(client->cfd);
    delete client; 
}

