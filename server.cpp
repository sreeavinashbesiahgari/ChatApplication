#include "server.h"


int main(int argc, char *argv[])
{
    signal(SIGINT, server::signal_handler);
    int co_fd_;
    struct sockaddr_in server_addr_, cli;
    
    
    int max_fd = 0, new_mesg_, mesg_length_, cli_len;
    char buffer[MAX_MESSAGE_LEN];

    if (argc < 2)
    {
        std::cerr << "No Config file given!" << std::endl;
        exit(1);
    }

    server::r_config_(argv[1]);
    int serverfd_ = socket(AF_INET, SOCK_STREAM, 0);
    if (serverfd_ == -1)
    {
        std::cerr << "Socket creation Error..." << std::endl;
        exit(1);
    }

    memset(&server_addr_, 0, sizeof(server_addr_));
    server_addr_.sin_family = AF_INET;
    server_addr_.sin_addr.s_addr = htonl(INADDR_ANY);
    server_addr_.sin_port = htons(server::PORT_);

    if (bind(serverfd_, (SA *)&server_addr_, sizeof(server_addr_)) != 0)
    {
        cerr << "Socket bind Fail....." << endl;
        exit(EXIT_FAILURE);
    }

    if (server::PORT_ == 0)
    {
        socklen_t addrlen = sizeof(server_addr_);
        getsockname(serverfd_, (SA *)&server_addr_, &addrlen);
        server::PORT_ = ntohs(server_addr_.sin_port);
        cout << "Server config mentions port as 0, selecting a random available port." << endl;
        cout << "ASSIGNED SERVER PORT: " << server::PORT_ << endl;

        char hostbuffer[256], *IPbuffer;
        gethostname(hostbuffer, sizeof(hostbuffer));
        struct hostent *host_entry = gethostbyname(hostbuffer);
        struct in_addr addr = {0};
        memcpy(&addr, host_entry->h_addr_list[0], sizeof(addr));
        IPbuffer = inet_ntoa(addr);

        char sentence[100] = "";
        snprintf(sentence, sizeof(sentence), "servhost:%s\nservport:%d", IPbuffer,server::PORT_);

        ofstream outfile("client.conf");
        if (outfile.is_open())
        {
            outfile << sentence;
            outfile.close();
        }
    }

    if (listen(serverfd_, 5) != 0)
    {
        cout << "Listen failed....." << endl;
        exit(0);
    }

    fd_set readmessages_fds;
    cli_len = sizeof(cli);
    
for (int i = 0; i < max_cli; ++i)
{
    server::clients_fds[i] = 0;
}
for (;;)
{
    FD_ZERO(&readmessages_fds);
    FD_SET(serverfd_, &readmessages_fds);
    max_fd = max(serverfd_, max_fd);

    std::for_each(std::begin(server::clients_fds), std::end(server::clients_fds),
                  [&](int fd_) {
        if (fd_ > 0) {
            FD_SET(fd_, &readmessages_fds);
            max_fd = std::max(fd_, max_fd);
        }
    });

    new_mesg_ = select(max_fd + 1, &readmessages_fds, NULL, NULL, NULL);
    if (new_mesg_ == -1)
    {
        cerr << "Select Error";
        exit(EXIT_FAILURE);
    }

    if (FD_ISSET(serverfd_, &readmessages_fds))
    {
        struct sockaddr cli;
        socklen_t cli_len = sizeof(cli);

        int co_fd_ = accept(serverfd_, &cli, &cli_len);
        if (co_fd_ < 0)
        {
            perror("accept");
            exit(EXIT_FAILURE);
        }

        int *empty_slot = std::find(server::clients_fds, server::clients_fds + max_cli, 0);
        if (empty_slot != server::clients_fds + max_cli)
        {
            *empty_slot = co_fd_;
        }
    }

    for (int i = 0; i < max_cli; ++i)
    {
        int fd_ = server::clients_fds[i];
        if (FD_ISSET(fd_, &readmessages_fds))
        {
            std::fill(buffer, buffer + MAX_MESSAGE_LEN, 0);
            server::mssg responsemsg;
            responsemsg.from = "server";
            mesg_length_ = read(fd_, buffer, MAX_MESSAGE_LEN);

            if (mesg_length_ <= 0)
            {
                cerr << "Error reading from file descriptor " << fd_ << endl;
                close(fd_);
                server::fd_to_unames_.erase(fd_);
                *std::remove(server::clients_fds, server::clients_fds + max_cli, fd_) = 0;
                max_fd = std::max(max_fd - 1, serverfd_);
                server::send_mesg_(fd_, {"connection closed!"});
                continue;
            }

            buffer[mesg_length_] = '\0';
            string bufferstring(buffer);
            
/*
 * auto process_exit_command = [&]() {
 *     responsemsg.mesg = "chat session exited.";
 *         server::send_mesg_(fd_, responsemsg);
 *
 *             if (auto it = server::fd_to_unames_.find(fd_); it != server::fd_to_unames_.end()) {
 *                     server::fd_to_unames_.erase(it);
 *                         }
 *
 *                             fd_ = -1;
 *                             };
 *                             */
/*
 * auto process_exit_command = [&]() {
 * responsemsg.mesg = "Goodbye! You have successfully exited the chat session.";
 * 					send_mesg_(fd_, responsemsg);
 * 										if (server::fd_to_unames_.count(fd_) > 0) 
 * 															{
 * 															    					server::fd_to_unames_.erase(fd_);
 * 															    										}
 * 															    															if (close(fd_) == -1)
 * 															    																				{
 * 															    																				    					cout << " Error closing the file descriptor...... " << endl ;
 * 															    																				    										}
 * 															    																				    															fd_ = 0;
 * 															    																				    																			} ;
 * 															    																				    																			*/    

   





    auto process_login_command = [&]() {
        if (bufferstring.size() < 7)
        {
            responsemsg.mesg = "arguments for LOGIN command is missing.";
            cout << responsemsg.mesg << endl;
            send_mesg_(fd_, responsemsg);
            return;
        }

        if (server::is_logged_in(fd_))
        {
            responsemsg.mesg = server::fd_to_unames_[fd_] + " is already logged in.";
        }
        else
        {
            string username = bufferstring.substr(6);
            auto it = std::find_if(server::fd_to_unames_.begin(), server::fd_to_unames_.end(),
               [&](const std::pair<int, std::string>& pair) { return pair.second == username; } ) ;


            if (it != server::fd_to_unames_.end())
            {
                responsemsg.mesg = it->second + " username is already in use.";
            }
            else
            {
                server::fd_to_unames_[fd_] = username;
                responsemsg.mesg = username + " logged in.";
            }
        }

        cout << responsemsg.mesg << endl;
        send_mesg_(fd_, responsemsg);
    };

    auto process_logout_command = [&]() {
        if (server::is_logged_in(fd_))
        {
            string username = server::fd_to_unames_[fd_];
            responsemsg.mesg = username + " has successfully logged out.";
            server::fd_to_unames_.erase(fd_);
        }
        else
        {
            responsemsg.mesg = "No user is currently logged in.";
        }

        cout << responsemsg.mesg << endl;
        send_mesg_(fd_, responsemsg);
    };

    auto process_chat_command = [&]() {
        if (!server::is_logged_in(fd_))
        {
            responsemsg.mesg = "chat can be used only after login.";
        }
        else
        {
            responsemsg.mesg = server::Exec_chat(fd_, bufferstring);
            responsemsg.mesg = responsemsg.mesg.substr(0, responsemsg.mesg.find("from"));
        }

        cout << responsemsg.mesg << endl;
        send_mesg_(fd_, responsemsg);
    };

    switch (server::Exec_Cmd(fd_, bufferstring))
    {
    case server::CMDS::EXIT_C_:
        {  
            responsemsg.mesg = "exited  ";
           cout << responsemsg.mesg << endl;
            send_mesg_(fd_, responsemsg);
           server::handle_exit_command(fd_);
    		break ;
    			}
    case server::CMDS::LOGIN:
        process_login_command();
        break;

    case server::CMDS::LOGOUT:
        process_logout_command();
        break;

    case server::CMDS::CHAT:
        process_chat_command();
        break;

    case server::CMDS::UNKNOWN:
        responsemsg.mesg = "DONT KNOW THIS COMMAND: " + bufferstring;
        send_mesg_(fd_, responsemsg);
        break;
    }
   }
  }
 }
}


