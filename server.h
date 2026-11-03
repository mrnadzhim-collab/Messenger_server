#ifndef SERVER_H
#define SERVER_H

#include <QTcpServer>
#include <QTcpSocket>
#include <QMap>
#include <QByteArray>
#include <QSqlDatabase>

class ClientSession : public QObject
{
    Q_OBJECT
public:
    explicit ClientSession(QTcpSocket *socket, QObject *parent = nullptr);
    QTcpSocket *socket() const { return m_socket; }
    qint64 userId() const { return m_userId; }
    void setUserId(qint64 id) { m_userId = id; }
    void send(const QJsonObject &obj);

signals:
    void packetReady(ClientSession *session, const QJsonObject &obj);
    void sessionEnded(ClientSession *session);

private slots:
    void onReadyRead();
    void onDisconnected();

private:
    QTcpSocket *m_socket;
    QByteArray m_buffer;
    qint64 m_userId;
};

class Server : public QTcpServer
{
    Q_OBJECT
public:
    explicit Server(quint16 port, const QString &dbPath, QObject *parent = nullptr);

protected:
    void incomingConnection(qintptr handle) override;

private:
    void initDb();

    void broadcast(const QJsonObject &obj, qint64 exceptId = -1);
    void sendError(ClientSession *session, const QString &text);
    void sendAuthOk(ClientSession *session, qint64 userId);
    void deliverPending(ClientSession *session);

    QSqlDatabase m_db;
    QMap<qint64, ClientSession *> m_online;
    QString m_dbPath;

private slots:
    void handlePacket(ClientSession *session, const QJsonObject &obj);
    void handleDisconnect(ClientSession *session);
};

#endif // SERVER_H
