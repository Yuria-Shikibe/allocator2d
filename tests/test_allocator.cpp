#include <gtest/gtest.h>
#include <vector>
#include <random>
#include <algorithm>
#include <utility>
#include <cstdint>
#include <iostream>
#include <optional>
#include "mo_yanxi/allocator2d.hpp"

namespace mo_yanxi {

struct BlockInfo {
    mo_yanxi::math::vector2<std::uint32_t> pos;
    mo_yanxi::math::vector2<std::uint32_t> size;
};

struct TestConfig {
    std::string test_name;
    std::uint32_t map_size;
    int max_fill_attempts;
    std::pair<std::uint32_t, std::uint32_t> size_range; // {min, max}
};

std::ostream& operator<<(std::ostream& os, const TestConfig& config) {
    return os << config.test_name;
}

class AllocatorTest : public testing::TestWithParam<TestConfig> {
protected:
    void RunSimulation(const TestConfig& config) {
        mo_yanxi::allocator2d_checked<> alloc(mo_yanxi::math::vector2<std::uint32_t>{config.map_size, config.map_size});
        std::vector<BlockInfo> active_blocks;
        std::mt19937 rng(42); // Fixed seed for reproducibility
        std::uniform_int_distribution<std::uint32_t> size_dist(config.size_range.first, config.size_range.second);

        // Phase 1: Random Allocation Fill
        std::size_t used_area = 0;
        int alloc_count = 0;
        for (int i = 0; i < config.max_fill_attempts; ++i) {
            std::uint32_t w = size_dist(rng);
            std::uint32_t h = size_dist(rng);
            auto size = mo_yanxi::math::vector2<std::uint32_t>{w, h};

            auto result = alloc.allocate(size);
            if (result) {
                used_area += w * h;
                active_blocks.push_back({*result, size});
                alloc_count++;
            }
        }

        // Verify area consistency
        EXPECT_EQ(used_area + alloc.remain_area(), alloc.extent().area()) << "Phase 1: Area mismatch";

        // Phase 2: Fragment (Free 50%)
        std::ranges::shuffle(active_blocks, rng);
        std::size_t remove_count = active_blocks.size() / 2;
        std::vector<BlockInfo> remaining_blocks;

        for (std::size_t i = 0; i < active_blocks.size(); ++i) {
            if (i < remove_count) {
                bool success = alloc.deallocate(active_blocks[i].pos);
                EXPECT_TRUE(success) << "Phase 2: Failed to deallocate at " << active_blocks[i].pos.x << "," << active_blocks[i].pos.y;
                if (success) {
                    used_area -= active_blocks[i].size.area();
                }
            } else {
                remaining_blocks.push_back(active_blocks[i]);
            }
        }
        active_blocks = std::move(remaining_blocks);
        EXPECT_EQ(used_area + alloc.remain_area(), alloc.extent().area()) << "Phase 2: Area mismatch";

        // Phase 3: Refill with smaller blocks
        std::uniform_int_distribution<std::uint32_t> small_size_dist(5, config.size_range.first + 5);
        int attempts = config.max_fill_attempts / 2;
        auto last_area = alloc.remain_area();

        for (int i = 0; i < attempts; ++i) {
            std::uint32_t w = small_size_dist(rng);
            std::uint32_t h = small_size_dist(rng);
            auto size = mo_yanxi::math::vector2<std::uint32_t>{w, h};

            auto result = alloc.allocate(size);
            if (result) {
                used_area += w * h;
                active_blocks.push_back({*result, size});
                EXPECT_EQ(alloc.remain_area(), last_area - w * h) << "Phase 3: Area calculation error";
                last_area = last_area - w * h;
            }
        }
        EXPECT_EQ(used_area + alloc.remain_area(), alloc.extent().area()) << "Phase 3: Area mismatch";

        // Phase 4: Partial Clear (Free 30%)
        std::ranges::shuffle(active_blocks, rng);
        remove_count = static_cast<std::size_t>(active_blocks.size() * 0.3);
        remaining_blocks.clear();

        for (std::size_t i = 0; i < active_blocks.size(); ++i) {
            if (i < remove_count) {
                bool success = alloc.deallocate(active_blocks[i].pos);
                EXPECT_TRUE(success) << "Phase 4: Failed to deallocate";
                if (success) {
                    used_area -= active_blocks[i].size.area();
                }
            } else {
                remaining_blocks.push_back(active_blocks[i]);
            }
        }
        active_blocks = std::move(remaining_blocks);
        EXPECT_EQ(used_area + alloc.remain_area(), alloc.extent().area()) << "Phase 4: Area mismatch";

        // Phase 5: Full Clear and Verify
        for (const auto& b : active_blocks) {
            bool success = alloc.deallocate(b.pos);
            EXPECT_TRUE(success) << "Phase 5: Failed to release block";
        }
        active_blocks.clear();

        std::uint32_t total_area = config.map_size * config.map_size;
        EXPECT_EQ(alloc.remain_area(), total_area) << "Phase 5: Memory leak detected";

        // Perfect merge verification
        auto huge_block = alloc.allocate({config.map_size, config.map_size});
        EXPECT_TRUE(huge_block.has_value()) << "Phase 5: Perfect merge failed (fragmentation issue)";
        if (huge_block) {
            alloc.deallocate(*huge_block);
        }
    }
};

TEST_P(AllocatorTest, Simulation) {
    RunSimulation(GetParam());
}

INSTANTIATE_TEST_SUITE_P(
    AllConfigs,
    AllocatorTest,
    testing::Values(
        TestConfig{"Standard", 2048, 10000, {32, 256}},
        TestConfig{"HighFragment", 1024, 10000, {4, 16}},
        TestConfig{"Aligned", 1024, 10000, {16, 16}}
    )
);

} // namespace mo_yanxi
