#include <catch2/catch_test_macros.hpp>
#include "module/module.h"

using namespace aph;

const char* test_module = "/lib64/libc.so.6";

TEST_CASE("Module Constructors and Destructor", "[Module]")
{
    SECTION("Default Constructor")
    {
        Module mod;
        REQUIRE(!mod);  // Module should be in a falsey state
    }

    SECTION("Parameterized Constructor")
    {
        Module mod(test_module);
        REQUIRE(mod);  // Module should be in a truthy state
    }

    SECTION("Move Constructor")
    {
        Module mod1(test_module);
        Module mod2(std::move(mod1));
        REQUIRE(mod2);   // mod2 should be in a truthy state
        REQUIRE(!mod1);  // mod1 should be in a falsey state
    }

    SECTION("Move Assignment Operator")
    {
        Module mod1(test_module);
        Module mod2;
        mod2 = std::move(mod1);
        REQUIRE(mod2);   // mod2 should be in a truthy state
        REQUIRE(!mod1);  // mod1 should be in a falsey state
    }
}

TEST_CASE("Module::getSymbol", "[Module]")
{
    Module mod("/lib64/libc.so.6");

    SECTION("Known Symbol")
    {
        auto known_func = mod.getSymbol<void* (*)(size_t)>("malloc");
        REQUIRE(known_func != nullptr);
    }

    SECTION("Unknown Symbol")
    {
        auto unknown_func = mod.getSymbol<void (*)()>("unknown_function");
        REQUIRE(unknown_func == nullptr);
    }
}

TEST_CASE("Module::operator bool", "[Module]")
{
    Module mod;

    SECTION("Unloaded Module")
    {
        REQUIRE(!mod);
    }

    SECTION("Loaded Module")
    {
        mod = Module(test_module);
        REQUIRE(mod);
    }
}
