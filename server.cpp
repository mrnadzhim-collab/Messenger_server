#include "server.h"

#include <QTcpSocket>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>
#include <QDateTime>
#include <QJsonArray>
#include <QJsonObject>
#include <QCryptographicHash>
#include <QDebug>
#include <QDir>
#include <QFileInfo>

#include "errors.h"
#include "protocol.h"
#include "types.h"

static QString hashPassword(const QString &password)
{
    return QString::fromUtf8(QCryptographicHash::hash(password.toUtf8(), QCryptographicHash::Sha256).toHex());
}

static QJsonObject makeMsg(qint64 id, qint64 from, qint64 to, const QString &text,
                           const QString &fileName, qint64 ts, int delivered, int read)
{
    QJsonObject m;
    m[JSON_FIELDS::F_ID] = (double)id;
    m[JSON_FIELDS::F_FROM] = (double)from;
    m[JSON_FIELDS::F_TO] = (double)to;
    m[JSON_FIELDS::F_TEXT] = text;
    m[JSON_FIELDS::F_FILENAME] = fileName;
    m[JSON_FIELDS::F_HASFILE] = !fileName.isEmpty();
    m[JSON_FIELDS::F_TS] = (double)ts;
    m[JSON_FIELDS::F_DELIVERED] = delivered;
    m[JSON_FIELDS::F_READ] = read;
    return m;
}

ClientSession::ClientSession(QTcpSocket *socket, QObject *parent)
    : QObject(parent), m_socket(socket), m_userId(-1)
{
    connect(m_socket, &QTcpSocket::readyRead, this, &ClientSession::onReadyRead);
    connect(m_socket, &QTcpSocket::disconnected, this, &ClientSession::onDisconnected);
}

void ClientSession::send(const QJsonObject &obj)
{
    m_socket->write(Protocol::encode(obj));
}

void ClientSession::onReadyRead()
{
    m_buffer.append(m_socket->readAll());
    const QList<QJsonObject> packets = Protocol::decode(m_buffer);
    for (const QJsonObject &obj : packets)
        emit packetReady(this, obj);
}

void ClientSession::onDisconnected()
{
    emit sessionEnded(this);
}

Server::Server(quint16 port, const QString &dbPath, QObject *parent)
    : QTcpServer(parent), m_dbPath(dbPath)
{
    initDb();

    if (!listen(QHostAddress::Any, port))
        qFatal("Не удалось запустить сервер на порту %d", (int)port);
}

void Server::incomingConnection(qintptr handle)
{
    QTcpSocket *sock = new QTcpSocket(this);
    if (!sock->setSocketDescriptor(handle)) {
        sock->deleteLater();
        return;
    }
    ClientSession *session = new ClientSession(sock, this);
    connect(session, &ClientSession::packetReady, this, &Server::handlePacket);
    connect(session, &ClientSession::sessionEnded, this, &Server::handleDisconnect);
}

void Server::initDb()
{
    QFileInfo fi(m_dbPath);
    if (!fi.dir().exists() && !QDir().mkpath(fi.dir().absolutePath()))
        qFatal(ERRORS::CREATE_DB_DIR_ERROR, qPrintable(fi.dir().absolutePath()));

    m_db = QSqlDatabase::addDatabase("QSQLITE");
    m_db.setDatabaseName(m_dbPath);
    if (!m_db.open())
        qFatal(ERRORS::OPEN_DB_ERROR, qPrintable(m_db.lastError().text()));

    QSqlQuery q(m_db);
    q.exec(QString(
        "CREATE TABLE IF NOT EXISTS users("
        "id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "username TEXT UNIQUE NOT NULL,"
        "password TEXT NOT NULL,"
        "name TEXT NOT NULL)"));
    q.exec(QString(
        "CREATE TABLE IF NOT EXISTS messages("
        "id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "from_id INTEGER NOT NULL,"
        "to_id INTEGER NOT NULL,"
        "text TEXT,"
        "file_name TEXT,"
        "file_data BLOB,"
        "ts INTEGER NOT NULL,"
        "delivered INTEGER DEFAULT 0,"
        "read INTEGER DEFAULT 0)"));

    qDebug() << "База данных готова!";
}

void Server::broadcast(const QJsonObject &obj, qint64 exceptId)
{
    for (auto it = m_online.begin(); it != m_online.end(); ++it)
        if (it.key() != exceptId)
            it.value()->send(obj);
}

void Server::sendError(ClientSession *session, const QString &text)
{
    QJsonObject o;
    o[JSON_FIELDS::F_TYPE] = JSON_MESSEGES::M_ERROR;
    o[JSON_FIELDS::F_MSG] = text;
    session->send(o);
}

void Server::sendAuthOk(ClientSession *session, qint64 userId)
{
    QSqlQuery q(m_db);
    q.prepare(QString("SELECT username, name FROM users WHERE id=?"));
    q.addBindValue(userId);
    QString username, name;
    if (q.exec() && q.next()) {
        username = q.value(0).toString();
        name = q.value(1).toString();
    }

    QJsonObject user;
    user[JSON_FIELDS::F_ID] = (double)userId;
    user[JSON_FIELDS::F_USERNAME] = username;
    user[JSON_FIELDS::F_NAME] = name;

    QJsonObject o;
    o[JSON_FIELDS::F_TYPE] = JSON_MESSEGES::M_AUTH_OK;
    o[JSON_FIELDS::F_USER] = user;
    session->send(o);
}

