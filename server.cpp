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

static QString hashPassword(const QString &password)
{
    return QString::fromUtf8(QCryptographicHash::hash(password.toUtf8(), QCryptographicHash::Sha256).toHex());
}

static QJsonObject makeMsg(qint64 id, qint64 from, qint64 to, const QString &text,
                           const QString &fileName, qint64 ts, int delivered, int read)
{
    QJsonObject m;
    m["id"] = (double)id;
    m["from"] = (double)from;
    m["to"] = (double)to;
    m["text"] = text;
    m["file_name"] = fileName;
    m["has_file"] = !fileName.isEmpty();
    m["ts"] = (double)ts;
    m["delivered"] = delivered;
    m["read"] = read;
    return m;
}

Server::Server(quint16 port, const QString &dbPath, QObject *parent)
    : QTcpServer(parent), m_dbPath(dbPath)
{
    initDb();

    if (!listen(QHostAddress::Any, port))
        qFatal("Не удалось запустить сервер на порту %d", (int)port);
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
