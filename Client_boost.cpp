#include <iostream>
#include <fstream>
#include <chrono>
#include <csignal>
#include <filesystem>
#include <boost/asio.hpp>

using boost::asio::ip::udp;

volatile sig_atomic_t g_running = true;
const int ONE_GB = 1073741824;

void signal_handler(int signal)
{
    if (signal == SIGINT)
    {
        g_running = false;
    }
}

int main(int argc, char *argv[])
{

    // Set up the signal handler
    std::signal(SIGINT, signal_handler);

    if (argc < 3)
    {
        std::cerr << "Usage: " << argv[0] << " <output_file> <listen_ip> <listen_port>" << std::endl;
        return 1;
    }

    const std::string output_file = argv[1];
    const std::string listen_ip = argv[2];
    const std::string listen_port = argv[3];

    boost::asio::io_service io_service;
    udp::socket socket(io_service);
    udp::endpoint listen_endpoint(boost::asio::ip::address::from_string(listen_ip), std::stoi(listen_port));
    socket.open(listen_endpoint.protocol());
    socket.bind(listen_endpoint);

    std::ofstream outfile(output_file, std::ios::out | std::ios::binary);
    if (!outfile.is_open())
    {
        std::cerr << "Unable to open output file: " << output_file << std::endl;
        return 1;
    }

    std::cout << "Listening on " << listen_ip << ":" << listen_port << std::endl;
    std::cout << "Saving incoming traffic to " << output_file << std::endl;

    /// Get the current time
    auto start_time = std::chrono::high_resolution_clock::now();

    try
    {
        while (g_running)
        {
            char data[8200];
            udp::endpoint sender_endpoint;
            size_t length = socket.receive_from(boost::asio::buffer(data, sizeof(data)), sender_endpoint);
            outfile.write(data, length);
        }

        // check if the file exists
        if (!std::filesystem::exists(output_file))
        {
            std::cerr << "File not found." << std::endl;
            return 1;
        }

        // open the file
        std::ifstream file(output_file, std::ios::binary);

        // get the file size
        std::streampos file_size = 0;
        if (file.seekg(0, std::ios::end))
        {
            file_size = file.tellg();
        }

        std::cout << "File size: " << static_cast<float>(file_size) / ONE_GB << " bytes" << std::endl;

        // close the file
        file.close();
        outfile.close();

        // Get the current time again
        auto end_time = std::chrono::high_resolution_clock::now();
        auto duration_seconds = std::chrono::duration_cast<std::chrono::seconds>(end_time - start_time).count();
        // Print the duration in seconds
        std::cout << "Duration: " << duration_seconds << " seconds to finish reading" << std::endl;
        std::cout << "Speed was: " << static_cast<float>(file_size) / (duration_seconds * ONE_GB) << " GB per second!" << std::endl;
    }
    catch (const std::exception &e)
    {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}
