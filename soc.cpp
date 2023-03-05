#include <iostream>

#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#include <poll.h>

using namespace std;

#define PORT 8080



int main()
{
    
    char buffer[1024] = { 0 };
    const char* response = "HTTP/1.1 200 Ok\r\nServer: nginx/1.21.5\r\nContent-Type: text/html; charset=utf-8\r\nContent-Length: 15\r\n\r\n\r\nhello world!!";
    int counter = 0;
    int nfds = 1, current_size = 0;


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
            printf("poll() End program.\n");
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
            else
            {

                cout << "Descriptor " << fds[i].fd << " is readable" << endl;

                rc = recv(fds[i].fd, buffer, sizeof(buffer), 0);
                if (rc < 0)
                {
                    perror("recv() failed"), close(fds[i].fd);
                    break;
                }

                if (rc == 0)
                {
                    cout << "Connection closed" << endl;
                    close(fds[i].fd);
                    break;
                }

                cout << rc << " bytes received" << endl; // << buffer << endl;

                rc = send(fds[i].fd, response, strlen(response), 0);
                if (rc < 0)
                {
                    perror("send() failed");
                    break;
                }

            }

        }

    }

    return 0;
}