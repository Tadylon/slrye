#include <services/plugin/plugin_manifest.h>

#include <catch2/catch_test_macros.hpp>

#include <support/temporary_directory.h>

TEST_CASE("Plugin manifests load metadata and default the library to the plugin id")
{
    TemporaryDirectory directory;
    const auto manifest_path = directory.write_file("plugin.yaml", R"(
id: com.example.material
name: Material Tools
version: 1.2.3
author: Example Studio
description: Material authoring tools
enabled: false
)");

    std::string error;
    const std::optional<slrye::PluginManifest> manifest =
        slrye::load_plugin_manifest(manifest_path, &error);

    REQUIRE(manifest.has_value());
    CHECK(error.empty());
    CHECK(manifest->id == "com.example.material");
    CHECK(manifest->name == "Material Tools");
    CHECK(manifest->version == "1.2.3");
    CHECK(manifest->library == manifest->id);
    CHECK_FALSE(manifest->enabled);
}

TEST_CASE("Plugin manifests keep an explicit library name")
{
    TemporaryDirectory directory;
    const auto manifest_path = directory.write_file("plugin.yaml", R"(
id: com.example.renderer
library: custom_renderer
)");

    const std::optional<slrye::PluginManifest> manifest =
        slrye::load_plugin_manifest(manifest_path);

    REQUIRE(manifest.has_value());
    CHECK(manifest->library == "custom_renderer");
    CHECK(manifest->enabled);
}

TEST_CASE("Plugin manifests reject missing ids and malformed YAML")
{
    TemporaryDirectory directory;
    const auto missing_id_path = directory.write_file("missing_id/plugin.yaml", "name: Missing Id\n");
    std::string missing_id_error;
    CHECK_FALSE(slrye::load_plugin_manifest(missing_id_path, &missing_id_error).has_value());
    CHECK_FALSE(missing_id_error.empty());

    const auto malformed_path = directory.write_file("malformed/plugin.yaml", "id: [unterminated\n");
    std::string malformed_error;
    CHECK_FALSE(slrye::load_plugin_manifest(malformed_path, &malformed_error).has_value());
    CHECK_FALSE(malformed_error.empty());
}
