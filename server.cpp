#include "server.h"

#include <QTcpSocket>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>
#include <QDateTime>
#include <QDebug>
#include <QDir>
#include <QFileInfo>

#include "errors.h"

Server::Server(quint16 port, const QString &dbPath, QObject *parent)
    : QTcpServer(parent), m_dbPath(dbPath)
{
    initDb();
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

    qDebug() << "База данных готова";
}
