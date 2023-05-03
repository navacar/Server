#include <iostream>
#include <boost/asio.hpp>
#include <boost/archive/binary_iarchive.hpp>
#include <boost/archive/binary_oarchive.hpp>
#include <boost/serialization/vector.hpp>
#include <filesystem>
#include <map>
#include <thread>
#include <mutex>
using boost::asio::ip::tcp;

class Server {
public:
    Server(boost::asio::io_service& io_service, short port)
        : io_service_(io_service), acceptor_(io_service, tcp::endpoint(tcp::v4(), port))
    {
        start_accept();
    }

private:
    
    void start_accept()
    {
        while(true){
            // Получаем запрос, делаем отдельный поток для его обработки
            auto socket = std::make_shared<tcp::socket>(acceptor_.get_executor());
            acceptor_.accept(*socket);
            std::thread([this, socket] {
                handle_request(socket);
            }).detach();
        }
    }

    // Метод для обработки запроса от клиента
    void handle_request(std::shared_ptr<tcp::socket> socket)
    {
        try {
            // Читаем тело запроса
            boost::asio::streambuf buf;
            boost::asio::read_until(*socket, buf, '\n');

            std::string request;
            std::istream response_stream(&buf);
            std::getline(response_stream,request);

            std::vector<std::string> files;

            // Проверяем, есть ли ответ в кэше
            {
                std::lock_guard<std::mutex> lock(cache_mutex_);

                auto it = cache_.find(request);
                if (it != cache_.end()) {
                    files = it->second;
                }
            }

            // Если ответа в кэше нет, то получаем список файлов из директории
            if (files.empty()) {
                get_directory_contents(files, request);
                // Добавляем ответ в кэш
                std::lock_guard<std::mutex> lock(cache_mutex_);
                cache_[request] = files;
            }

            // Отправляем ответ клиенту
            boost::asio::streambuf buffer;
            std::ostream os(&buffer);
            boost::archive::binary_oarchive oarchive(os);
            oarchive << files;
            boost::asio::write(*socket, buffer);
        } catch (std::exception& e) {
            std::cerr << "Exception in thread: " << e.what() << std::endl;
        }
    }

    // Метод для получения списка файлов в директории
    void get_directory_contents(std::vector<std::string> &files, const std::string& path) {
        try {
            for (const auto& entry : std::filesystem::directory_iterator(path)) {
                files.emplace_back(entry.path().filename().string());
            }
        } catch (std::filesystem::filesystem_error& ex) {
            std::cerr << "Error while getting directory contents: " << ex.what() << std::endl;
        }
    }

private:
    boost::asio::io_service& io_service_;
    tcp::acceptor acceptor_;

    // Кэш для хранения ответов по недавно запрошенным путям
    // Так как в задании не было условия очищать кеш, я его не очищаю
    std::map<std::string, std::vector<std::string>> cache_;
    std::mutex cache_mutex_;
};

int main(int argc, char* argv[])
{
    if (argc != 2) {
        std::cerr << "Usage: server <port>\n";
        return 1;
    }

    try {
        boost::asio::io_service io_service;
        Server server(io_service, std::atoi(argv[1]));
        io_service.run();
    } catch (std::exception& e) {
        std::cerr << "Exeption: " << e.what() << std::endl;
    }

    return 0;
}