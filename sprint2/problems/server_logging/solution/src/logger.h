#pragma once

#include <boost/log/trivial.hpp>
#include <boost/log/core.hpp>
#include <boost/log/expressions.hpp>
#include <boost/log/utility/setup/console.hpp>
#include <boost/log/attributes/current_thread_id.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/core.hpp>
#include <boost/json.hpp>
#include <chrono>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>
#include <utility>

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

template <typename Handler>
class LoggingRequestHandler {
public:
    explicit LoggingRequestHandler(Handler& handler) : handler_(handler) {}

    template <typename Req, typename Send>
    auto operator()(Req&& req, Send&& send) {
        try {
            std::string ip = "127.0.0.1"; 
            std::string uri{req.target().data(), req.target().size()};
            std::string method{req.method_string().data(), req.method_string().size()};

            log_request(ip, uri, method);

            auto response = handler_(std::forward<Req>(req), std::forward<Send>(send));

            std::string content_type = "";
            if (!response[boost::beast::http::field::content_type].empty())
                content_type = std::string(
                    response[boost::beast::http::field::content_type].data(),
                    response[boost::beast::http::field::content_type].size()
                );

            log_response(ip,
                         0,
                         response.result_int(),
                         content_type);

            return response;
        } catch (const std::exception& e) {
            log_error(1, e.what(), "handler");
            throw;
        }
    }

private:
    Handler& handler_;
};