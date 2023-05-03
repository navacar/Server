#include <iostream>
#include <string>
#include <vector>
#include <boost/asio.hpp>
#include <boost/archive/binary_iarchive.hpp>
#include <boost/serialization/vector.hpp>

using boost::asio::ip::tcp;

int main(int argc, char* argv[]) {
    try {
        if (argc != 4) {
            std::cerr << "Usage: client <host> <port> <path>\n";
            return 1;
        }

        boost::asio::io_context io_context;
        tcp::resolver resolver(io_context);
        tcp::resolver::results_type endpoints = resolver.resolve(tcp::v4(), argv[1], argv[2]);

        tcp::socket socket(io_context);
        boost::asio::connect(socket, endpoints);

        std::string request = argv[3];
        request += "\n";
        boost::asio::write(socket, boost::asio::buffer(request));

        boost::asio::streambuf buf;
        boost::asio::read_until(socket, buf, '\0');

        std::istream is(&buf);
        std::vector<std::string> vec;
        boost::archive::binary_iarchive ia(is);
        ia >> vec;

        for (const auto& str : vec)
        {
            std::cout << str << " ";
        }
        std::cout << std::endl;
    } catch (std::exception& e) {
        std::cerr << "Exception: " << e.what() << "\n";
    }

    return 0;
}