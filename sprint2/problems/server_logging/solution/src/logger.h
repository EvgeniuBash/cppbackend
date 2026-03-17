#pragma once

#include <boost/log/trivial.hpp>
#include <boost/log/utility/setup/console.hpp>
#include <boost/log/attributes/current_thread_id.hpp>
#include <boost/log/attributes/local_clock.hpp>
#include <boost/log/expressions.hpp>
#include <boost/json.hpp>
#include <boost/asio.hpp>
#include <boost/beast/http.hpp>
#include <string>

namespace logging = boost::log;
namespace attrs = boost::log::attributes;
namespace expr = boost::log::expressions;
namespace json = boost::json;

BOOST_LOG_ATTRIBUTE_KEYWORD(additional_data, "AdditionalData", json::value)

void init_logging();

void log_server_started(int port, const std::string& address);
void log_server_exited(int code, const std::string& exception = "");

void log_request(const std::string& ip, const std::string& uri, const std::string& method);
void log_response(const std::string& ip, int response_time, int code, const std::string& content_type);

void log_error(int code, const std::string& text, const std::string& where);

template<class Handler>
class LoggingRequestHandler {
public:
    explicit LoggingRequestHandler(Handler& handler) : decorated_(handler) {}

    template<typename Req, typename Send>
    auto operator()(Req&& req, Send&& send) {
        std::string ip = "127.0.0.1"; // TODO: при наличии endpoint извлекать реальный IP
        std::string uri = req.target().to_string();
        std::string method = req.method_string().to_string();

        log_request(ip, uri, method);

        auto response = decorated_(std::forward<Req>(req), std::forward<Send>(send));

        std::string content_type;
        if (response.find(boost::beast::http::field::content_type) != response.end())
            content_type = response[boost::beast::http::field::content_type].to_string();

        log_response(ip, 0, response.result_int(), content_type);

        return response;
    }

private:
    Handler& decorated_;
};