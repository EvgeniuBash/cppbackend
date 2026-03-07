#include "http_server.h"
#include "request_handler.h"

#include <boost/asio/dispatch.hpp>
#include <iostream>
#include <chrono>

namespace http_server {

void SessionBase::Run() {
    net::dispatch(stream_.get_executor(),
        beast::bind_front_handler(&SessionBase::Read, shared_from_this()));
}

SessionBase::SessionBase(tcp::socket&& socket)
    : stream_(std::move(socket)) {}

void SessionBase::Read() {
    request_ = {};
    stream_.expires_after(std::chrono::seconds(30));

    http::async_read(stream_, buffer_, request_,
        beast::bind_front_handler(&SessionBase::OnRead, shared_from_this()));
}

void SessionBase::OnRead(beast::error_code ec, std::size_t) {
    if (ec == http::error::end_of_stream)
        return Close();

    if (ec)
        return ReportError(ec, "read");

    HandleRequest(std::move(request_));
}

void SessionBase::OnWrite(bool close, beast::error_code ec, std::size_t) {
    if (ec)
        return ReportError(ec, "write");

    if (close)
        return Close();

    Read();
}

void SessionBase::Close() {
    beast::error_code ec;
    stream_.socket().shutdown(tcp::socket::shutdown_send, ec);

    if (ec) {
        std::cerr << "shutdown: " << ec.message() << std::endl;
    }
}

void SessionBase::ReportError(beast::error_code ec, std::string_view what) {
    std::cerr << what << ": " << ec.message() << std::endl;
}

template <typename RequestHandler>
class HandlerSession : public SessionBase {
public:
    HandlerSession(tcp::socket&& socket, RequestHandler& handler)
        : SessionBase(std::move(socket))
        , handler_(handler) {
    }

private:
    void HandleRequest(HttpRequest&& request) override {
        handler_(std::move(request), [self = this->shared_from_this()](auto&& response) {
            self->Write(std::move(response));
        });
    }

    RequestHandler& handler_;
};

template <typename RequestHandler>
class Session : public SessionBase {
public:
    Session(tcp::acceptor&& acceptor, RequestHandler&& handler)
        : SessionBase(tcp::socket(acceptor.get_executor()))
        , acceptor_(std::move(acceptor))
        , handler_(std::make_shared<RequestHandler>(std::move(handler))) {
    }

    void Run() {
        acceptor_.async_accept(
            net::make_strand(acceptor_.get_executor()),
            [self = std::static_pointer_cast<Session<RequestHandler>>(this->shared_from_this())]
            (beast::error_code ec, tcp::socket socket) {
                self->OnAccept(ec, std::move(socket));
            }
        );
    }

private:
    void OnAccept(beast::error_code ec, tcp::socket socket) {
        if (!ec) {
            std::make_shared<HandlerSession<RequestHandler>>(std::move(socket), *handler_)->Run();
        }
        Run();
    }

    void HandleRequest(HttpRequest&& request) override {
        std::cerr << "Error: Session::HandleRequest should never be called" << std::endl;
    }

    tcp::acceptor acceptor_;
    RequestHandler handler_;
};


template void ServeHttp<http_handler::RequestHandler>(
    net::io_context&, const tcp::endpoint&, http_handler::RequestHandler&&);

} // namespace http_server