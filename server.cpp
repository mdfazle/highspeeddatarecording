#include <iostream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <cstring>
#include <unistd.h>

int main()
{
    int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0)
    {
        std::cerr << "Failed to create socket" << std::endl;
        return 1;
    }

    int port = 1234;
    struct sockaddr_in addr = {0};
    addr.sin_family = AF_INET;
    // Replace INADDR_ANY with the IP address of the client
    addr.sin_addr.s_addr = inet_addr("127.0.0.1");
    addr.sin_port = htons(port);

    int n = 0;
    int packet_size = 8200;
    int num_packets = 1024;
    int total_data_size = packet_size * num_packets;

    char buffer[packet_size];
    memset(buffer, 'A', packet_size);

    // Send UDP packets until desired amount of data has been sent
    while (n < 1000000000)
    {
        int bytes_sent = sendto(sockfd, buffer, packet_size, 0,
                                (struct sockaddr *)&addr, sizeof(addr));
        if (bytes_sent < 0)
        {
            std::cerr << "Failed to send packet" << std::endl;
            close(sockfd);
            return 1;
        }
        n += bytes_sent;
    }

    close(sockfd);
    return 0;
}
