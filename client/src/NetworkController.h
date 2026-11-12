#pragma once
#include <QObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QPointer>
#include <QWebSocket>
#include <QTimer>
#include <QStringList>

class NetworkController : public QObject {
    Q_OBJECT
    Q_PROPERTY(bool busy READ busy NOTIFY changed)
    Q_PROPERTY(QString response READ response NOTIFY changed)
    Q_PROPERTY(QString state READ state NOTIFY changed)
    Q_PROPERTY(QString log READ log NOTIFY changed)
public:
    explicit NetworkController(QObject *parent = nullptr);
    bool busy() const { return !reply.isNull(); }
    QString response() const { return responseText; }
    QString state() const;
    QString log() const { return messages.join('\n'); }
    Q_INVOKABLE void request(const QString &base, bool echo, const QString &body);
    Q_INVOKABLE void connectSocket(const QString &url);
    Q_INVOKABLE void disconnectSocket();
    Q_INVOKABLE void shutdown();
signals:
    void changed();
    void readyToQuit();
private:
    void append(const QString &text);
    QNetworkAccessManager manager;
    QPointer<QNetworkReply> reply;
    QWebSocket socket;
    QTimer requestTimer, connectTimer, closeTimer;
    QString responseText;
    QStringList messages;
    bool quitting = false;
    bool timedOut = false;
};
