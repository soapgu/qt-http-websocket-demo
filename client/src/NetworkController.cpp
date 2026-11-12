#include "NetworkController.h"
#include <QDateTime>
#include <QNetworkProxy>
#include <QNetworkRequest>

NetworkController::NetworkController(QObject *parent) : QObject(parent) {
    manager.setProxy(QNetworkProxy::NoProxy);
    socket.setProxy(QNetworkProxy::NoProxy);
    for (auto timer : {&requestTimer, &connectTimer, &closeTimer}) timer->setSingleShot(true);
    connect(&requestTimer, &QTimer::timeout, this, [this] { if (reply) { timedOut = true; reply->abort(); } });
    connect(&connectTimer, &QTimer::timeout, this, [this] { append("连接超时"); socket.abort(); });
    connect(&closeTimer, &QTimer::timeout, this, [this] { socket.abort(); if (quitting) emit readyToQuit(); });
    connect(&socket, &QWebSocket::connected, this, [this] { connectTimer.stop(); append("已连接"); });
    connect(&socket, &QWebSocket::disconnected, this, [this] { connectTimer.stop(); closeTimer.stop(); append("已断开"); if (quitting) emit readyToQuit(); });
    connect(&socket, &QWebSocket::stateChanged, this, [this] { emit changed(); });
    connect(&socket, &QWebSocket::errorOccurred, this, [this] { connectTimer.stop(); append("错误：" + socket.errorString()); });
    connect(&socket, &QWebSocket::textMessageReceived, this, [this](const QString &text) { append("接收文本：" + text); });
    connect(&socket, &QWebSocket::binaryMessageReceived, this, [this](const QByteArray &data) { append("接收二进制：" + QString::fromLatin1(data.toHex())); });
}
QString NetworkController::state() const {
    switch (socket.state()) {
    case QAbstractSocket::ConnectedState: return "已连接";
    case QAbstractSocket::ConnectingState: case QAbstractSocket::HostLookupState: return "连接中";
    case QAbstractSocket::ClosingState: return "断开中";
    default: return "未连接";
    }
}
void NetworkController::append(const QString &text) {
    messages.append(QDateTime::currentDateTime().toString("HH:mm:ss.zzz ") + text);
    while (messages.size() > 500) messages.removeFirst();
    emit changed();
}
void NetworkController::request(const QString &base, bool echo, const QString &body) {
    if (busy() || quitting) return;
    QUrl url(base.trimmed());
    if (!url.isValid() || (url.scheme() != "http" && url.scheme() != "https") || url.host().isEmpty()) {
        responseText = "请输入有效的 HTTP 服务地址"; emit changed(); return;
    }
    url.setPath(echo ? "/api/echo" : "/api/status"); url.setQuery(QString()); url.setFragment(QString());
    timedOut = false;
    QNetworkRequest req(url); req.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    reply = echo ? manager.post(req, body.toUtf8()) : manager.get(req);
    responseText = "请求中…"; emit changed(); requestTimer.start(5000);
    connect(reply, &QNetworkReply::finished, this, [this, current = reply.data()] {
        requestTimer.stop();
        const auto status = current->attribute(QNetworkRequest::HttpStatusCodeAttribute);
        responseText = "HTTP " + (status.isValid() ? status.toString() : QString("无响应")) + "\n" + QString::fromUtf8(current->isOpen() ? current->readAll() : QByteArray());
        if (current->error() != QNetworkReply::NoError) responseText += "\n错误：" + current->errorString();
        if (timedOut) responseText += "\n请求超时（5 秒）";
        current->deleteLater(); reply.clear(); emit changed();
    });
}
void NetworkController::connectSocket(const QString &address) {
    if (quitting || socket.state() != QAbstractSocket::UnconnectedState) return;
    QUrl url(address.trimmed());
    if (!url.isValid() || (url.scheme() != "ws" && url.scheme() != "wss") || url.host().isEmpty()) { append("请输入有效的 WebSocket 地址"); return; }
    connectTimer.start(5000); socket.open(url);
}
void NetworkController::disconnectSocket() {
    connectTimer.stop();
    if (socket.state() == QAbstractSocket::UnconnectedState) return;
    if (socket.state() != QAbstractSocket::ConnectedState) {
        socket.abort();
        if (quitting) emit readyToQuit();
        return;
    }
    closeTimer.start(2000); socket.close();
}
void NetworkController::shutdown() {
    if (quitting) return;
    quitting = true; requestTimer.stop(); connectTimer.stop();
    if (reply) reply->abort();
    if (socket.state() == QAbstractSocket::UnconnectedState) emit readyToQuit();
    else disconnectSocket();
}
