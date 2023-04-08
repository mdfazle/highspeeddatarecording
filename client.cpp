#include <iostream>
#include <sys/socket.h>
#include <cstring>
#include <unistd.h>
#include <vector>
#include "ThreadPool/ThreadPool.h"
#include <chrono>
#include <limits.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <mutex>

// Constants
const unsigned short SERVER_PORT = 1234;
const char *SERVER_IP_ADDRESS = "127.0.0.1";
const int PACKET_SIZE = 8200;
const char *FILENAME = "Output/UDP_OUT";
const int NUMBER_OF_THREADS = std::thread::hardware_concurrency();

// Global variables
int COLLECTION_SIZE = 0;
std::vector<FILE *> FilePool;
std::vector<ThreadPool *> myReaderPool;
std::vector<ThreadPool *> myWriterPool;
int MESSAGE_COUNTER = 0;

std::mutex CounterMTX;

// Function declarations
void WriteToFile(std::vector<char> *buffer, std::vector<char> Header, const int bytes, FILE *fp);
void ThreadFunc(const int server_fd, const int file_no, const int size_to_collect);
void intToCharArray(const int &value, std::vector<char> &result);

int main(int argc, char **argv)
{

    // Check command line arguments
    if (argc < 2)
    {
        std::cout << "Usage: " << argv[0] << " [size in GB]" << std::endl;
        return 1;
    }
    COLLECTION_SIZE = std::atoi(argv[1]);

    // Compute size to collect per thread
    const long long size_to_collect = (1073741824 / NUMBER_OF_THREADS) * COLLECTION_SIZE;

    std::cout << "Program will Collect " << COLLECTION_SIZE << " GB of data" << std::endl;

    // Initialize file and thread pools
    for (int i = 0; i < NUMBER_OF_THREADS; i++)
    {
        FilePool.push_back(fopen((FILENAME + std::to_string(i) + ".bin").c_str(), "w+"));
        myReaderPool.push_back(new ThreadPool(1));
        myWriterPool.push_back(new ThreadPool(1));
    }

    // Create a UDP socket
    int server_socket = socket(AF_INET, SOCK_DGRAM, 0);
    if (server_socket < 0)
    {
        std::cerr << "Error creating server socket" << std::endl;
        return 1;
    }

    // Bind the socket to the specified port
    struct sockaddr_in server_address;
    std::memset(&server_address, 0, sizeof(server_address));
    server_address.sin_family = AF_INET;
    server_address.sin_addr.s_addr = htonl(INADDR_ANY);
    server_address.sin_port = htons(SERVER_PORT);
    int bind_result = bind(server_socket, (struct sockaddr *)&server_address, sizeof(server_address));
    if (bind_result < 0)
    {
        std::cerr << "Error binding server socket to port" << std::endl;
        close(server_socket);
        return 1;
    }
    else
    {
        std::cerr << "Connected to Socket. Now collecting data......" << std::endl;
    }

    /// Get the current time
    auto start_time = std::chrono::high_resolution_clock::now();

    // Start threads to collect data
    for (int i = 0; i < NUMBER_OF_THREADS; i++)
    {
        myReaderPool[i]->enqueue(ThreadFunc, server_socket, i, size_to_collect);
    }

    // Wait for threads to finish
    for (int i = 0; i < NUMBER_OF_THREADS; i++)
    {
        delete myReaderPool[i];
    }

    // Close the socket
    close(server_socket);

    // Get the current time again
    auto end_time = std::chrono::high_resolution_clock::now();
    auto duration_seconds = std::chrono::duration_cast<std::chrono::seconds>(end_time - start_time).count();
    // Print the duration in seconds
    std::cout << "Duration: " << duration_seconds << " seconds to finish reading" << std::endl;

    std::cout << "Data collection ended. Now finishing saving of data......" << std::endl;

    // Wait for data writing to be finished
    for (int i = 0; i < NUMBER_OF_THREADS; i++)
    {
        delete myWriterPool[i];
        fclose(FilePool[i]);
    }

    // Calculate how much time taken to save the data
    std::cout << "Saved the data. Now closing the Program. " << std::endl;
    end_time = std::chrono::high_resolution_clock::now();
    duration_seconds = std::chrono::duration_cast<std::chrono::seconds>(end_time - start_time).count();

    // Print the duration in seconds
    std::cout << "Duration: " << duration_seconds << " seconds to complete writing" << std::endl;

    return 0;
}

void WriteToFile(std::vector<char> *buffer, std::vector<char> Header, const int bytes, FILE *fp)
{
    fwrite(Header.data(), Header.size(), 1, fp);
    fwrite(buffer->data(), bytes, 1, fp);
    delete buffer;
}

void ThreadFunc(const int server_fd, const int FileNo, const int SizeToCollect)
{
    long long total_bytes_by_Thread = 0;

    while (total_bytes_by_Thread < SizeToCollect)
    {
        std::vector<char> *buffer = new std::vector<char>(PACKET_SIZE);
        std::vector<char> HEADER_VECTOR{'a', 'b', 'c', 'd'};
        CounterMTX.lock();
        int currentCounter = MESSAGE_COUNTER;
        int bytes = recv(server_fd, buffer->data(), PACKET_SIZE, 0);
        MESSAGE_COUNTER++;
        CounterMTX.unlock();
        std::vector<char> MESSAGE_CNT(sizeof(int));
        intToCharArray(currentCounter, MESSAGE_CNT);
        HEADER_VECTOR.insert(HEADER_VECTOR.end(), MESSAGE_CNT.begin(), MESSAGE_CNT.end());
        myWriterPool[FileNo]
            ->enqueue(WriteToFile, buffer, HEADER_VECTOR, buffer->size(), FilePool[FileNo]);
        total_bytes_by_Thread += bytes;
    }
}

void intToCharArray(const int &value, std::vector<char> &result)
{
    result.resize(sizeof(int) / sizeof(char));
    std::memcpy(result.data(), reinterpret_cast<const char *>(&value), sizeof(int));
}