#pragma once

#include <dpp/snowflake.h>
#include <mariadb/mysql.h>
#include <string>

namespace psdns {
  class storage final 
  {
    MYSQL *m_con = nullptr;
    
  public:
    /**
     * constructor / connects to the database
     * 
     * @param host the database host
     * @param user the database user
     * @param pass the database user password
     * @param db  the database name
     */
    storage(
      const std::string&, 
      const std::string&,
      const std::string&,
      const std::string&);
    
    storage(const storage&) = delete;
    storage(storage&&) = delete;
    auto operator=(const storage&) = delete;

    /**
     * destructor / closes the databasebase connection
     * 
     */
    ~storage();

    /**
     * inserts/updates the given author/content
     * 
     * @param id the message id
     * @param author the author string to use
     * @param content the content of the news
     */
    void insert(
      const dpp::snowflake&,
      const std::string&, 
      const std::string&) const;

    /**
     * removes a message (flags the message as delted)
     * 
     * @param id the message id
     */
    void remove(const dpp::snowflake&) const;
  };
}
