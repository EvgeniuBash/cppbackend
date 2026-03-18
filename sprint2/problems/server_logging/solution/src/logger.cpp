#include "logger.h"

#include <boost/log/core.hpp>
#include <boost/log/expressions.hpp>
#include <boost/log/utility/setup/common_attributes.hpp>
#include <boost/log/sinks/text_ostream_backend.hpp>
#include <boost/log/utility/setup/console.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>

using namespace boost::log;

BOOST_LOG_ATTRIBUTE_KEYWORD(timestamp, "TimeStamp", boost::posix_time::ptime)
BOOST_LOG_ATTRIBUTE_KEYWORD(message, "Message", std::string)
BOOST_LOG_ATTRIBUTE_KEYWORD(additional_data, "AdditionalData", json::value)

void InitLogging() {
    add_common_attributes();

    auto sink = add_console_log(std::cout);

    sink->set_formatter(
        [](logging::record_view const& rec, logging::formatting_ostream& strm) {

            json::object log_obj;

            auto ts = rec[timestamp];
            if (ts) {
                log_obj["timestamp"] =
                    to_iso_extended_string(*ts);
            }

            auto msg = rec[message];
            if (msg) {
                log_obj["message"] = *msg;
            }

            auto data = rec[additional_data];
            if (data) {
                log_obj["data"] = *data;
            } else {
                log_obj["data"] = json::object{};
            }

            strm << json::serialize(log_obj);
        }
    );
}