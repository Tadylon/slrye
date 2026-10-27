#include <domain/entity_id.h>

#include <catch2/catch_test_macros.hpp>

TEST_CASE("EntityId distinguishes stable identities from the invalid default")
{
    const slrye::EntityId invalid;
    const slrye::EntityId first(42);
    const slrye::EntityId same_as_first(42);
    const slrye::EntityId second(43);

    CHECK_FALSE(invalid.is_valid());
    CHECK_FALSE(static_cast<bool>(invalid));
    CHECK(first.is_valid());
    CHECK(first == same_as_first);
    CHECK(first != second);
    CHECK(first.value() == 42);
}
