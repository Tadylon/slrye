#include <services/project/recent_projects.h>

#include <catch2/catch_test_macros.hpp>

#include <filesystem>
#include <vector>

#include <support/temporary_directory.h>

TEST_CASE("Recent projects are unique, newest-first, and bounded")
{
    TemporaryDirectory directory;
    const std::filesystem::path first = directory.path() / "first.slproj";
    const std::filesystem::path second = directory.path() / "second.slproj";
    const std::filesystem::path third = directory.path() / "third.slproj";

    slrye::RecentProjects recent(2);
    CHECK(recent.add(first));
    CHECK(recent.add(second));
    CHECK(recent.add(third));
    REQUIRE(recent.entries().size() == 2);
    CHECK(recent.entries()[0] == third);
    CHECK(recent.entries()[1] == second);

    CHECK(recent.add(first));
    REQUIRE(recent.entries().size() == 2);
    CHECK(recent.entries()[0] == first);
    CHECK(recent.entries()[1] == third);
    CHECK_FALSE(recent.add(first));

    CHECK(recent.remove(third));
    CHECK_FALSE(recent.remove(third));
    REQUIRE(recent.entries().size() == 1);
    CHECK(recent.entries()[0] == first);
}

TEST_CASE("Recent projects normalize duplicates before applying the limit")
{
    TemporaryDirectory directory;
    const std::filesystem::path project = directory.path() / "project.slproj";
    const std::filesystem::path duplicate =
        directory.path() / "nested" / ".." / "project.slproj";

    slrye::RecentProjects recent(5);
    recent.replace({project, duplicate, project});

    REQUIRE(recent.entries().size() == 1);
    CHECK(recent.entries()[0] == project.lexically_normal());
}
