#include "storage.hpp"

#include <dpp/snowflake.h>
#include <mariadb/mariadb_com.h>
#include <mariadb/mysql.h>
#include <mutex>
#include <stdexcept>
#include <string>
#include <cstring>
#include <vector>

namespace psdns {

  /**
   * constructor / connects to the database
   * 
   * @param host the database host
   * @param user the database user
   * @param pass the database user password
   * @param db  the database name
   */
  storage::storage(
    const std::string& host,
    const std::string& user,
    const std::string& pass,
    const std::string& db)
  {
    MYSQL *con = nullptr;
    if (!(con = mysql_init(nullptr))) {
      throw std::runtime_error{"could init mysql"};
    }

    if (0 != mysql_options(con, MYSQL_SET_CHARSET_NAME, "utf8mb4")) {
      auto err = std::string{mysql_error(con)};
      mysql_close(con);
      throw std::runtime_error{"could not set default charset: " + err};
    }

    bool reconnect = true;
    mysql_options(con, MYSQL_OPT_RECONNECT, &reconnect);

    auto host_s = host.c_str();
    auto user_s = user.c_str();
    auto pass_s = pass.c_str();
    auto db_s = db.c_str();

    auto res = mysql_real_connect(
      con, host_s, user_s, pass_s, db_s, 
      3306, nullptr, 0);

    if (!res) {
      auto err = std::string{mysql_error(con)};
      mysql_close(con);
      throw std::runtime_error("could connect to database: " + err);
    }

    m_con = con;
  }

  /**
   * destructor / closes the databasebase connection
   * 
   */
  storage::~storage() 
  {
    if (m_con) {
      mysql_close(m_con);
      m_con = nullptr;
    }
  }

  /**
   * inserts the given author/content to the news table
   * 
   * @param id the message id
   * @param author the author string to use
   * @param content the content of the news
   */
  void storage::insert(
    const dpp::snowflake& id,
    const std::string& author, 
    const std::string& content)
  {
    const auto lock = std::lock_guard<std::mutex>{m_mutex};

    if (0 != mysql_ping(m_con)) {
      throw std::runtime_error{"mysql connection lost"};
    }

    auto stmt = mysql_stmt_init(m_con);
    if (!stmt) {
      throw std::runtime_error{"could not init mysql statement"};
    }

    auto query =
      "INSERT INTO news (id, server_id, author, content, created_at) "
      "VALUES (?, 1, ?, ?, UNIX_TIMESTAMP()) "
      "ON DUPLICATE KEY UPDATE "
        "content = VALUES(content), "
        "updated_at = UNIX_TIMESTAMP()";

    if (0 != mysql_stmt_prepare(stmt, query, std::strlen(query))) {
      auto err = std::string{mysql_stmt_error(stmt)};
      mysql_stmt_close(stmt);
      throw std::runtime_error{"could not prepare statement: " + err};
    }

    MYSQL_BIND bind[3];
    std::memset(bind, 0, sizeof(bind));

    auto id_str = std::to_string(id);
    auto id_len = id_str.length();
    bind[0].buffer_type = MYSQL_TYPE_STRING;
    bind[0].buffer = const_cast<char*>(id_str.c_str());
    bind[0].length = &id_len;
    
    auto author_len = author.length();
    bind[1].buffer_type = MYSQL_TYPE_STRING;
    bind[1].buffer = const_cast<char*>(author.c_str());
    bind[1].length = &author_len;

    auto content_len = content.length();
    bind[2].buffer_type = MYSQL_TYPE_STRING;
    bind[2].buffer = const_cast<char*>(content.c_str());
    bind[2].length = &content_len;

    mysql_stmt_bind_param(stmt, bind);
    if (0 != mysql_stmt_execute(stmt)) {
      auto err = std::string{mysql_stmt_error(stmt)};
      mysql_stmt_close(stmt);
      throw std::runtime_error{"error while executing statement: " + err};
    }

    mysql_stmt_close(stmt);
  }

  /**
   * removes a message (flags the message as delted)
   * 
   * @param id the message id
   */
  void storage::remove(const dpp::snowflake& id)
  {
    const auto lock = std::lock_guard<std::mutex>{m_mutex};

    if (0 != mysql_ping(m_con)) {
      throw std::runtime_error{"mysql connection lost"};
    }

    auto stmt = mysql_stmt_init(m_con);
    if (!stmt) {
      throw std::runtime_error{"could not init mysql statement"};
    }

    auto query =
      "UPDATE news SET is_deleted = TRUE, deleted_at = UNIX_TIMESTAMP() "
      "WHERE id = ? AND server_id = 1";

    if (0 != mysql_stmt_prepare(stmt, query, std::strlen(query))) {
      auto err = std::string{mysql_stmt_error(stmt)};
      mysql_stmt_close(stmt);
      throw std::runtime_error{"could not prepare statement: " + err};
    }

    MYSQL_BIND bind[1];
    std::memset(bind, 0, sizeof(bind));

    auto id_str = std::to_string(id);
    auto id_len = id_str.length();
    bind[0].buffer_type = MYSQL_TYPE_STRING;
    bind[0].buffer = const_cast<char*>(id_str.c_str());
    bind[0].length = &id_len;

    mysql_stmt_bind_param(stmt, bind);
    if (0 != mysql_stmt_execute(stmt)) {
      auto err = std::string{mysql_stmt_error(stmt)};
      mysql_stmt_close(stmt);
      throw std::runtime_error{"error while executing statement: " + err};
    }

    mysql_stmt_close(stmt);
  }

  /**
   * removes multiple messages
   * 
   * @param ids the message ids
   */
  void storage::remove_all(const std::vector<dpp::snowflake>& ids)
  {
    if (ids.empty()) {
      // nothing to do
      return;
    }
    
    const auto lock = std::lock_guard<std::mutex>{m_mutex};

    if (0 != mysql_ping(m_con)) {
      throw std::runtime_error{"mysql connection lost"};
    }

    auto stmt = mysql_stmt_init(m_con);
    if (!stmt) {
      throw std::runtime_error{"could not init mysql statement"};
    }

    auto query =
      "UPDATE news SET is_deleted = TRUE, deleted_at = UNIX_TIMESTAMP() "
      "WHERE id = ? AND server_id = 1";

    if (0 != mysql_stmt_prepare(stmt, query, std::strlen(query))) {
      auto err = std::string{mysql_stmt_error(stmt)};
      mysql_stmt_close(stmt);
      throw std::runtime_error{"could not prepare statement: " + err};
    }

    MYSQL_BIND bind[1];
    std::memset(bind, 0, sizeof(bind));

    for (const auto& id : ids) {
      auto id_str = std::to_string(id);
      auto id_len = id_str.length();
      bind[0].buffer_type = MYSQL_TYPE_STRING;
      bind[0].buffer = const_cast<char*>(id_str.c_str());
      bind[0].length = &id_len;

      mysql_stmt_bind_param(stmt, bind);
      if (0 != mysql_stmt_execute(stmt)) {
        auto err = std::string{mysql_stmt_error(stmt)};
        mysql_stmt_close(stmt);
        throw std::runtime_error{"error while executing statement: " + err};
      }
    }

    mysql_stmt_close(stmt);
  }
}
