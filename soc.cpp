#include <iostream>

#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

# include <poll.h>
# include <iostream>
# include <fstream>
#include <sstream>

#include <sys/stat.h>

using namespace std;

#define PORT 8080

std::string getFileContent(std::string file_name)
{
    std::string content, line;
    std::ifstream MyReadFile(file_name);

    if (!MyReadFile.is_open())
        return "";

    while (getline (MyReadFile, line))
        content += line + '\n';

    MyReadFile.close();
    return content.substr(0, content.size() - 1);
}

size_t getFileSize(std::string filename) { return getFileContent(filename).size(); }

int sendFileInPackets(std::string file, struct pollfd *fds, int i)
{
    std::stringstream response;
    
    response << "HTTP/1.1 200 Ok\r\n";
    response << "Server: nginx/1.21.5\r\n";
    response << "Content-Type: text/html\r\n";
    response << "charset=utf-8\r\n";
    response << "Content-Length: " + std::to_string(getFileSize(file)) + "\r\n\r\n";

    if (getFileContent(file).size() > 1024)
    {
        send(fds[i].fd, response.str().c_str(), response.str().size(), 0);

        char buffer[1023] = { 0 };

        ifstream responsedFile(file.c_str(), ios::binary);

        while (responsedFile.read(buffer, 1024))
        {
            send(fds[i].fd, buffer, 1024, 0);
            std::cout << "buffer: " << buffer << std::endl;
            memset(buffer, 0, 1024);
        }

        int bytesRemaining = responsedFile.gcount();
        if (bytesRemaining > 0) {
            responsedFile.read(buffer, bytesRemaining);
            send(fds[i].fd, buffer, bytesRemaining, 0);
        }

        return 0;
    }

    response << getFileContent(file);
    send(fds[i].fd, response.str().c_str(), response.str().size(), 0);

    return 0;
}

int communicate(struct pollfd *fds, int i)
{
    int size;
    char buffer[1024] = { 0 };

    cout << "Descriptor " << fds[i].fd << " is readable" << endl;

    size = recv(fds[i].fd, buffer, sizeof(buffer), 0);
    if (size < 0)
    {
        cout << "recv failed" << endl; close(fds[i].fd);
        return false;
    }

    if (not size)
    {
        cout << "Connection closed" << endl;
        close(fds[i].fd);
        return false;
    }

    cout << size << " bytes received" << endl;
    std::string file = "index.html";

    sendFileInPackets(file, fds, i);

    return true;
}


int main()
{
    
    char buffer[1024] = { 0 };
    int counter = 0;
    int nfds = 1, current_size = 0;



    // std::cout << file << std::endl;


    struct sockaddr_in srv;
    int addrlen = sizeof(srv);

    int listener = socket(AF_INET, SOCK_STREAM, 0);
    if (listener < 0)
    {
        cout << "the socket not opened" << '\n';
        return 1;
    }

    memset(&srv, 0, addrlen);
    srv.sin_family = AF_INET;
    srv.sin_port = htons(8080);
    srv.sin_addr.s_addr = INADDR_ANY;

    int opt = 1;

    /*************************************************************/
    /* Allow socket descriptor to be reuseable                   */
    /*************************************************************/

    if (setsockopt(listener, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0)
    {
        cout << "setsockopt failed" << '\n';
        close(listener), exit(EXIT_FAILURE);
    }

    if (bind(listener, (struct sockaddr *)&srv, addrlen) < 0)
    {
        cout << "bind failed" << '\n';
        close(listener), exit(EXIT_FAILURE);
    }

    int nRet = listen(listener, 5);
    if (nRet < 0)
    {
        cout << "fail to listen to local port" << '\n';
        close(listener), exit(EXIT_FAILURE);
    }


    struct pollfd fds[20];
    
    /*************************************************************/
    /* Initialize the pollfd structure                           */
    /*************************************************************/
    memset(fds, 0, sizeof(fds));
    

    for(int i = 0; i < 20; i++)
        fds[i].fd = -1;
    
    fds[0].fd = listener;
    fds[0].events = POLLIN;

    int rc;
    int new_client;

	while (true)
    {

        cout << endl << "waiting ..." << endl;

        /*********************************************************************/
        /* return the number of fds that are ready for reading               */
        /* (stop scanning after i find that many)                            */
        /* deleting items from the set => set any fd field to a negative num */
        /* ber and poll() will ignore it                                     */
        /*********************************************************************/
    
        rc = poll(fds, nfds, -1);
        cout << "poll return " <<  rc << endl;
        
        if (rc < 0)
        {
            perror("poll() failed");
            break;
        }

        if (rc == 0)
        {
            cerr << "poll End program." << endl;
            break;
        }

        current_size = nfds;
        cout << "current_size " << current_size << '\n';
        for(int i = 0; i < current_size; i++)
        {
            if (fds[i].revents == 0)
                continue;
            
            if (fds[i].revents != POLLIN)
            {
                cerr << "Error! revents = " << fds[i].revents << endl;
                close(fds[i].fd);
                fds[i].fd = -1;
                break;
            }

            // acceptConnection
            if (fds[i].fd == listener)
            {
                
                new_client = accept(listener, (struct sockaddr*)&srv , (socklen_t*)&addrlen);
                if (new_client < 0)
                {
                    cerr << "fail to accept connection" << '\n';
                    return 1;
                }

                fds[nfds].fd = new_client;
                fds[nfds].events = POLLIN;
                nfds++;

                cout << "New incoming connection " << new_client << endl;

            }
            else if ( !communicate(fds, i) )
                break;

        }

    }

    return 0;
}