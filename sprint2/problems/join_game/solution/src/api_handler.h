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
    void Handle(http::request<Body, http::basic_fields<Allocator>>&& req,
                Send&& send);

private:
    model::Game& game_;
    model::PlayerManager& players_;

    template <typename Send>
    void HandleJoin(const http::request<http::string_body>& req, Send&& send);

    template <typename Send>
    void HandlePlayers(const http::request<http::string_body>& req, Send&& send);
};

} // namespace http_handler