#include "sdk.h"
//
#include <boost/asio/signal_set.hpp>
#include <iostream>
#include <mutex>
#include <thread>
#include <vector>

#include "http_server.h"

namespace {
namespace net = boost::asio;
using namespace std::literals;
namespace sys = boost::system;
namespace http = boost::beast::http;

// Запрос, тело которого представлено в виде строки
using StringRequest = http::request<http::string_body>;
// Ответ, тело которого представлено в виде строки
using StringResponse = http::response<http::string_body>;

struct ContentType {
    ContentType() = delete;
    constexpr static std::string_view TEXT_HTML = "text/html"sv;
};

// Создаёт StringResponse с заданными параметрами
StringResponse MakeStringResponse(http::status status, std::string_view body, unsigned http_version,
                                  bool keep_alive,
                                  std::string_view content_type = ContentType::TEXT_HTML) {
    StringResponse response(status, http_version);
    response.set(http::field::content_type, content_type);
    response.body() = body;
    response.content_length(body.size());
    response.keep_alive(keep_alive);
    return response;
}

StringResponse HandleRequest(StringRequest&& req) {
    // Извлекаем target и убираем ведущий '/'
    std::string target(req.target());
    if (!target.empty() && target[0] == '/') {
        target = target.substr(1);
    }

    // Обрабатываем различные HTTP методы
    switch (req.method()) {
        case http::verb::get: {
            // GET запрос - возвращаем Hello, {target}
            std::string body = "Hello, " + target;
            return MakeStringResponse(http::status::ok, body, req.version(), req.keep_alive());
        }
        
        case http::verb::head: {
            // HEAD запрос - такой же как GET, но без тела
            std::string body = "Hello, " + target;
            StringResponse response = MakeStringResponse(http::status::ok, "", req.version(), req.keep_alive());
            response.content_length(body.size());  // Content-Length как для GET
            return response;
        }
        
        default: {
            // Неподдерживаемый метод
            StringResponse response = MakeStringResponse(
                http::status::method_not_allowed, 
                "Invalid method.", 
                req.version(), 
                req.keep_alive()
            );
            response.set(http::field::allow, "GET, HEAD");
            return response;
        }
    }
}

// Запускает функцию fn на n потоках, включая текущий (используем std::thread вместо std::jthread)
template <typename Fn>
void RunWorkers(unsigned n, const Fn& fn) {
    n = std::max(1u, n);
    std::vector<std::thread> workers;
    workers.reserve(n - 1);
    
    // Запускаем n-1 рабочих потоков, выполняющих функцию fn
    for (unsigned i = 0; i < n - 1; ++i) {
        workers.emplace_back(fn);
    }
    
    fn(); // Основной поток тоже работает
    
    // Ждём завершения всех потоков
    for (auto& t : workers) {
        if (t.joinable()) {
            t.join();
        }
    }
}

}  // namespace

int main() {
    const unsigned num_threads = std::thread::hardware_concurrency();

    net::io_context ioc(num_threads);

    // Подписываемся на сигналы и при их получении завершаем работу сервера
    net::signal_set signals(ioc, SIGINT, SIGTERM);
    signals.async_wait([&ioc](const sys::error_code& ec, [[maybe_unused]] int signal_number) {
        if (!ec) {
            ioc.stop();
        }
    });

    const auto address = net::ip::make_address("0.0.0.0");
    constexpr net::ip::port_type port = 8080;
    
    // Запускаем HTTP сервер с обработчиком запросов
    http_server::ServeHttp(ioc, {address, port}, [](auto&& req, auto&& sender) {
        sender(HandleRequest(std::forward<decltype(req)>(req)));
    });

    // Эта надпись сообщает тестам о том, что сервер запущен и готов обрабатывать запросы
    std::cout << "Server has started..."sv << std::endl;

    RunWorkers(num_threads, [&ioc] {
        ioc.run();
    });
    
    return 0;
}