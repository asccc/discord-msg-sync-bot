#include "storage.hpp"

#include <cstdlib>
#include <dpp/dispatcher.h>
#include <dpp/dpp.h>
#include <dpp/snowflake.h>

int main(void)
{
  auto store = psdns::storage{
    std::getenv("DB_HOST"), std::getenv("DB_USERNAME"), 
    std::getenv("DB_PASSWORD"), std::getenv("DB_NAME")};

  auto channel = dpp::snowflake{std::getenv("CHANNEL_ID")};

  auto bot = dpp::cluster{
    std::getenv("DISCORD_TOKEN"), 
    dpp::i_guild_messages | dpp::i_message_content};

#ifndef NDEBUG
  bot.on_log(dpp::utility::cout_logger());
#endif

  bot.on_message_create([&](const dpp::message_create_t& event) {
    if (event.msg.channel_id != channel) {
      return; // do not handle
    }
    store.insert(
      event.msg.id,
      event.msg.author.username, 
      event.msg.content);
  });

  bot.on_message_update([&](const dpp::message_update_t& event) {
    if (event.msg.channel_id != channel) {
      return; // do not handle
    }
    store.insert(
      event.msg.id, 
      event.msg.author.username,
      event.msg.content);
  });

  bot.on_message_delete([&](const dpp::message_delete_t& event) {
    if (event.channel_id != channel) {
      return; // do not handle
    }
    store.remove(event.id);
  });

  bot.on_message_delete_bulk([&](const dpp::message_delete_bulk_t& event) {
    if (event.deleting_channel.id != channel) {
      return; // do not handle
    }
    store.remove_all(event.deleted);
  });

  bot.start(dpp::st_wait);
  return 0;
}
