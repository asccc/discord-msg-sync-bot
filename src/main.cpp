#include "storage.hpp"

#include <cstdlib>
#include <dpp/dispatcher.h>
#include <dpp/dpp.h>
#include <dpp/snowflake.h>

int main(void)
{
  auto store = psdns::storage{
    std::getenv("DB_HOST"),
    std::getenv("DB_USERNAME"), 
    std::getenv("DB_PASSWORD"), 
    std::getenv("DB_NAME")};

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

    const auto& id = event.msg.id;

    // handle snapshots
    if (event.msg.has_snapshot()) {
      const auto& snapshots = event.msg.message_snapshots;
      for (const auto& message : snapshots.messages) {
        auto content = message.content;
        auto author = message.author.username.empty() ?
          event.msg.author.username :
          message.author.username;

        if (!message.embeds.empty()) {
          // use embed author?
          content += message.embeds[0].description;
        }

        store.insert(id, author, content);
      }
      return;
    }

    // handle normal messages
    const auto& author = event.msg.author.username;
    auto content = event.msg.content;

    if (!event.msg.embeds.empty()) {
      // use embed author?
      content += event.msg.embeds[0].description;
    }

    store.insert(id, author, content);
  });

  bot.on_message_update([&](const dpp::message_update_t& event) {
    if (event.msg.channel_id != channel) {
      return; // do not handle
    }
    
    // only normal messages can update
    const auto& id = event.msg.id;
    const auto& author = event.msg.author.username;
    auto content = event.msg.content;

    if (!event.msg.embeds.empty()) {
      // use embed author?
      content += event.msg.embeds[0].description;
    }

    store.insert(id, author, content);
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
