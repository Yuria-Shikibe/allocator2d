#include <gtest/gtest.h>

#include <algorithm>
#include <cstdint>
#include <random>
#include <utility>
#include <vector>

#include "include/mo_yanxi/allocator2d.hpp"

using mo_yanxi::math::usize2;

TEST(Allocator2D, AllocateAndReleaseRestoresArea) {
    mo_yanxi::allocator2d<> alloc{{256, 256}};

    auto a = alloc.allocate({32, 64});
    ASSERT_TRUE(a.has_value());
    auto b = alloc.allocate({48, 16});
    ASSERT_TRUE(b.has_value());

    EXPECT_TRUE(alloc.deallocate(*a));
    EXPECT_TRUE(alloc.deallocate(*b));
    EXPECT_EQ(alloc.remain_area(), alloc.extent().area());
}

TEST(Allocator2D, PerfectMergeAfterFragmentation) {
    mo_yanxi::allocator2d<> alloc{{128, 128}};
    std::vector<usize2> sizes{{16, 16}, {32, 16}, {16, 32}, {24, 24}, {8, 40}};
    std::vector<usize2> positions;
    positions.reserve(sizes.size());

    for (const auto& size : sizes) {
        auto pos = alloc.allocate(size);
        ASSERT_TRUE(pos.has_value());
        positions.push_back(*pos);
    }

    std::ranges::reverse(positions);
    for (const auto& pos : positions) {
        EXPECT_TRUE(alloc.deallocate(pos));
    }

    EXPECT_EQ(alloc.remain_area(), alloc.extent().area());
    auto whole = alloc.allocate({128, 128});
    EXPECT_TRUE(whole.has_value());
}

TEST(Allocator2D, RejectsOversizedAllocation) {
    mo_yanxi::allocator2d<> alloc{{64, 64}};
    EXPECT_FALSE(alloc.allocate({65, 1}).has_value());
    EXPECT_FALSE(alloc.allocate({1, 65}).has_value());
    EXPECT_FALSE(alloc.allocate({0, 0}).has_value());
    EXPECT_EQ(alloc.remain_area(), alloc.extent().area());
}

TEST(Allocator2D, ReusesFreedNonLeafByFurtherSplitting) {
    mo_yanxi::allocator2d<> alloc{{64, 64}};

    auto a = alloc.allocate({32, 32});
    ASSERT_TRUE(a.has_value());
    EXPECT_EQ(*a, (usize2{0, 0}));

    auto b = alloc.allocate({32, 64});
    ASSERT_TRUE(b.has_value());
    EXPECT_EQ(*b, (usize2{32, 0}));

    auto c = alloc.allocate({32, 32});
    ASSERT_TRUE(c.has_value());
    EXPECT_EQ(*c, (usize2{0, 32}));

    EXPECT_TRUE(alloc.deallocate(*a));

    auto d = alloc.allocate({16, 16});
    auto e = alloc.allocate({16, 16});
    auto f = alloc.allocate({16, 16});
    auto g = alloc.allocate({16, 16});

    ASSERT_TRUE(d.has_value());
    ASSERT_TRUE(e.has_value());
    ASSERT_TRUE(f.has_value());
    ASSERT_TRUE(g.has_value());

    EXPECT_EQ(*d, (usize2{0, 0}));
    EXPECT_EQ(*e, (usize2{0, 16}));
    EXPECT_EQ(*f, (usize2{16, 0}));
    EXPECT_EQ(*g, (usize2{16, 16}));
    EXPECT_FALSE(alloc.allocate({1, 1}).has_value());

    EXPECT_TRUE(alloc.deallocate(*d));
    EXPECT_TRUE(alloc.deallocate(*e));
    EXPECT_TRUE(alloc.deallocate(*f));
    EXPECT_TRUE(alloc.deallocate(*g));

    auto whole = alloc.allocate({32, 32});
    ASSERT_TRUE(whole.has_value());
    EXPECT_EQ(*whole, (usize2{0, 0}));

    EXPECT_TRUE(alloc.deallocate(*whole));
    EXPECT_TRUE(alloc.deallocate(*b));
    EXPECT_TRUE(alloc.deallocate(*c));
    EXPECT_EQ(alloc.remain_area(), alloc.extent().area());

    auto full = alloc.allocate({64, 64});
    EXPECT_TRUE(full.has_value());
}

TEST(Allocator2D, MovePreservesNestedAllocatorLinks) {
    mo_yanxi::allocator2d<> alloc{{64, 64}};

    auto a = alloc.allocate({32, 32});
    auto b = alloc.allocate({32, 64});
    auto c = alloc.allocate({32, 32});

    ASSERT_TRUE(a.has_value());
    ASSERT_TRUE(b.has_value());
    ASSERT_TRUE(c.has_value());

    EXPECT_TRUE(alloc.deallocate(*a));

    auto d = alloc.allocate({16, 16});
    auto e = alloc.allocate({16, 16});
    auto f = alloc.allocate({16, 16});
    auto g = alloc.allocate({16, 16});

    ASSERT_TRUE(d.has_value());
    ASSERT_TRUE(e.has_value());
    ASSERT_TRUE(f.has_value());
    ASSERT_TRUE(g.has_value());

    auto moved = std::move(alloc);

    EXPECT_TRUE(moved.deallocate(*d));
    EXPECT_TRUE(moved.deallocate(*e));
    EXPECT_TRUE(moved.deallocate(*f));
    EXPECT_TRUE(moved.deallocate(*g));
    EXPECT_TRUE(moved.deallocate(*b));
    EXPECT_TRUE(moved.deallocate(*c));
    EXPECT_EQ(moved.remain_area(), moved.extent().area());

    auto full = moved.allocate({64, 64});
    ASSERT_TRUE(full.has_value());
    EXPECT_EQ(*full, (usize2{0, 0}));
}
