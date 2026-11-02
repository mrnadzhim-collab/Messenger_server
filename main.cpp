#include <QCoreApplication>
#include <QDebug>

#include "server.h"

int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);

    quint16 port = 4545;
    QString dbPath = QString("/var/lib/local-messenger/messenger.db");

    Server server(port, dbPath);
    Q_UNUSED(server)

    qDebug() << QString("Сервер запущен на порту %1").arg(port);
    qDebug() << QString("Файл базы данных: %1").arg(dbPath);

    return a.exec();
}
