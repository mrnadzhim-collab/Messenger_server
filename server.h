#ifndef SERVER_H
#define SERVER_H

#include <QTcpServer>
#include <QTcpSocket>
#include <QMap>
#include <QByteArray>
#include <QSqlDatabase>

class Server : public QTcpServer
{
    Q_OBJECT
public:
    explicit Server(quint16 port, const QString &dbPath, QObject *parent = nullptr);

private:
    void initDb();

    QSqlDatabase m_db;
    QString m_dbPath;
};

#endif // SERVER_H
