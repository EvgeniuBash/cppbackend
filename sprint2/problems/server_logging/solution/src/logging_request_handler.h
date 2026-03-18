#pragma once

#include <boost/log/trivial.hpp>
#include <boost/json.hpp>
#include <chrono>

namespace logging = boost::log;
namespace json = boost::json;

template <typename Handler>
class LoggingRequestHandler {
public:
    explicit LoggingRequestHandler(Handler& handler)
        : handler_(handler) {}

    template <typename Request, typename Send>
    void operator()(Request&& req, Send&& send, std::string ip) {

        auto start = std::chrono::steady_clock::now();

        json::object req_data{
            {"ip", ip},
            {"URI", std::string(req.target())},
            {"method", std::string(req.method_string())}
        };

        BOOST_LOG_TRIVIAL(info)
            << logging::add_value(additional_data, req_data)
            << "request received";

        handler_(
            std::forward<Request>(req),
            [start, send = std::forward<Send>(send), ip](auto&& response) mutable {

                auto end = std::chrono::steady_clock::now();
                auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();

                json::object resp_data{
                    {"response_time", duration},
                    {"code", response.result_int()}
                };

                if (response.base().find(boost::beast::http::field::content_type) != response.base().end()) {
                    resp_data["content_type"] =
                        response.base()[boost::beast::http::field::content_type].to_string();
                } else {
                    resp_data["content_type"] = nullptr;
                }

                BOOST_LOG_TRIVIAL(info)
                    << logging::add_value(additional_data, resp_data)
                    << "response sent";

                send(std::forward<decltype(response)>(response));
            }
        );
    }

private:
    Handler& handler_;
};