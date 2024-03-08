#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <asio.hpp>

using namespace std;
using namespace boost::asio;
using namespace boost::asio::ip;

class HttpRequestHandler {
public:
    static string generate_response(const string& request) {
        string response = "HTTP/1.1 200 OK\r\n";
        response += "Content-Type: text/html\r\n\r\n";
        response += "<html><head><title>Server</title></head><body>";
        response += "<h1>Answer Responce Responce</h1>";
        response += "</body></html>";
        return response;
    }

    static string get_file_content(const string& filename) {
        ifstream file(filename.c_str(), ios::in | ios::binary);
        if (file) {
            ostringstream content;
            content << file.rdbuf();
            file.close();
            return content.str();
        }
        return "";
    }

    static string generate_file_response(const string& filename) {
        string response = "HTTP/1.1 200 OK\r\n";
        response += "Content-Type: text/html\r\n\r\n";
        response += get_file_content(filename);
        return response;
    }

    static string generate_not_found_response() {
        string response = "HTTP/1.1 404 Not Found\r\n";
        response += "Content-Type: text/html\r\n\r\n";
        response += "<html><head><title>404 Not Found</title></head><body>";
        response += "<h1>404 Not Found</h1>";
        response += "</body></html>";
        return response;
    }
};

class HttpServer {
public:
    HttpServer(io_service& io_service, short port)
        : acceptor_(io_service, tcp::endpoint(tcp::v4(), port)),
        socket_(io_service) {
        start_accept();
    }

private:
    void start_accept() {
        acceptor_.async_accept(socket_, [this](boost::system::error_code ec) {
            if (!ec) {
                handle_request();
            }
            start_accept();
            });
    }

    void handle_request() {
        async_read_until(socket_, request_, "\r\n\r\n",
            [this](boost::system::error_code ec, std::size_t bytes_transferred) {
                if (!ec) {
                    std::istream request_stream(&request_);
                    std::string request;
                    getline(request_stream, request);

                    if (request.find("GET") != std::string::npos) {
                        size_t start = request.find_first_of("/") + 1;
                        size_t end = request.find_first_of(" ", start);
                        std::string filename = request.substr(start, end - start);

                        if (filename.empty() || filename == "index.html") {
                            write_response(HttpRequestHandler::generate_response(request));
                        }
                        else {
                            write_response(HttpRequestHandler::generate_file_response(filename));
                        }
                    }
                    else {
                        write_response(HttpRequestHandler::generate_not_found_response());
                    }
                }
            }
        );
    }

    void write_response(const string& response) {
        async_write(socket_, buffer(response),
            [this](boost::system::error_code ec, std::size_t bytes_transferred) {
                if (!ec) {
                    socket_.close();
                }
            }
        );
    }

    tcp::acceptor acceptor_;
    tcp::socket socket_;
    streambuf request_;
};

int main() {
    try {
        io_service io_service;
        HttpServer server(io_service, 8080);
        io_service.run();
    }
    catch (const std::exception& e) {
        cerr << "Exception: " << e.what() << endl;
    }

    return 0;
}
