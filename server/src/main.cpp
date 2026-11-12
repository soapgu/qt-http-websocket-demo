#include <httplib.h>
#include <nlohmann/json.hpp>

#include <atomic>
#include <chrono>
#include <csignal>
#include <iostream>
#include <thread>

namespace {
volatile std::sig_atomic_t stop_requested = 0;
void handle_signal(int) { stop_requested = 1; }
}

int main()
{
    using nlohmann::json;
    std::signal(SIGINT, handle_signal);
    std::signal(SIGTERM, handle_signal);
    std::atomic<bool> stopping{false};
    std::atomic<bool> listener_finished{false};
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
    server.Post("/api/echo", [](const httplib::Request &request, httplib::Response &response) {
        try {
            auto data = json::parse(request.body);
            response.set_content(json{{"data", std::move(data)}}.dump(), "application/json");
        } catch (const json::exception &) {
            response.status = 400;
            response.set_content(json{{"error", "请求体必须是合法 JSON"}}.dump(), "application/json");
        }
    });

    server.WebSocket("/ws", [&stopping](const httplib::Request &, httplib::ws::WebSocket &ws) {
        ws.set_read_timeout(std::chrono::milliseconds(200));
        std::string message;
        while (!stopping.load()) {
            const auto result = ws.read(message);
            if (result == httplib::ws::ReadResult::Timeout) { continue; }
            if (result == httplib::ws::ReadResult::Fail) { return; }
            const bool sent = result == httplib::ws::ReadResult::Binary
                ? ws.send(message.data(), message.size()) : ws.send(message);
            if (!sent) { return; }
        }
        ws.close(httplib::ws::CloseStatus::GoingAway, "server shutting down");
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
