#pragma once

#include <utility>

template <typename Handler>
class LoggingRequestHandler {
public:
    explicit LoggingRequestHandler(Handler& handler)
        : handler_(handler) {
    }

    template <typename Request, typename Send>
    void operator()(Request&& req, Send&& send) {
        handler_(
            std::forward<Request>(req),
            std::forward<Send>(send)
        );
    }

private:
    Handler& handler_;
};