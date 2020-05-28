#pragma once
#include "seeker/loggerApi.h"
#include "sqlite3.h"
#include <string>
#include <mutex>
#include <vector>

namespace seeker {
using std::function;
using std::mutex;
using std::string;
using std::vector;

class SqliteDB {
  const string dbFile;

  sqlite3* dbHandle = nullptr;
  mutex insertMutex;


  SqliteDB(const string& file) : dbFile(file) {
    if (file.length() == 0) {
      string msg = "Database should be init first.";
      E_LOG(msg);
      throw std::runtime_error(msg);
    }

    int rc = sqlite3_open(dbFile.c_str(), &dbHandle);
    if (rc) {
      E_LOG("Can't open database[{}]: {}", dbFile, sqlite3_errmsg(dbHandle));
      sqlite3_close(dbHandle);
      throw std::runtime_error("open database error.");
    }
  }

  static SqliteDB& getInstence(const string& file = "") {
    static SqliteDB instence{file};
    return instence;
  }

 public:
  SqliteDB(const SqliteDB&) = delete;
  SqliteDB& operator=(const SqliteDB&) = delete;

  ~SqliteDB() {
    if (dbHandle) {
      W_LOG("Sqlite3 Closing.");
      sqlite3_close(dbHandle);
    } else {
      W_LOG("No sqlite3 to close.");
    }
  }

  static void init(const string& file) {
    static bool inited = false;
    if (!inited) {
      inited = true;
      I_LOG("Init Database with file = {}", file);
      getInstence(file);
      I_LOG("Database inited success. db file = {}", file);
    } else {
      W_LOG("Database has been inited before, do nothing.");
    }
  }

  static void perpare(const string& sql, sqlite3_stmt** stmt) {
    auto& ins = getInstence();
    if (sqlite3_prepare_v2(ins.dbHandle, sql.c_str(), -1, stmt, nullptr) != SQLITE_OK) {
      string errMsg =
          fmt::format("Prepare SQL [{}], Error: {}", sql, sqlite3_errmsg(ins.dbHandle));
      W_LOG(errMsg);
      throw std::runtime_error(errMsg);
    }
  }
  
  static std::unique_ptr<sqlite3_stmt, function<void(sqlite3_stmt*)>> perpare(const string& sql) {
    static auto deletor = [](sqlite3_stmt* s) {
      I_LOG("deletor: sqlite3_finalize stmt.");
      sqlite3_finalize(s);
    };

    sqlite3_stmt* stmt = nullptr;
    auto& ins = getInstence();
    if (sqlite3_prepare_v2(ins.dbHandle, sql.c_str(), -1, &stmt, nullptr) != SQLITE_OK) {
      string errMsg =
          fmt::format("Prepare SQL [{}], Error: {}", sql, sqlite3_errmsg(ins.dbHandle));
      W_LOG(errMsg);
      throw std::runtime_error(errMsg);
    }

    std::unique_ptr<sqlite3_stmt, function<void(sqlite3_stmt*)>> rst{stmt, deletor};
    return rst;

  }


  static int exec(const string& sql,
                  int (*callback)(void*, int, char**, char**),
                  void* arg1,
                  string& err) {
    auto& ins = getInstence();
    //T_LOG("exec sql: {}.", sql);
    char* zErrMsg = nullptr;
    int rc = sqlite3_exec(ins.dbHandle, sql.c_str(), callback, arg1, &zErrMsg);
    if (zErrMsg != nullptr) {
      err = zErrMsg;
    }
    //T_LOG("exec rc=[{}] sql : {}.", rc, sql);
    return rc;
  }

  static int64_t lastInsertRowid() {
    auto& ins = getInstence();
    return sqlite3_last_insert_rowid(ins.dbHandle);
  }

  static mutex& getInsertMutex() {
    auto& ins = getInstence();
    return ins.insertMutex;
  }

  //TODO for debug.
  static vector<string> listTables();
  static string descTable(const string& tableName);
  static vector<string> getAllRowString(const string& tableName);
};

}  // namespace seeker