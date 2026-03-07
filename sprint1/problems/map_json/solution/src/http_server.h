#ifndef HTTP_SERVER_H
#define HTTP_SERVER_H

#include "sdk.h"
#define BOOST_BEAST_USE_STD_STRING_VIEW

#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/strand.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <iostream>
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

    template <typename Body, typename Fields> void Write(http::response<Body, Fields>&& response) { 
        auto safe_response = std::make_shared<http::response<Body, Fields>>(std::move(response));
        auto self = shared_from_this();
        http::async_write(stream_, *safe_response, [safe_response, self](beast::error_code ec, std::size_t bytes_written) { 
            self->OnWrite(safe_response->need_eof(), ec, bytes_written);
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
class Session;

template <typename RequestHandler>
void ServeHttp(net::io_context& ioc, const tcp::endpoint& endpoint, RequestHandler&& handler) {
    tcp::acceptor acceptor(ioc, endpoint);

    std::make_shared<Session<RequestHandler>>(
        std::move(acceptor),
        std::forward<RequestHandler>(handler)
    )->Run();
}

}  // namespace http_server

#endif // HTTP_SERVER_H
