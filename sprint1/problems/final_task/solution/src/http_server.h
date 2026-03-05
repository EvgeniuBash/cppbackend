#pragma once
#include "sdk.h"
#define BOOST_BEAST_USE_STD_STRING_VIEW

#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/strand.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <memory>

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

    template <typename Body, typename Fields>
    void Write(http::response<Body, Fields>&& response) {
        auto safe_response =
            std::make_shared<http::response<Body, Fields>>(std::move(response));

        auto self = shared_from_this();

        http::async_write(
            stream_,
            *safe_response,
            [safe_response, self](beast::error_code ec, std::size_t bytes) {
                self->OnWrite(safe_response->need_eof(), ec, bytes);
            });
    }

protected:
    using HttpRequest = http::request<http::string_body>;

    explicit SessionBase(tcp::socket&& socket);
    virtual ~SessionBase() = default;

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
    Session(tcp::socket&& socket, RequestHandler& handler)
        : SessionBase(std::move(socket))
        , handler_(handler) {}

private:
    void HandleRequest(HttpRequest&& req) override {
        handler_(
            std::move(req),
            [self = this->shared_from_this()](auto&& response) {
                static_cast<SessionBase&>(*self).Write(std::move(response));
            });
    }

    RequestHandler& handler_;
};

template <typename RequestHandler>
void ServeHttp(net::io_context& ioc,
               tcp::endpoint endpoint,
               RequestHandler&& handler) {

    tcp::acceptor acceptor{ioc};

    beast::error_code ec;

    acceptor.open(endpoint.protocol(), ec);
    acceptor.set_option(net::socket_base::reuse_address(true), ec);
    acceptor.bind(endpoint, ec);
    acceptor.listen(net::socket_base::max_listen_connections, ec);

    for (;;) {
        tcp::socket socket{ioc};
        acceptor.accept(socket);

        std::make_shared<Session<RequestHandler>>(
            std::move(socket),
            handler
        )->Run();
    }
}

}  // namespace http_server
