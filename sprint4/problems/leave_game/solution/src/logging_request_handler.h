#pragma once

#include "logger.h"

#include <boost/beast/http.hpp>

#include <chrono>
#include <string>
#include <utility>

namespace logging = boost::log;
namespace json = boost::json;
namespace http = boost::beast::http;

template <typename Handler>
class LoggingRequestHandler {
public:
    explicit LoggingRequestHandler(Handler& handler)
        : handler_(handler) {
    }

    template <typename Request, typename Send>
    void operator()(Request&& req, Send&& send) {
        auto start = std::chrono::steady_clock::now();

        const auto target = req.target();
        const auto method = req.method_string();

        json::object req_data{
            {"ip", "unknown"},
            {"URI", std::string(target.data(), target.size())},
            {"method", std::string(method.data(), method.size())}
        };

        BOOST_LOG_TRIVIAL(info)
            << boost::log::add_value(additional_data, req_data)
            << "request received";

        handler_(
            std::forward<Request>(req),
            [start, send = std::forward<Send>(send)](auto&& response) mutable {
                auto end = std::chrono::steady_clock::now();

                auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(
                    end - start
                ).count();

                json::object resp_data{
                    {"response_time", duration},
                    {"code", response.result_int()}
                };

                auto content_type_it = response.base().find(http::field::content_type);

                if (content_type_it != response.base().end()) {
                    const auto value = response.base()[http::field::content_type];
                    resp_data["content_type"] = std::string(value.data(), value.size());
                } else {
                    resp_data["content_type"] = nullptr;
                }

                BOOST_LOG_TRIVIAL(info)
                    << boost::log::add_value(additional_data, resp_data)
                    << "response sent";

                send(std::forward<decltype(response)>(response));
            }
        );
    }

private:
    Handler& handler_;
};