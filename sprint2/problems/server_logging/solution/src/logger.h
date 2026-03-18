#pragma once

#include <boost/log/attributes.hpp>
#include <boost/log/expressions/keyword.hpp>
#include <boost/log/trivial.hpp>
#include <boost/json.hpp>

BOOST_LOG_ATTRIBUTE_KEYWORD(additional_data, "AdditionalData", boost::json::value)

namespace logging = boost::log;
namespace json = boost::json;

void InitLogging();