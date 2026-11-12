#include <QCoreApplication>
#include <QTextStream>

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);
    QTextStream(stdout) << "客户端构建骨架启动成功，界面和网络功能尚未实现。" << Qt::endl;
    return 0;
}
