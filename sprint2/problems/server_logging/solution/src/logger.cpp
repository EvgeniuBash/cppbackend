#include "logger.h"
#include <boost/log/core.hpp>
#include <boost/log/expressions.hpp>
#include <boost/log/utility/setup/console.hpp>
#include <boost/log/attributes/current_thread_id.hpp>
#include <boost/log/attributes/scoped_attribute.hpp>
#include <boost/json.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>

namespace logging = boost::log;
namespace attrs = boost::log::attributes;
namespace json = boost::json;

void init_logging() {
    logging::add_console_log(
        std::cout,
        logging::keywords::format = [](auto const& record, auto& stream) {
            json::object obj;
            obj["timestamp"] = boost::posix_time::to_iso_extended_string(
                boost::posix_time::microsec_clock::local_time()
            );
            obj["message"] = record[logging::trivial::message];

            auto attr = record[additional_data];
            obj["data"] = attr ? attr.get() : json::object{};

            stream << json::serialize(obj);
        }
    );

    logging::core::get()->add_global_attribute("ThreadID", attrs::current_thread_id());
}

void log_server_started(int port, const std::string& address) {
    json::object data{{"port", port}, {"address", address}};
    BOOST_LOG_TRIVIAL(info) << logging::add_value(additional_data, data) << "server started";
}

void log_server_exited(int code, const std::string& exception) {
    json::object data{{"code", code}};
    if (!exception.empty()) data["exception"] = exception;
    BOOST_LOG_TRIVIAL(info) << logging::add_value(additional_data, data) << "server exited";
}

void log_request(const std::string& ip, const std::string& uri, const std::string& method) {
    json::object data{{"ip", ip}, {"URI", uri}, {"method", method}};
    BOOST_LOG_TRIVIAL(info) << logging::add_value(additional_data, data) << "request received";
}

void log_response(const std::string& ip, int response_time, int code, const std::string& content_type) {
    json::object data{
        {"ip", ip},
        {"response_time", response_time},
        {"code", code},
        {"content_type", content_type.empty() ? json::value(nullptr) : content_type}
    };
    BOOST_LOG_TRIVIAL(info) << logging::add_value(additional_data, data) << "response sent";
}

void log_error(int code, const std::string& text, const std::string& where) {
    json::object data{{"code", code}, {"text", text}, {"where", where}};
    BOOST_LOG_TRIVIAL(error) << logging::add_value(additional_data, data) << "error";
}