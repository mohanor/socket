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
    const char* response = "HTTP/1.1 200 Ok\r\nServer: nginx/1.21.5\r\nDate: Sat, 04 Mar 2023 02:44:26 GMT\r\nContent-Type: text/html; charset=utf-8\r\nContent-Length: 51\r\nLast-Modified: Tue, 28 Feb 2023 21:07:43 GMT\r\nConnection: keep-alive\r\nKeep-Alive: timeout=15\r\nAccept-Ranges: bytes\r\n\r\n";
    int counter = 0;
    int nfds = 1, current_size = 0;


    struct sockaddr_in srv;
    int addrlen = sizeof(srv);

    int fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd < 0)
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

    if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0)
    {
        cout << "setsockopt failed" << '\n';
        close(fd), exit(EXIT_FAILURE);
    }

    if (bind(fd, (struct sockaddr *)&srv, addrlen) < 0)
    {
        cout << "bind failed" << '\n';
        close(fd), exit(EXIT_FAILURE);
    }

    int nRet = listen(fd, 5);
    if (nRet < 0)
    {
        cout << "fail to listen to local port" << '\n';
        close(fd), exit(EXIT_FAILURE);
    }


    struct pollfd fds[200];
    
    /*************************************************************/
    /* Initialize the pollfd structure                           */
    /*************************************************************/
    memset(fds, 0, sizeof(fds));
    
    fds[0].fd = fd;
    fds[0].events = POLLIN;

    int rc;
    int new_ds;

	while (true)
    {

        printf("Waiting ...\n");
        rc = poll(fds, nfds, -1);
        if (rc < 0)
        {
            perror("poll() failed");
            break;
        }

        
        if (rc == 0)
        {
            printf("poll() timed out.  End program.\n");
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
                printf("Error! revents = %d\n", fds[i].revents);
                close(fds[i].fd);
                break;
            }

            if (fds[i].fd == fd)
            {
                //do {
                    new_ds = accept(fd, (struct sockaddr*)&srv , (socklen_t*)&addrlen);

                    if (new_ds < 0)
                    {
                        cout << "fail to accept connection" << '\n';
                        return 1;
                    }

                    fds[nfds].fd = new_ds;
                    fds[nfds].events = POLLIN;
                    nfds++;

                    printf("New incoming connection  %d\n", new_ds);

                //} while (new_ds != -1);
            }
            else
            {
                printf("Descriptor %d is readable\n", fds[i].fd);

                do
                {
                    rc = recv(fds[i].fd, buffer, sizeof(buffer), 0);

                    if (rc < 0)
                    {
                        perror("recv() failed");
                        close(fds[i].fd);
                        break;
                    }

                    if (rc == 0)
                    {
                        printf("Connection closed\n");
                        close(fds[i].fd);
                        break;
                    }

                    int len = rc;
                    printf("%d bytes received\n", len);

                    rc = send(fds[i].fd, "hello world!!", strlen("hello world!!"), 0);
                    if (rc < 0)
                    {
                        perror("send() failed");
                        break;
                    }

                } while (true);
            }

        }

        // int clientSocket = accept(fd, (struct sockaddr*)&srv , (socklen_t*)&addrlen);
        // if (clientSocket < 0)
        // {
        //     cout << "fail to accept connection" << '\n';
        //     return 1;
        // }
        
        // cout << "clientSocket " << clientSocket << '\n';
        // int valrecv = recv(clientSocket, buffer, 1024, 0);
        // send(clientSocket, response, strlen(response), 0);

        // send(clientSocket, "Hello, World!!", strlen("Hello, World!!"), 0);

        // close(clientSocket);
        
    }

    return 0;
}