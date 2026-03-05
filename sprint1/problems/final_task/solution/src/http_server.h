#pragma once

#include <boost/asio.hpp>
#include <boost/beast.hpp>
#include <memory>
#include <string>
#include <utility>

namespace net = boost::asio;
namespace beast = boost::beast;
namespace http = beast::http;
using tcp = net::ip::tcp;

namespace http_server {

using HttpRequest = http::request<http::string_body>;

class SessionBase : public std::enable_shared_from_this<SessionBase> {
public:
    explicit SessionBase(tcp::socket&& socket)
        : socket_(std::move(socket)) {
    }

    void Run() {
        Read();
    }

    template <typename Response>
    void Write(Response&& response) {
    auto self = shared_from_this();

    auto resp = std::make_shared<std::decay_t<Response>>(std::forward<Response>(response));

    http::async_write(socket_, *resp,
        [self, resp](beast::error_code ec, std::size_t) {
            self->socket_.shutdown(tcp::socket::shutdown_send, ec);
        });
    }

protected:
    virtual void HandleRequest(HttpRequest&& request) = 0;

private:
    void Read() {
        auto self = shared_from_this();

        http::async_read(socket_, buffer_, request_,
            [self](beast::error_code ec, std::size_t) {
                if (!ec) {
                    self->HandleRequest(std::move(self->request_));
                }
            });
    }

    tcp::socket socket_;
    beast::flat_buffer buffer_;
    HttpRequest request_;
};

template <typename RequestHandler>
class HttpSession : public SessionBase {
public:
    HttpSession(tcp::socket&& socket, RequestHandler& handler)
        : SessionBase(std::move(socket))
        , handler_(handler) {
    }

private:
    void HandleRequest(HttpRequest&& req) override {
        handler_(std::move(req),
            [self = this->shared_from_this()](auto&& response) {
                self->Write(std::forward<decltype(response)>(response));
            });
    }

    RequestHandler& handler_;
};

template <typename RequestHandler>
void ServeHttp(net::io_context& ioc,
               const tcp::endpoint& endpoint,
               RequestHandler&& handler) {

    auto acceptor = std::make_shared<tcp::acceptor>(ioc);

    beast::error_code ec;

    acceptor->open(endpoint.protocol(), ec);
    acceptor->set_option(net::socket_base::reuse_address(true), ec);
    acceptor->bind(endpoint, ec);
    acceptor->listen(net::socket_base::max_listen_connections, ec);

    std::function<void()> do_accept;

    do_accept = [acceptor, &ioc, &handler, &do_accept]() {
        acceptor->async_accept(
            [&ioc, &handler, acceptor, &do_accept](beast::error_code ec, tcp::socket socket) {

                if (!ec) {
                    std::make_shared<HttpSession<RequestHandler>>(
                        std::move(socket),
                        handler
                    )->Run();
                }

                do_accept();
            });
    };

    do_accept();
}

} // namespace http_server
