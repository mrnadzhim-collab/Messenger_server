#ifndef TYPES_H
#define TYPES_H

#include <QString>

// Описание типов сообщений
namespace JSON_MESSEGES
{
const QString M_AUTH_OK         = QString("auth_ok");       // успешная авторизация
const QString M_ERROR           = QString("error");         // ошибка
const QString M_LOGIN           = QString("login");         // попытка авторизации
const QString M_REGISTER        = QString("register");      // регистрация
const QString M_PRESENCE        = QString("presence");      // статус
const QString M_PENDING         = QString("pending");       // отправка непрочитанных сообщений
const QString M_DELIVERED       = QString("delivered");     // сообщение с информированием о доставки сообщения
const QString M_USERS           = QString("users");         // пользователи
const QString M_HISTORY         = QString("history");       // история
const QString M_READ            = QString("read");          // прочтено
}

// Описание полей JSON-документа
namespace JSON_FIELDS
{
const QString F_TYPE            = QString("type");          // тип сообщения
const QString F_USER            = QString("user");          // пользователь
const QString F_MSG             = QString("msg");           // сообщение
const QString F_MSGS            = QString("msgs");          // сообщения
const QString F_USERNAME        = QString("username");      // логин
const QString F_PASSWORD        = QString("password");      // пароль
const QString F_NAME            = QString("name");          // имя пользователя
const QString F_ID              = QString("id");            // идентификатор пользователя
const QString F_IDS             = QString("ids");           // идентификаторы
const QString F_ONLINE          = QString("online");        // признак нахождения в сети
const QString F_FROM            = QString("from");          // от кого
const QString F_TO              = QString("to");            // кому
const QString F_TEXT            = QString("text");          // текст
const QString F_FILENAME        = QString("file_name");     // имя файла
const QString F_HASFILE         = QString("has_file");      // признак наличия файла
const QString F_TS              = QString("ts");            // время
const QString F_DELIVERED       = QString("delivered");     // признак доставки сообщения
const QString F_READ            = QString("read");          // признак прочтения сообщения
const QString F_LIST            = QString("list");          // список пользователей
const QString F_PEER            = QString("peer");          // собеседник
}

#endif // TYPES_H
