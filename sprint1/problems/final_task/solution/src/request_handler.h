#pragma once

#include <boost/json.hpp>
#include <boost/beast/http.hpp>

#include "model.h"

namespace http_handler {

namespace beast = boost::beast;
namespace http = beast::http;
namespace json = boost::json;

namespace api {
inline constexpr std::string_view MAPS = "/api/v1/maps";
inline constexpr std::string_view MAP = "/api/v1/maps/";
inline constexpr std::string_view API = "/api/";
}

class RequestHandler {
public:
    explicit RequestHandler(model::Game& game)
        : game_{game} {
    }

    RequestHandler(const RequestHandler&) = delete;
    RequestHandler& operator=(const RequestHandler&) = delete;

    template <typename Body, typename Allocator, typename Send>
    void operator()(http::request<Body, http::basic_fields<Allocator>>&& req,
                    Send&& send) {

        std::string target = std::string(req.target());

        if (req.method() != http::verb::get) {
            return send(MakeMethodNotAllowed(req.version()));
        }

        if (target == api::MAPS) {
            return send(GetMaps(req.version()));
        }

        if (target.starts_with(api::MAP)) {
            return send(GetMap(target, req.version()));
        }

        if (target.starts_with(api::API)) {
            return send(MakeBadRequest(req.version()));
        }
    }

private:
    http::response<http::string_body> GetMaps(unsigned version);
    http::response<http::string_body> GetMap(const std::string& target, unsigned version);

    http::response<http::string_body> MakeBadRequest(unsigned version);
    http::response<http::string_body> MakeMethodNotAllowed(unsigned version);

    model::Game& game_;
};

}  // namespace http_handler
