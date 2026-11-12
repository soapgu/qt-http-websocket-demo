#include "NetworkController.h"
#include <QCoreApplication>
#include <QElapsedTimer>
#include <QThread>
#include <iostream>
#include <QTcpServer>
#include <functional>
static void waitFor(std::function<bool()> predicate) {
 QElapsedTimer timer;timer.start();
 while (!predicate() && timer.elapsed()<6500) { QCoreApplication::processEvents(); QThread::msleep(5); }
 if (!predicate()) { std::cerr << "验证超时\n"; std::exit(1); }
}
int main(int argc,char**argv){
 QCoreApplication app(argc,argv);NetworkController c;
 c.connectSocket("ws://127.0.0.1:8080/ws");waitFor([&]{return c.state()=="已连接";});
 c.request("http://127.0.0.1:8080",false,"");waitFor([&]{return !c.busy();});
 if(!c.response().contains("200"))return 2;
 c.request("http://127.0.0.1:8080",true,"{\"message\":\"中文\"}");waitFor([&]{return !c.busy()&&c.log().contains("中文");});
 c.request("http://127.0.0.1:8080",true,"invalid");waitFor([&]{return !c.busy();});if(!c.response().contains("400"))return 3;
 c.disconnectSocket();waitFor([&]{return c.state()=="未连接";});
 c.connectSocket("ws://127.0.0.1:8080/ws");waitFor([&]{return c.state()=="已连接";});
 bool done=false;QObject::connect(&c,&NetworkController::readyToQuit,[&]{done=true;});c.shutdown();waitFor([&]{return done;});
 NetworkController bad;bad.connectSocket("ws://127.0.0.1:1/ws");waitFor([&]{return bad.log().contains("错误");});
 QTcpServer silent;silent.listen(QHostAddress::LocalHost,0);
 QString endpoint="http://127.0.0.1:"+QString::number(silent.serverPort());
 NetworkController timeout;timeout.request(endpoint,false,"");waitFor([&]{return !timeout.busy();});if(!timeout.response().contains("超时"))return 4;
 NetworkController pending;bool stopped=false;QObject::connect(&pending,&NetworkController::readyToQuit,[&]{stopped=true;});pending.request(endpoint,false,"");pending.shutdown();waitFor([&]{return stopped;});
 NetworkController connecting;bool closed=false;QObject::connect(&connecting,&NetworkController::readyToQuit,[&]{closed=true;});connecting.connectSocket("ws://127.0.0.1:"+QString::number(silent.serverPort()));connecting.shutdown();waitFor([&]{return closed;});
 std::cout<<"客户端 HTTP、广播、400、断开重连、退出及连接失败通过\n";
}
