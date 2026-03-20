#pragma once

#include <boost/beast/http.hpp>
#include <boost/json.hpp>
#include "model.h"
#include "player.h"

namespace http_handler {

namespace http = boost::beast::http;
namespace json = boost::json;

class ApiHandler {
public:
    ApiHandler(model::Game& game, model::PlayerManager& players)
        : game_(game), players_(players) {}

    
    template <typename Body, typename Allocator, typename Send>
    void ApiHandler::Handle(http::request<Body, http::basic_fields<Allocator>>&& req,
    Send&& send) {

        std::string target = std::string(req.target());

        if (target == "/api/v1/game/join") {
            HandleJoin(req, send);
            return;
        }

        if (target == "/api/v1/game/players") {
            HandlePlayers(req, send);
            return;
        }

        http::response<http::string_body> res{http::status::bad_request, req.version()};
        res.body() = "Bad request";
        res.prepare_payload();
        send(std::move(res));
    }

private:
    model::Game& game_;
    model::PlayerManager& players_;

    template <typename Send>
    void HandleJoin(const http::request<http::string_body>& req, Send&& send);

    template <typename Send>
    void HandlePlayers(const http::request<http::string_body>& req, Send&& send);
};

} // namespace http_handler