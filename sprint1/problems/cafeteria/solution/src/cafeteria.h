#pragma once
#ifdef _WIN32
#include <sdkddkver.h>
#endif

#include <boost/asio/io_context.hpp>
#include <boost/asio/steady_timer.hpp>
#include <memory>

#include "hotdog.h"
#include "result.h"

namespace net = boost::asio;

using HotDogHandler = std::function<void(Result<HotDog> hot_dog)>;

class HotDogOrder : public std::enable_shared_from_this<HotDogOrder> {
public:
    HotDogOrder(net::io_context& io,
                Store& store,
                std::shared_ptr<GasCooker> cooker,
                HotDogHandler handler)
        : io_(io)
        , store_(store)
        , cooker_(std::move(cooker))
        , handler_(std::move(handler))
        , sausage_timer_(io)
        , bread_timer_(io) {
    }

    void Start() {
        sausage_ = store_.GetSausage();
        bread_ = store_.GetBread();

        sausage_->StartFry(*cooker_, [self = shared_from_this()] {
            self->StartSausageTimer();
        });

        bread_->StartBake(*cooker_, [self = shared_from_this()] {
            self->StartBreadTimer();
        });
    }

private:
    void StartSausageTimer() {
        sausage_timer_.expires_after(std::chrono::milliseconds(1500));
        sausage_timer_.async_wait([self = shared_from_this()](auto) {
            self->sausage_->StopFry();
            self->TryComplete();
        });
    }

    void StartBreadTimer() {
        bread_timer_.expires_after(std::chrono::milliseconds(1000));
        bread_timer_.async_wait([self = shared_from_this()](auto) {
            self->bread_->StopBaking();
            self->TryComplete();
        });
    }

    void TryComplete() {
        if (completed_)
            return;

        if (!sausage_->IsCooked() || !bread_->IsCooked())
            return;

        completed_ = true;

        try {
            HotDog hotdog(
                sausage_->GetId(),
                sausage_,
                bread_);

            handler_(Result<HotDog>(std::move(hotdog)));
        } catch (...) {
            handler_(Result<HotDog>::FromCurrentException());
        }
    }

private:
    net::io_context& io_;
    Store& store_;
    std::shared_ptr<GasCooker> cooker_;
    HotDogHandler handler_;

    std::shared_ptr<Sausage> sausage_;
    std::shared_ptr<Bread> bread_;

    net::steady_timer sausage_timer_;
    net::steady_timer bread_timer_;

    bool completed_ = false;
};

class Cafeteria {
public:
    explicit Cafeteria(net::io_context& io)
        : io_{io} {
    }

    void OrderHotDog(HotDogHandler handler) {
        net::post(io_, [this, handler = std::move(handler)]() mutable {
            std::make_shared<HotDogOrder>(
                io_,
                store_,
                gas_cooker_,
                std::move(handler)
            )->Start();
        });
    }

private:
    net::io_context& io_;
    Store store_;
    std::shared_ptr<GasCooker> gas_cooker_ = std::make_shared<GasCooker>(io_);
};
