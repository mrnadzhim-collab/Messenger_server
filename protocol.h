#ifndef PROTOCOL_H
#define PROTOCOL_H

#include <QByteArray>
#include <QJsonObject>
#include <QJsonDocument>
#include <QJsonParseError>
#include <QtEndian>

namespace Protocol {

inline QByteArray encode(const QJsonObject &obj)
{
    QByteArray payload = QJsonDocument(obj).toJson(QJsonDocument::Compact);
    QByteArray frame;
    frame.resize(4);
    qToBigEndian((quint32)payload.size(), reinterpret_cast<uchar*>(frame.data()));
    frame.append(payload);
    return frame;
}

inline QList<QJsonObject> decode(QByteArray &buffer)
{
    QList<QJsonObject> out;
    while (buffer.size() >= 4) {
        quint32 len = qFromBigEndian<quint32>(reinterpret_cast<const uchar*>(buffer.constData()));
        if ((quint64)buffer.size() < (quint64)4 + len)
            break;
        QJsonParseError err;
        QJsonDocument doc = QJsonDocument::fromJson(buffer.mid(4, (int)len), &err);
        buffer.remove(0, 4 + (int)len);
        if (err.error == QJsonParseError::NoError && doc.isObject())
            out << doc.object();
    }
    return out;
}

}

#endif // PROTOCOL_H
