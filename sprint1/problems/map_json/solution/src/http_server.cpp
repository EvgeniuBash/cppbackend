#include "http_server.h"

namespace http_server {

SessionBase::SessionBase(tcp::socket&& socket)
    : stream_(std::move(socket)) {}

SessionBase::~SessionBase() = default;

void SessionBase::Run() {
    net::dispatch(
        stream_.get_executor(),
        beast::bind_front_handler(
            &SessionBase::Read,
            shared_from_this()
        )
    );
}

void SessionBase::Read() {
    request_ = {};

    stream_.expires_after(std::chrono::seconds(30));

    http::async_read(
        stream_,
        buffer_,
        request_,
        beast::bind_front_handler(
            &SessionBase::OnRead,
            shared_from_this()
        )
    );
}

void SessionBase::OnRead(beast::error_code ec, std::size_t) {
    if (ec == http::error::end_of_stream)
        return Close();

    if (ec)
        return ReportError(ec, "read");

    HandleRequest(std::move(request_));
}

void SessionBase::Close() {
    stream_.socket().shutdown(tcp::socket::shutdown_send);
}

void SessionBase::OnWrite(bool close, beast::error_code ec, std::size_t) {
    if (ec)
        return ReportError(ec, "write");

    if (close)
        return Close();

    Read();
}

void SessionBase::ReportError(beast::error_code ec, std::string_view what) {
    std::cerr << what << ": " << ec.message() << std::endl;
}

template <typename RequestHandler>
Listener<RequestHandler>::Listener(
    net::io_context& ioc,
    const tcp::endpoint& endpoint,
    RequestHandler&& request_handler)
    : ioc_(ioc)
    , acceptor_(net::make_strand(ioc))
    , request_handler_(std::forward<RequestHandler>(request_handler)) {

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

template <typename RequestHandler>
void Listener<RequestHandler>::Run() {
    DoAccept();
}

template <typename RequestHandler>
void Listener<RequestHandler>::DoAccept() {
    acceptor_.async_accept(
        net::make_strand(ioc_),
        beast::bind_front_handler(
            &Listener::OnAccept,
            this->shared_from_this()
        )
    );
}

template <typename RequestHandler>
void Listener<RequestHandler>::OnAccept(beast::error_code ec, tcp::socket socket) {
    if (ec) {
        std::cerr << "Accept error: " << ec.message() << std::endl;
        return;
    }

    AsyncRunSession(std::move(socket));
    DoAccept();
}

template <typename RequestHandler>
void Listener<RequestHandler>::AsyncRunSession(tcp::socket&& socket) {
    std::make_shared<Session<RequestHandler>>(
        std::move(socket),
        request_handler_
    )->Run();
}

}