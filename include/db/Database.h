#pragma once

#include <string>
#include <stdexcept>
#include <sqlite3.h>

namespace workout::infra {

class Database {
public:
    explicit Database(const std::string& dbPath) {
        if (sqlite3_open(dbPath.c_str(), &db_) != SQLITE_OK) {
            throw std::runtime_error("Failed to open database: " + std::string(sqlite3_errmsg(db_)));
        }
        // Включаем поддержку внешних ключей
        execute("PRAGMA foreign_keys = ON;");
    }

    ~Database() {
        if (db_) {
            sqlite3_close(db_);
        }
    }

    Database(const Database&) = delete;
    Database& operator=(const Database&) = delete;

    // Выполнить SQL-скрипт из файла (для schema.sql и seed.sql)
    void executeScript(const std::string& sqlContent) {
        char* errMsg = nullptr;
        if (sqlite3_exec(db_, sqlContent.c_str(), nullptr, nullptr, &errMsg) != SQLITE_OK) {
            std::string error = errMsg;
            sqlite3_free(errMsg);
            throw std::runtime_error("SQL error: " + error);
        }
    }

    // Выполнить одиночный запрос без возврата данных
    void execute(const std::string& sql) {
        executeScript(sql);
    }

    sqlite3* getRawConnection() const noexcept { return db_; }

    // Транзакции
    void beginTransaction() { execute("BEGIN TRANSACTION;"); }
    void commit() { execute("COMMIT;"); }
    void rollback() { execute("ROLLBACK;"); }

private:
    sqlite3* db_ = nullptr;
};

} // namespace workout::infra
