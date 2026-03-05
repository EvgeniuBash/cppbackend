#include "http_server.h"

#include <boost/asio.hpp>
#include <iostream>

namespace net = boost::asio;
using tcp = net::ip::tcp;

int main(int argc, const char* argv[]) {
    try {
        net::io_context ioc{1};

        tcp::endpoint endpoint{tcp::v4(), 8080};

        auto handler = ...;

        std::cout << "Server started" << std::endl;

        http_server::ServeHttp(ioc, endpoint, handler);

        ioc.run();   // 🔴 ОБЯЗАТЕЛЬНО
    }
    catch (const std::exception& e) {
        std::cerr << e.what() << std::endl;
    }
}
