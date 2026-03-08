#include "http_server.h"

namespace http_server {

SessionBase::SessionBase(tcp::socket&& socket)
    : stream_(std::move(socket)) {}

SessionBase::~SessionBase() = default;

void SessionBase::Run() {
    net::dispatch(stream_.get_executor(),
        beast::bind_front_handler(&SessionBase::Read, shared_from_this()));
}

void SessionBase::Read() {
    request_ = {};
    stream_.expires_after(std::chrono::seconds(30));

    http::async_read(stream_, buffer_, request_,
        beast::bind_front_handler(&SessionBase::OnRead, shared_from_this()));
}

void SessionBase::OnRead(beast::error_code ec, std::size_t) {
    if (ec == http::error::end_of_stream) {
        return Close();
    }

    if (ec) {
        return ReportError(ec, "read");
    }

    HandleRequest(std::move(request_));
}

void SessionBase::Close() {
    stream_.socket().shutdown(tcp::socket::shutdown_send);
}

void SessionBase::OnWrite(bool close, beast::error_code ec, std::size_t) {
    if (ec) {
        return ReportError(ec, "write");
    }

    if (close) {
        return Close();
    }

    Read();
}

void SessionBase::ReportError(beast::error_code ec, std::string_view what) {
    std::cerr << what << ": " << ec.message() << std::endl;
}

}
