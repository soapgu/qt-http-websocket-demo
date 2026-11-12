#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include "NetworkController.h"

int main(int argc, char *argv[]) {
    QGuiApplication app(argc, argv);
    app.setQuitOnLastWindowClosed(false);
    NetworkController controller;
    QQmlApplicationEngine engine;
    engine.rootContext()->setContextProperty("network", &controller);
    QObject::connect(&controller, &NetworkController::readyToQuit, &app, &QCoreApplication::quit, Qt::QueuedConnection);
    QObject::connect(&engine, &QQmlApplicationEngine::objectCreationFailed, &app, [] { QCoreApplication::exit(1); }, Qt::QueuedConnection);
    engine.loadFromModule("DemoClient", "Main");
    return app.exec();
}