void Server::deliverPending(ClientSession *session)
{
    const qint64 me = session->userId();

    QSqlQuery q(m_db);
    q.prepare(QString(
        "SELECT id, from_id, to_id, text, file_name, ts, read FROM messages "
        "WHERE to_id=? AND delivered=0 ORDER BY id"));
    q.addBindValue(me);
    QJsonArray msgs;
    QMap<qint64, QJsonArray> bySender;
    if (q.exec()) {
        while (q.next()) {
            qint64 id = q.value(0).toLongLong();
            qint64 from = q.value(1).toLongLong();
            msgs.append(makeMsg(id, from, me, q.value(3).toString(),
                                q.value(4).toString(), q.value(5).toLongLong(), 1, q.value(6).toInt()));
            bySender[from].append((double)id);
        }
    }

    if (msgs.isEmpty())
        return;

    QJsonObject o;
    o[JSON_FIELDS::F_TYPE] = JSON_MESSEGES::M_PENDING;
    o[JSON_FIELDS::F_MSGS] = msgs;
    session->send(o);

    QSqlQuery u(m_db);
    u.prepare(QStringLiteral("UPDATE messages SET delivered=1 WHERE to_id=? AND delivered=0"));
    u.addBindValue(me);
    u.exec();

    for (auto it = bySender.begin(); it != bySender.end(); ++it) {
        QJsonObject n;
        n[JSON_FIELDS::F_TYPE] = JSON_MESSEGES::M_DELIVERED;
        n[JSON_FIELDS::F_IDS] = it.value();
        if (m_online.contains(it.key()))
            m_online[it.key()]->send(n);
    }
}

void Server::handlePacket(ClientSession *session, const QJsonObject &obj)
{
    const QString t = obj[JSON_FIELDS::F_TYPE].toString();

    if (t == JSON_MESSEGES::M_REGISTER) {
        const QString username = obj[JSON_FIELDS::F_USERNAME].toString().trimmed();
        const QString password = obj[JSON_FIELDS::F_PASSWORD].toString();
        const QString name = obj[JSON_FIELDS::F_NAME].toString().trimmed();

        if (username.isEmpty() || password.isEmpty() || name.isEmpty()) {
            sendError(session, QString("Заполните все поля"));
            return;
        }

        QSqlQuery check(m_db);
        check.prepare(QString("SELECT id FROM users WHERE username=?"));
        check.addBindValue(username);
        if (check.exec() && check.next()) {
            sendError(session, QString("Пользователь с таким логином уже существует"));
            return;
        }

        QSqlQuery ins(m_db);
        ins.prepare(QString("INSERT INTO users(username, password, name) VALUES(?,?,?)"));
        ins.addBindValue(username);
        ins.addBindValue(hashPassword(password));
        ins.addBindValue(name);
        if (!ins.exec()) {
            sendError(session, QString("Ошибка регистрации"));
            return;
        }

        const qint64 id = ins.lastInsertId().toLongLong();
        if (m_online.contains(id))
            m_online[id]->socket()->disconnectFromHost();

        session->setUserId(id);
        m_online.insert(id, session);
        sendAuthOk(session, id);

        QJsonObject o;
        o[JSON_FIELDS::F_TYPE] = JSON_MESSEGES::M_PRESENCE;
        o[JSON_FIELDS::F_ID] = (double)id;
        o[JSON_FIELDS::F_ONLINE] = true;

        broadcast(o);
        deliverPending(session);

        return;
    }

    if (t == JSON_MESSEGES::M_LOGIN) {
        const QString username = obj[JSON_FIELDS::F_USERNAME].toString().trimmed();
        const QString password = obj[JSON_FIELDS::F_PASSWORD].toString();

        QSqlQuery q(m_db);
        q.prepare(QString("SELECT id, password FROM users WHERE username=?"));
        q.addBindValue(username);
        if (!(q.exec() && q.next())) {
            sendError(session, QString("Неверный логин или пароль"));
            return;
        }
        if (q.value(1).toString() != hashPassword(password)) {
            sendError(session, QString("Неверный логин или пароль"));
            return;
        }
        const qint64 id = q.value(0).toLongLong();
        if (m_online.contains(id))
            m_online[id]->socket()->disconnectFromHost();

        session->setUserId(id);
        m_online.insert(id, session);
        sendAuthOk(session, id);

        QJsonObject o;
        o[JSON_FIELDS::F_TYPE] = JSON_MESSEGES::M_PRESENCE;
        o[JSON_FIELDS::F_ID] = (double)id;
        o[JSON_FIELDS::F_ONLINE]= true;

        broadcast(o);
        deliverPending(session);

        return;
    }

    if (session->userId() < 0) {
        sendError(session, QString("Сначала выполните вход"));
        return;
    }
}

void Server::handleDisconnect(ClientSession *session)
{
    const qint64 id = session->userId();
    if (id >= 0 && m_online.value(id) == session) {
        m_online.remove(id);

        QJsonObject o;
        o[JSON_FIELDS::F_TYPE] = JSON_MESSEGES::M_PRESENCE;
        o[JSON_FIELDS::F_ID] = (double)id;
        o[JSON_FIELDS::F_ONLINE] = false;

        broadcast(o);
    }
    session->deleteLater();
}
