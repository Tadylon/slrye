#include <platform/file_io.h>

#include <catch2/catch_test_macros.hpp>

#include <filesystem>
#include <string>

#include <support/temporary_directory.h>

TEST_CASE("Atomic text files can be written, replaced, and read")
{
    TemporaryDirectory directory;
    const std::filesystem::path path = directory.path() / "nested" / "settings.yaml";

    std::string error;
    REQUIRE(slrye::platform::write_text_file_atomic(path, "first\n", &error));
    CHECK(error.empty());

    std::string contents;
    REQUIRE(slrye::platform::read_text_file(path, contents, &error));
    CHECK(contents == "first\n");

    REQUIRE(slrye::platform::write_text_file_atomic(path, "second\n", &error));
    REQUIRE(slrye::platform::read_text_file(path, contents, &error));
    CHECK(contents == "second\n");
    CHECK_FALSE(std::filesystem::exists(path.string() + ".tmp"));
}

TEST_CASE("Reading a missing text file reports an error")
{
    TemporaryDirectory directory;
    std::string contents;
    std::string error;

    CHECK_FALSE(slrye::platform::read_text_file(
        directory.path() / "missing.yaml",
        contents,
        &error));
    CHECK_FALSE(error.empty());
}
