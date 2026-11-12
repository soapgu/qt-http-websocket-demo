#include <httplib.h>
#include <nlohmann/json.hpp>

#include <iostream>

int main()
{
    using nlohmann::json;
    httplib::Server server;
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

    if (!server.bind_to_port("127.0.0.1", 8080)) {
        std::cerr << "无法绑定 127.0.0.1:8080，请检查端口是否已被占用。" << std::endl;
        return 1;
    }
    std::cout << "HTTP 服务已启动：http://127.0.0.1:8080" << std::endl;
    if (!server.listen_after_bind()) {
        std::cerr << "HTTP 服务监听异常结束。" << std::endl;
        return 1;
    }
    return 0;
}
