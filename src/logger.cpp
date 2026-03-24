#include "logger.h"
#include <iostream>
#include <boost/log/core.hpp>
#include <boost/log/expressions.hpp>
#include <boost/log/sinks/text_ostream_backend.hpp>
#include <boost/log/sinks/sync_frontend.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>

namespace logging = boost::log;
namespace sinks = boost::log::sinks;
namespace expr = boost::log::expressions;
namespace json = boost::json;

BOOST_LOG_ATTRIBUTE_KEYWORD(timestamp, "TimeStamp", boost::posix_time::ptime)
BOOST_LOG_ATTRIBUTE_KEYWORD(message, "Message", std::string)

void InitLogging() {
    logging::add_common_attributes();

    auto sink = logging::add_console_log(
        std::cout,
        boost::log::keywords::auto_flush = true // ✅ автофлаш
    );

    sink->set_formatter(
        [](logging::record_view const& rec, logging::formatting_ostream& strm) {

            json::object log_obj;

            if (auto ts = rec[timestamp])
                log_obj["timestamp"] = to_iso_extended_string(*ts);

            if (auto msg = rec[message])
                log_obj["message"] = *msg;

            if (auto data = rec[additional_data])
                log_obj["data"] = *data;
            else
                log_obj["data"] = json::object{};

            strm << json::serialize(log_obj);
        }
    );
    logging::core::get()->set_filter(logging::trivial::severity >= logging::trivial::info);
}