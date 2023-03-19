#include <iostream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <cstring>
#include <unistd.h>
#include <thread>
#include <vector>
#include <fstream>

#include "ThreadPool.h"

const int NUM_THREADS = std::thread::hardware_concurrency();
const std::string OUTPUT_FILE_NAME = "Output/PacketsReceived_";
std::vector<std::ofstream *> FILE_POOL;
std::vector<ThreadPool *> THREAD_POOL;
int TotalObjectReceived = 0;

void WriteToFile(int sockfd, int sequence)
{
    fd_set rfds;
    struct timeval tv;
    int retval;

    /* Watch sockfd to see when it has input. */
    FD_ZERO(&rfds);
    FD_SET(sockfd, &rfds);

    /* Wait up to five seconds. */
    tv.tv_sec = 0;
    tv.tv_usec = 2000;

    retval = select(sockfd + 1, &rfds, NULL, NULL, &tv);
    if (retval == -1)
    {
        std::cerr << "Failed to call select()" << std::endl;
    }
    else if (retval == 0)
    {
        std::cerr << "Timeout occurred" << std::endl;
    }
    else
    {
        if (FD_ISSET(sockfd, &rfds))
        {
            // Socket is ready to read
            char buffer[8200];
            int bytes = recv(sockfd, buffer, sizeof(buffer), 0);
            if (bytes < 0)
            {
                std::cerr << "Failed to receive packet" << std::endl;
                close(sockfd);
                return;
            }
            // Process the received data
            FILE_POOL[sequence]->write(buffer, bytes);
        }
    }
}

int main()
{
    std::cout << "number of available threads " << NUM_THREADS << std::endl;

    for (int i = 0; i < NUM_THREADS; i++)
    {
        FILE_POOL.push_back(new std::ofstream(OUTPUT_FILE_NAME + std::to_string(i) + ".bin", std::ios_base::app | std::ios_base::out));
        THREAD_POOL.push_back(new ThreadPool(1));
    }
    int sockfd = socket(AF_INET, SOCK_DGRAM, 0);

    if (sockfd < 0)
    {
        std::cerr << "Failed to create socket" << std::endl;
        return 1;
    }

    int port = 1234;
    struct sockaddr_in addr = {0};
    addr.sin_family = AF_INET;
    // Replace INADDR_ANY with the IP address of the server
    addr.sin_addr.s_addr = inet_addr("127.0.0.1");
    addr.sin_port = htons(port);

    if (bind(sockfd, (struct sockaddr *)&addr, sizeof(addr)) < 0)
    {
        std::cerr << "Failed to bind socket" << std::endl;
        close(sockfd);
        return 1;
    }

    int sequence_number;

    while (TotalObjectReceived < 1309441)
    {
        sequence_number = TotalObjectReceived % NUM_THREADS;
        THREAD_POOL[sequence_number]->enqueue(WriteToFile, sockfd, sequence_number);
        TotalObjectReceived++;
        std::cout << TotalObjectReceived << std::endl;
    }

    for (int i = 0; i < NUM_THREADS; i++)
    {
        THREAD_POOL[i]->wait();
        std::cout << "Theead " << i << " is Done!" << std::endl;
        FILE_POOL[i]->close();
    }

    close(sockfd);
    return 0;
}
