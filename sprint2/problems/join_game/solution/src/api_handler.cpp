#include "api_handler.h"

namespace http_handler {

template <typename Send>
void ApiHandler::HandleJoin(const http::request<http::string_body>& req,
                            Send&& send) {

    if (req.method() != http::verb::post) {
        http::response<http::string_body> res{http::status::method_not_allowed, req.version()};
        res.set(http::field::allow, "POST");
        send(std::move(res));
        return;
    }

    json::value body;

    try {
        body = json::parse(req.body());
    } catch (...) {
        http::response<http::string_body> res{http::status::bad_request, req.version()};
        send(std::move(res));
        return;
    }

    auto obj = body.as_object();

    std::string name = obj["userName"].as_string().c_str();
    std::string map_id = obj["mapId"].as_string().c_str();

    const model::Map* map = game_.FindMap(model::Map::Id{map_id});

    if (!map) {
        http::response<http::string_body> res{http::status::not_found, req.version()};
        send(std::move(res));
        return;
    }

    auto& player = players_.AddPlayer(name, map->GetId());

    json::object resp{
        {"authToken", player.GetToken()},
        {"playerId", player.GetId()}
    };

    http::response<http::string_body> res{http::status::ok, req.version()};
    res.set(http::field::content_type, "application/json");
    res.body() = json::serialize(resp);
    res.prepare_payload();

    send(std::move(res));
}

} //namespace http_handler