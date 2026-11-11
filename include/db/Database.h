#pragma once

#include <sqlite3.h>

#include <cstdint>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <utility>

namespace workout::infra {

class Database {
public:
    explicit Database(const std::string& dbPath) {
        const int rc = sqlite3_open(dbPath.c_str(), &db_);
        if (rc != SQLITE_OK) {
            const std::string message = db_ ? sqlite3_errmsg(db_) : "unknown error";
            if (db_) {
                sqlite3_close(db_);
                db_ = nullptr;
            }
            throw std::runtime_error("Failed to open database '" + dbPath + "': " + message);
        }

        try {
            execute("PRAGMA foreign_keys = ON;");
        } catch (...) {
            sqlite3_close(db_);
            db_ = nullptr;
            throw;
        }

        // Не падать сразу на SQLITE_BUSY при конкурентном доступе.
        sqlite3_busy_timeout(db_, 5000);
    }

    ~Database() {
        if (db_) {
            sqlite3_close(db_);
        }
    }

    Database(const Database&) = delete;
    Database& operator=(const Database&) = delete;

    // Выполнить SQL-скрипт (для schema.sql и seed.sql)
    void executeScript(const std::string& sqlContent) {
        char* errMsg = nullptr;
        if (sqlite3_exec(db_, sqlContent.c_str(), nullptr, nullptr, &errMsg) != SQLITE_OK) {
            std::string error = errMsg ? errMsg : "unknown error";
            sqlite3_free(errMsg);
            throw std::runtime_error("SQL error: " + error);
        }
    }

    // Выполнить SQL-скрипт из файла
    void executeFile(const std::string& path) {
        std::ifstream file(path, std::ios::binary);
        if (!file) {
            throw std::runtime_error("Failed to open SQL file: " + path);
        }

        std::ostringstream buffer;
        buffer << file.rdbuf();
        executeScript(buffer.str());
    }

    // Выполнить одиночный запрос без возврата данных
    void execute(const std::string& sql) {
        executeScript(sql);
    }

    sqlite3* getRawConnection() const noexcept { return db_; }

    [[nodiscard]] int64_t lastInsertRowId() const noexcept {
        return sqlite3_last_insert_rowid(db_);
    }

    [[nodiscard]] int changes() const noexcept { return sqlite3_changes(db_); }

    // Транзакции
    void beginTransaction() { execute("BEGIN TRANSACTION;"); }
    void commit() { execute("COMMIT;"); }
    void rollback() { execute("ROLLBACK;"); }

private:
    sqlite3* db_ = nullptr;
};

/**
 * @brief RAII-обёртка над sqlite3_stmt.
 *
 * Гарантирует sqlite3_finalize при выходе из области видимости и превращает
 * коды ошибок SQLite в исключения.
 */
class Statement {
public:
    Statement(sqlite3* db, const std::string& sql) : db_(db) {
        if (sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt_, nullptr) != SQLITE_OK) {
            throw std::runtime_error("SQL prepare failed: " + error() + " [" + sql + "]");
        }
    }

    ~Statement() {
        if (stmt_) {
            sqlite3_finalize(stmt_);
        }
    }

    Statement(const Statement&) = delete;
    Statement& operator=(const Statement&) = delete;

    Statement(Statement&& other) noexcept : db_(other.db_), stmt_(other.stmt_) {
        other.stmt_ = nullptr;
    }

    Statement& operator=(Statement&& other) noexcept {
        if (this != &other) {
            if (stmt_) {
                sqlite3_finalize(stmt_);
            }
            db_ = other.db_;
            stmt_ = other.stmt_;
            other.stmt_ = nullptr;
        }
        return *this;
    }

    void bind(int index, int value) {
        check(sqlite3_bind_int(stmt_, index, value));
    }

    void bind(int index, int64_t value) {
        check(sqlite3_bind_int64(stmt_, index, value));
    }

    void bind(int index, double value) {
        check(sqlite3_bind_double(stmt_, index, value));
    }

    void bind(int index, const std::string& value) {
        check(sqlite3_bind_text(stmt_, index, value.c_str(),
                                static_cast<int>(value.size()), SQLITE_TRANSIENT));
    }

    void bindNull(int index) {
        check(sqlite3_bind_null(stmt_, index));
    }

    /** @return true, если получена строка (SQLITE_ROW); false по завершении (SQLITE_DONE). */
    bool step() {
        const int rc = sqlite3_step(stmt_);
        if (rc == SQLITE_ROW) {
            return true;
        }
        if (rc == SQLITE_DONE) {
            return false;
        }
        throw std::runtime_error("SQL step failed: " + error());
    }

    /** Выполнить запрос, который не должен возвращать строки. */
    void exec() {
        if (step()) {
            throw std::runtime_error("SQL statement unexpectedly returned rows");
        }
    }

    void reset() {
        sqlite3_reset(stmt_);
        sqlite3_clear_bindings(stmt_);
    }

    [[nodiscard]] int columnInt(int index) const { return sqlite3_column_int(stmt_, index); }
    [[nodiscard]] int64_t columnInt64(int index) const { return sqlite3_column_int64(stmt_, index); }
    [[nodiscard]] double columnDouble(int index) const { return sqlite3_column_double(stmt_, index); }
    [[nodiscard]] bool columnIsNull(int index) const {
        return sqlite3_column_type(stmt_, index) == SQLITE_NULL;
    }

    [[nodiscard]] std::string columnText(int index) const {
        const unsigned char* text = sqlite3_column_text(stmt_, index);
        return text ? reinterpret_cast<const char*>(text) : std::string{};
    }

    [[nodiscard]] sqlite3_stmt* raw() const noexcept { return stmt_; }

private:
    void check(int rc) {
        if (rc != SQLITE_OK) {
            throw std::runtime_error("SQL bind failed: " + error());
        }
    }

    [[nodiscard]] std::string error() const {
        return db_ ? sqlite3_errmsg(db_) : "unknown error";
    }

    sqlite3* db_ = nullptr;
    sqlite3_stmt* stmt_ = nullptr;
};

} // namespace workout::infra