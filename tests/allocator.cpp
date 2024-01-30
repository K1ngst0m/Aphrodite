#include <catch2/catch_test_macros.hpp>
#include "allocator/allocator.h"

using namespace aph::memory;

TEST_CASE("Allocator functionality", "[allocator]")
{
    SECTION("malloc_internal and free_internal")
    {
        void* ptr = aph_malloc(10 * KB);  // Allocating 10KB
        REQUIRE(ptr != nullptr);
        aph_free(ptr);
    }

    SECTION("memalign_internal and free_internal")
    {
        void* ptr = aph_memalign(64, 10 * KB);  // Allocating 10KB with 64-byte alignment
        REQUIRE(ptr != nullptr);
        REQUIRE(reinterpret_cast<uintptr_t>(ptr) % 64 == 0);  // Check alignment
        aph_free(ptr);
    }

    SECTION("calloc_internal and free_internal")
    {
        void* ptr = aph_calloc(256, 40);  // Allocating space for 256 items of 40 bytes each
        REQUIRE(ptr != nullptr);
        // Optionally, check that memory is zeroed
        for(size_t i = 0; i < 256 * 40; ++i)
        {
            REQUIRE(reinterpret_cast<char*>(ptr)[i] == 0);
        }
        aph_free(ptr);
    }

    SECTION("calloc_memalign_internal and free_internal")
    {
        void* ptr =
            aph_calloc_memalign(256, 64, 40);  // Allocating space for 256 items of 40 bytes each with 64-byte alignment
        REQUIRE(ptr != nullptr);
        REQUIRE(reinterpret_cast<uintptr_t>(ptr) % 64 == 0);  // Check alignment
        // Optionally, check that memory is zeroed
        for(size_t i = 0; i < 256 * 40; ++i)
        {
            REQUIRE(reinterpret_cast<char*>(ptr)[i] == 0);
        }
        aph_free(ptr);
    }

    SECTION("realloc_internal and free_internal")
    {
        void* ptr = aph_malloc(10 * KB);  // Allocating 10KB
        REQUIRE(ptr != nullptr);
        ptr = aph_realloc(ptr, 20 * KB);  // Reallocating to 20KB
        REQUIRE(ptr != nullptr);
        aph_free(ptr);
    }

    SECTION("new_internal and delete_internal")
    {
        struct TestStruct
        {
            int x, y, z;
        };

        TestStruct* ptr = aph_new(TestStruct, 1, 2, 3);
        REQUIRE(ptr != nullptr);
        REQUIRE(ptr->x == 1);
        REQUIRE(ptr->y == 2);
        REQUIRE(ptr->z == 3);
        aph_delete(ptr);
    }
}

#include "allocator/alignedAlloc.h"

TEST_CASE("Aligned Allocator functionality", "[allocator]")
{
    using namespace aph;

    SECTION("memAlignAlloc and memAlignFree")
    {
        void* ptr = memAlignAlloc(64, 1024);  // Allocating 1024 bytes with 64-byte alignment
        REQUIRE(ptr != nullptr);
        REQUIRE(reinterpret_cast<uintptr_t>(ptr) % 64 == 0);  // Check alignment
        memAlignFree(ptr);
    }

    SECTION("memAlignCalloc and memAlignFree")
    {
        void* ptr = memAlignCalloc(64, 1024);  // Allocating and zeroing 1024 bytes with 64-byte alignment
        REQUIRE(ptr != nullptr);
        REQUIRE(reinterpret_cast<uintptr_t>(ptr) % 64 == 0);  // Check alignment
        // Optionally, check that memory is zeroed
        for(size_t i = 0; i < 1024; ++i)
        {
            REQUIRE(reinterpret_cast<char*>(ptr)[i] == 0);
        }
        memAlignFree(ptr);
    }

    SECTION("AlignedDeleter")
    {
        void* ptr = memAlignAlloc(64, 1024);  // Allocating 1024 bytes with 64-byte alignment
        REQUIRE(ptr != nullptr);
        AlignedDeleter deleter;
        deleter(ptr);  // This should not throw
    }

    SECTION("AlignedAllocation")
    {
        struct TestStruct : public AlignedAllocation<TestStruct>
        {
            int x, y, z;
        };

        TestStruct* ptr = new TestStruct();
        REQUIRE(ptr != nullptr);
        delete ptr;

        TestStruct* arr = new TestStruct[10];
        REQUIRE(arr != nullptr);
        delete[] arr;
    }
}
