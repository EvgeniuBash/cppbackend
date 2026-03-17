#include "logger.h"
#include <boost/log/core.hpp>
#include <boost/log/expressions.hpp>
#include <boost/log/utility/setup/console.hpp>
#include <boost/log/attributes/current_thread_id.hpp>
#include <boost/log/attributes/timer.hpp>
#include <iostream>
#include <chrono>
#include <ctime>
#include <iomanip>
#include <sstream>

namespace logging = boost::log;
namespace attrs = boost::log::attributes;
namespace expr = boost::log::expressions;
namespace keywords = boost::log::keywords;
namespace json = boost::json;

void init_logging() {
    logging::add_console_log(
        std::clog,
        keywords::format = [](logging::record_view const& rec, logging::formatting_ostream& strm) {
            json::object obj;

            auto ts = logging::extract<std::chrono::system_clock::time_point>("TimeStamp", rec);
            if (ts) {
                auto dur = ts.get().time_since_epoch();
                auto sec = std::chrono::duration_cast<std::chrono::seconds>(dur);
                auto micros = std::chrono::duration_cast<std::chrono::microseconds>(dur - sec).count();
                std::time_t t = std::chrono::system_clock::to_time_t(ts.get());
                std::tm tm = *std::gmtime(&t);
                char buf[32];
                std::strftime(buf, sizeof(buf), "%FT%T", &tm);
                std::stringstream ss;
                ss << buf << "." << std::setw(6) << std::setfill('0') << micros;
                obj["timestamp"] = ss.str();
            }

            obj["message"] = rec[expr::smessage].get();

            auto attr = rec[additional_data];
            obj["data"] = attr ? attr.get() : json::object{};

            strm << json::serialize(obj);
        }
    );

    logging::core::get()->add_global_attribute("TimeStamp", attrs::local_clock());
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
    BOOST_LOG_TRIVIAL(info) << logging::add_value(additional_data, data) << "error";
}