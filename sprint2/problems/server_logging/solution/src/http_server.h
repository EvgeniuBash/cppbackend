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

    using HttpRequest = http::request<http::string_body>;

    explicit SessionBase(tcp::socket&& socket);
    virtual ~SessionBase();

    void Run();

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

protected:
    virtual void HandleRequest(HttpRequest&& request) = 0;

    beast::tcp_stream stream_;

private:
    void Read();
    void OnRead(beast::error_code ec, std::size_t bytes_read);
    void Close();
    void OnWrite(bool close, beast::error_code ec, std::size_t bytes_written);
    void ReportError(beast::error_code ec, std::string_view what);

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
        auto ip = stream_.socket().remote_endpoint().address().to_string();

        request_handler_(
            std::move(request),
            [self = this->shared_from_this()](auto&& response) {
                self->Write(std::move(response));
            },
            ip
        );
    }

    RequestHandler request_handler_;
};

template <typename RequestHandler>
class Listener : public std::enable_shared_from_this<Listener<RequestHandler>> {
public:
    template <typename Handler>
    Listener(net::io_context& ioc, const tcp::endpoint& endpoint, Handler&& request_handler)
        : ioc_(ioc)
        , acceptor_(net::make_strand(ioc))
        , request_handler_(std::forward<Handler>(request_handler)) {

        beast::error_code ec;

        acceptor_.open(endpoint.protocol(), ec);
        if (ec)
            throw std::runtime_error("Failed to open acceptor");

        acceptor_.set_option(net::socket_base::reuse_address(true), ec);
        if (ec)
            throw std::runtime_error("Failed to set option");

        acceptor_.bind(endpoint, ec);
        if (ec)
            throw std::runtime_error("Failed to bind");

        acceptor_.listen(net::socket_base::max_listen_connections, ec);
        if (ec)
            throw std::runtime_error("Failed to listen");
    }

    void Run() {
        DoAccept();
    }

private:
    void DoAccept() {
        acceptor_.async_accept(
            net::make_strand(ioc_),
            beast::bind_front_handler(
                &Listener::OnAccept,
                this->shared_from_this()
            )
        );
    }

    void OnAccept(beast::error_code ec, tcp::socket socket) {
        if (ec) {
            BOOST_LOG_TRIVIAL(error)
                << logging::add_value(
                       boost::log::attributes::named_scope::value_type::value_type("AdditionalData"),
                       json::object{
                           {"code", ec.value()},
                           {"text", ec.message()},
                           {"where", "accept"}
                       })
                << "error";
            return;
        }

        AsyncRunSession(std::move(socket));
        DoAccept();
    }

    void AsyncRunSession(tcp::socket&& socket) {
        std::make_shared<Session<RequestHandler>>(
            std::move(socket),
            request_handler_
        )->Run();
    }

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