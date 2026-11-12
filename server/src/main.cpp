#include <httplib.h>
#include <nlohmann/json.hpp>

#include <atomic>
#include <chrono>
#include <csignal>
#include <iostream>
#include <thread>
#include <mutex>
#include <deque>
#include <vector>
#include <memory>
#include <algorithm>

namespace {
volatile std::sig_atomic_t stop_requested = 0;
struct Subscription { std::deque<std::string> messages; bool overflow = false; };
void handle_signal(int) { stop_requested = 1; }
}

int main()
{
    using nlohmann::json;
    std::signal(SIGINT, handle_signal);
    std::signal(SIGTERM, handle_signal);
    std::atomic<bool> stopping{false};
    std::atomic<bool> listener_finished{false};
    std::mutex subscriptions_mutex;
    std::vector<std::shared_ptr<Subscription>> subscriptions;
    httplib::Server server;
    server.set_read_timeout(2, 0);
    server.set_write_timeout(2, 0);
    server.set_keep_alive_timeout(2);
    // 禁用上游默认的端口复用，确保重复启动时绑定失败。
    server.set_socket_options([](socket_t socket) {
        httplib::set_socket_opt(socket, SOL_SOCKET, SO_REUSEADDR, 1);
    });
    server.Get("/api/status", [](const httplib::Request &, httplib::Response &response) {
        response.set_content(json{{"status", "ok"}}.dump(), "application/json");
    });
    server.Post("/api/echo", [&](const httplib::Request &request, httplib::Response &response) {
        try {
            auto data = json::parse(request.body);
            const auto message = data.dump();
            response.set_content(json{{"data", data}}.dump(), "application/json");
            std::lock_guard<std::mutex> lock(subscriptions_mutex);
            if (!stopping.load()) {
                for (auto &subscriber : subscriptions) {
                    if (subscriber->overflow) { continue; }
                    if (subscriber->messages.size() >= 100) {
                        subscriber->overflow = true;
                    } else { subscriber->messages.push_back(message); }
                }
            }
        } catch (const json::exception &) {
            response.status = 400;
            response.set_content(json{{"error", "请求体必须是合法 JSON"}}.dump(), "application/json");
        }
    });

    server.WebSocket("/ws", [&](const httplib::Request &, httplib::ws::WebSocket &ws) {
        auto subscriber = std::make_shared<Subscription>();
        {
            std::lock_guard<std::mutex> lock(subscriptions_mutex);
            subscriptions.push_back(subscriber);
        }
        ws.set_read_timeout(std::chrono::milliseconds(200));
        bool overflow = false;
        while (!stopping.load()) {
            std::string outgoing;
            bool pending = false;
            {
                std::lock_guard<std::mutex> lock(subscriptions_mutex);
                overflow = subscriber->overflow;
                if (!overflow && !subscriber->messages.empty()) {
                    outgoing = std::move(subscriber->messages.front());
                    subscriber->messages.pop_front();
                    pending = true;
                }
            }
            if (overflow) { break; }
            if (pending) {
                if (!ws.send(outgoing)) { break; }
                continue;
            }
            std::string ignored;
            if (ws.read(ignored) == httplib::ws::ReadResult::Fail) { break; }
        }
        {
            std::lock_guard<std::mutex> lock(subscriptions_mutex);
            subscriptions.erase(std::remove(subscriptions.begin(), subscriptions.end(), subscriber), subscriptions.end());
        }
        if (ws.is_open()) {
            ws.close(httplib::ws::CloseStatus::GoingAway,
                     overflow ? "subscriber queue full" : "server shutting down");
        }
    });

    if (!server.bind_to_port("127.0.0.1", 8080)) {
        std::cerr << "无法绑定 127.0.0.1:8080，请检查端口是否已被占用。" << std::endl;
        return 1;
    }
    std::thread manager([&] {
        while (!listener_finished.load()) {
            if (stop_requested) {
                stopping.store(true);
                server.stop();
                return;
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
        }
    });
    std::cout << "HTTP 服务已启动：http://127.0.0.1:8080" << std::endl;
    const bool listened = server.listen_after_bind();
    stopping.store(true);
    listener_finished.store(true);
    manager.join();
    if (!listened && !stop_requested) {
        std::cerr << "HTTP 服务监听异常结束。" << std::endl;
        return 1;
    }
    std::cout << "服务已停止，连接与线程已回收。" << std::endl;
    return 0;
}
