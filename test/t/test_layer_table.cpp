
#include <test.hpp>

#include <vtzero/layer_table.hpp>

static_assert(std::is_nothrow_move_constructible<vtzero::layer_table<int>>::value, "layer_table is not nothrow move constructible");
static_assert(std::is_nothrow_move_assignable<vtzero::layer_table<int>>::value, "layer_table is not nothrow move assignable");

TEST_CASE("default constructed layer_table") {
    vtzero::layer_table<int> t;
    REQUIRE(t.empty());
    REQUIRE(t.size() == 0);
}

TEST_CASE("empty layer_table") {
    vtzero::layer_table<int> t{vtzero::data_view{}, 3};
    REQUIRE(t.empty());
    REQUIRE(t.size() == 0);
    REQUIRE_THROWS_AS(t.at(0), const vtzero::out_of_range_exception&);
    try {
        t.at(2);
    } catch (const vtzero::out_of_range_exception& e) {
        REQUIRE(std::string{e.what()} == "Index out of range: 2");
        REQUIRE(e.layer_num() == 3);

    }
}

TEST_CASE("layer_table with content") {
    std::array<int, 4> data{{10, 20, 30, 40}};

    std::array<char, 1 + sizeof(int) * data.size()> buffer;
    buffer[0] = 42;
    std::memcpy(buffer.data() + 1, data.data(), sizeof(int) * data.size());

    vtzero::layer_table<int> t{vtzero::data_view{&buffer[1], sizeof(int) * data.size()}, 3};
    REQUIRE_FALSE(t.empty());
    REQUIRE(t.size() == 4);
    REQUIRE(t.at(1) == 20);
    REQUIRE_THROWS_AS(t.at(5), const vtzero::out_of_range_exception&);
    REQUIRE_THROWS_WITH(t.at(5), "Index out of range: 5");
}

