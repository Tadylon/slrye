#include <engine/plugin/plugin_manager.h>

#include <catch2/catch_test_macros.hpp>

#include <filesystem>
#include <string>
#include <vector>

#ifndef SLRYE_DEMO_PLUGIN_DIR
#error SLRYE_DEMO_PLUGIN_DIR must point at the built demo plugin directory
#endif

TEST_CASE("Plugin manager loads and shuts down the real demo plugin")
{
    std::vector<std::string> log_messages;
    slrye::PluginManager& manager = slrye::PluginManager::instance();
    slrye::PluginManager& same_manager = slrye::PluginManager::instance();
    manager.shutdown();
    manager.set_logger([&log_messages](const std::string& message)
    {
        log_messages.push_back(message);
    });

    CHECK(&manager == &same_manager);
    REQUIRE(manager.load_plugin(std::filesystem::path(SLRYE_DEMO_PLUGIN_DIR)));
    REQUIRE(manager.plugins().size() == 1);
    CHECK(manager.plugins().front().manifest.id == "com.slrye.demo");
    CHECK(manager.plugins().front().capabilities != SLRYE_PLUGIN_CAPABILITY_NONE);
    CHECK_FALSE(log_messages.empty());

    manager.shutdown();
    CHECK(manager.plugins().empty());
    manager.set_logger({});
}
