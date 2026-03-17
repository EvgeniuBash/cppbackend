#pragma once

#include <boost/log/trivial.hpp>
#include <boost/log/attributes/mutable_constant.hpp>
#include <boost/log/utility/manipulators/add_value.hpp>
#include <boost/json.hpp>
#include <string>

namespace logging = boost::log;
namespace json = boost::json;

BOOST_LOG_ATTRIBUTE_KEYWORD(additional_data, "AdditionalData", json::value)

void init_logging();

void log_server_started(int port = 8080, const std::string& address = "0.0.0.0");
void log_server_exited(int code = 0, const std::string& exception = "");
void log_request(const std::string& ip, const std::string& uri, const std::string& method);
void log_response(const std::string& ip, int response_time, int code, const std::string& content_type);
void log_error(int code, const std::string& text, const std::string& where);

template<class Handler>
class LoggingRequestHandler {
public:
    explicit LoggingRequestHandler(Handler& h) : decorated_(h) {}

    template<typename Req, typename Send>
    auto operator()(Req&& req, Send&& send) {
        try {
            std::string ip = req.remote_endpoint().address().to_string();
            std::string uri = std::string(req.target());
            std::string method = req.method_string().to_string();

            log_request(ip, uri, method);

            auto resp = decorated_(std::forward<Req>(req), std::forward<Send>(send));

            int code = resp.result_int();
            std::string content_type;
            if (resp.find(boost::beast::http::field::content_type) != resp.end())
                content_type = resp[boost::beast::http::field::content_type].to_string();
            else
                content_type = "";

            int response_time = 1; // заглушка для теста
            log_response(ip, response_time, code, content_type.empty() ? "" : content_type);

            return resp;
        } catch (const boost::system::system_error& e) {
            log_error(e.code().value(), e.code().message(), "read");
            throw;
        }
    }

private:
    Handler& decorated_;
};