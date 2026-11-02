#ifndef ERRORS_H
#define ERRORS_H

#include <QString>

namespace ERRORS
{
const char *CREATE_DB_DIR_ERROR             = "ОШИБКА БД: НЕ УДАЛОСЬ СОЗДАТЬ КАТАЛОГ ДЛЯ ХРАНЕНИЯ БД. %s";
const char *OPEN_DB_ERROR                   = "ОШИБКА БД: НЕ УДАЛОСЬ ОТКРЫТЬ ФАЙЛ БД. %s";
}

#endif // ERRORS_H
