#include "http_server.h"

#include <boost/asio.hpp>
#include <iostream>

namespace net = boost::asio;
using tcp = net::ip::tcp;

int main() {
    try {
        net::io_context ioc{1};

        tcp::endpoint endpoint{tcp::v4(), 8080};

        auto handler = [](auto&& req, auto&& send) {
            http::response<http::string_body> res;
            res.version(req.version());
            res.result(http::status::ok);
            res.set(http::field::content_type, "text/plain");
            res.body() = "Hello";
            res.prepare_payload();

            send(std::move(res));
        };

        std::cout << "Server started" << std::endl;

        http_server::ServeHttp(ioc, endpoint, handler);
    }
    catch (std::exception const& e) {
        std::cerr << e.what() << std::endl;
    }
}
