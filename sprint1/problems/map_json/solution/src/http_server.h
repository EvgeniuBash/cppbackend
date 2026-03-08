#pragma once

#include "sdk.h"

#define BOOST_BEAST_USE_STD_STRING_VIEW

#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/strand.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>

#include <memory>
#include <iostream>

namespace http_server {

namespace net = boost::asio;
using tcp = net::ip::tcp;
namespace beast = boost::beast;
namespace http = beast::http;

class SessionBase : public std::enable_shared_from_this<SessionBase> {
public:
    SessionBase(const SessionBase&) = delete;
    SessionBase& operator=(const SessionBase&) = delete;

    void Run();
    virtual ~SessionBase();

protected:
    using HttpRequest = http::request<http::string_body>;

    explicit SessionBase(tcp::socket&& socket);

    template <typename Body, typename Fields>
    void Write(http::response<Body, Fields>&& response) {
        auto safe_response =
            std::make_shared<http::response<Body, Fields>>(std::move(response));

        auto self = shared_from_this();

        http::async_write(
            stream_,
            *safe_response,
            [safe_response, self](beast::error_code ec, std::size_t bytes_written) {
                self->OnWrite(safe_response->need_eof(), ec, bytes_written);
            });
    }

private:
    void Read();
    void OnRead(beast::error_code ec, std::size_t bytes_read);
    void Close();
    void OnWrite(bool close, beast::error_code ec, std::size_t bytes_written);
    void ReportError(beast::error_code ec, std::string_view what);

    virtual void HandleRequest(HttpRequest&& request) = 0;

    beast::tcp_stream stream_;
    beast::flat_buffer buffer_;
    HttpRequest request_;
};

template <typename RequestHandler>
class Session : public SessionBase {
public:
    template <typename Handler>
    Session(tcp::socket&& socket, Handler&& request_handler)
        : SessionBase(std::move(socket))
        , request_handler_(std::forward<Handler>(request_handler)) {}

private:
    void HandleRequest(HttpRequest&& request) override {
        request_handler_(
            std::move(request),
            [self = this->shared_from_this()](auto&& response) {
                self->Write(std::move(response));
            });
    }

    RequestHandler request_handler_;
};

template <typename RequestHandler>
class Listener : public std::enable_shared_from_this<Listener<RequestHandler>> {
public:
    template <typename Handler>
    Listener(net::io_context& ioc, const tcp::endpoint& endpoint, Handler&& request_handler);

    void Run();

private:
    void DoAccept();
    void OnAccept(beast::error_code ec, tcp::socket socket);
    void AsyncRunSession(tcp::socket&& socket);

    net::io_context& ioc_;
    tcp::acceptor acceptor_;
    RequestHandler request_handler_;
};

template <typename RequestHandler>
void ServeHttp(net::io_context& ioc, const tcp::endpoint& endpoint, RequestHandler&& handler) {
    using MyListener = Listener<std::decay_t<RequestHandler>>;

    std::make_shared<MyListener>(
        ioc,
        endpoint,
        std::forward<RequestHandler>(handler)
    )->Run();
}

} 
