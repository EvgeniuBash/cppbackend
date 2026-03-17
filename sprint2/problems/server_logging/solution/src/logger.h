#pragma once

#include <boost/log/trivial.hpp>
#include <boost/log/expressions.hpp>
#include <boost/log/core.hpp>
#include <boost/log/utility/setup/console.hpp>
#include <boost/log/attributes/scoped_attribute.hpp>
#include <boost/log/attributes/current_thread_id.hpp>
#include <boost/json.hpp>
#include <chrono>
#include <iomanip>
#include <sstream>
#include <string>
#include <string_view>

namespace logging = boost::log;
namespace attrs = boost::log::attributes;
namespace json = boost::json;

BOOST_LOG_ATTRIBUTE_KEYWORD(additional_data, "AdditionalData", json::value)

inline void init_logging() {
    logging::add_console_log(
        std::cout,
        logging::keywords::format = [](auto const& record, auto& stream) {
            const auto& json_data = record[additional_data];

            json::object obj;
            obj["timestamp"] = boost::posix_time::to_iso_extended_string(
                boost::posix_time::microsec_clock::local_time()
            );
            obj["message"] = record[logging::trivial::message];

            if (json_data) {
                obj["data"] = json::value_cast<json::value>(json_data.get());
            } else {
                obj["data"] = json::object{};
            }

            stream << json::serialize(obj);
        }
    );
    logging::core::get()->add_global_attribute("ThreadID", attrs::current_thread_id());
}

template <typename Handler>
class LoggingRequestHandler {
public:
    explicit LoggingRequestHandler(Handler& handler) : decorated_(handler) {}

    template <typename Req, typename Send>
    void operator()(Req&& req, Send&& send) {
        try 
            json::object req_data;
            req_data["ip"] = "127.0.0.1"; 
            req_data["URI"] = std::string(req.target());
            req_data["method"] = std::string(req.method_string());

            BOOST_LOG_TRIVIAL(info)
                << logging::add_value(additional_data, req_data)
                << "request received";

            auto start = std::chrono::steady_clock::now();

            auto logging_send = [&](auto&& response) {
                auto end = std::chrono::steady_clock::now();
                auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();

                json::object resp_data;
                resp_data["response_time"] = static_cast<int>(ms);
                resp_data["code"] = static_cast<int>(response.result_int());

                if (response.find(boost::beast::http::field::content_type) != response.end()) {
                    resp_data["content_type"] = response[boost::beast::http::field::content_type].to_string();
                } else {
                    resp_data["content_type"] = nullptr;
                }

                BOOST_LOG_TRIVIAL(info)
                    << logging::add_value(additional_data, resp_data)
                    << "response sent";

                send(std::forward<decltype(response)>(response));
            };

            decorated_(std::forward<Req>(req), logging_send);

        } catch (const std::exception& e) {
            json::object err;
            err["code"] = -1;
            err["text"] = e.what();
            err["where"] = "handler";

            BOOST_LOG_TRIVIAL(error)
                << logging::add_value(additional_data, err)
                << "error";

            throw;
        }
    }

private:
    Handler& decorated_;
};