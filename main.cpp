#include <version>
//Used to pass CI

#ifdef ENABLE_TEST
#ifndef __cpp_lib_print
#error "Print Not Supported"
#endif
#endif

#ifdef ENABLE_TEST
#include <string>
#include <cstdint>
#include <algorithm>
#include <random>
#include <print>
#include <chrono> // [Added] Needed for timing
#endif

#include "include/mo_yanxi/allocator2d.hpp"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

//----------------------------------------------------------------------------------------------------
//  -TEST USAGE-    -TEST USAGE-    -TEST USAGE-    -TEST USAGE-    -TEST USAGE-    -TEST USAGE-
//----------------------------------------------------------------------------------------------------
#ifdef ENABLE_TEST

// [Added] Simple profiler to track duration stats
class perf_profiler {
    using clock = std::chrono::high_resolution_clock;
    using duration = std::chrono::nanoseconds;

    duration total_dur_{0};
    duration success_dur_{0};
    duration fail_dur_{0};

    std::size_t total_count_{0};
    std::size_t success_count_{0};
    std::size_t fail_count_{0};

public:
    void record(duration d, bool success) {
        total_dur_ += d;
        total_count_++;
        if (success) {
            success_dur_ += d;
            success_count_++;
        } else {
            fail_dur_ += d;
            fail_count_++;
        }
    }

    void print_stats(const std::string& label) const {
        auto to_ns_double = [](duration d, std::size_t count) {
            return count == 0 ? 0.0 : static_cast<double>(d.count()) / count;
        };

        std::println("  -> [Timing: {}]", label);
        std::println("     Total:   {:>8.2f} ns/op ({} ops)", to_ns_double(total_dur_, total_count_), total_count_);
        if (success_count_ > 0)
            std::println("     Success: {:>8.2f} ns/op ({} ops)", to_ns_double(success_dur_, success_count_), success_count_);
        if (fail_count_ > 0)
            std::println("     Fail:    {:>8.2f} ns/op ({} ops)", to_ns_double(fail_dur_, fail_count_), fail_count_);
    }
};

struct block_info {
    mo_yanxi::math::vector2<std::uint32_t> pos;
    mo_yanxi::math::vector2<std::uint32_t> size;
    std::uint8_t r, g, b;
};

class canvas {
    int width, height;
    std::vector<std::uint8_t> pixels;

public:
    canvas(int w, int h) : width(w), height(h), pixels(w * h * 3, 0) {}

    void draw_rect(int x, int y, int w, int h, std::uint8_t r, std::uint8_t g, std::uint8_t b) {
        for (int j = y; j < y + h; ++j) {
            for (int i = x; i < x + w; ++i) {
                if (i >= 0 && i < width && j >= 0 && j < height) {
                    int idx = (j * width + i) * 3;
                    if(r != 0)assert(pixels[idx] == 0);
                    if(g != 0)assert(pixels[idx + 1] == 0);
                    if(b != 0)assert(pixels[idx + 2] == 0);
                    pixels[idx] = r;
                    pixels[idx + 1] = g;
                    pixels[idx + 2] = b;
                }
            }
        }
    }

    void save(const std::string& filename) {
        if (stbi_write_png(filename.c_str(), width, height, 3, pixels.data(), width * 3)) {
            std::println("  -> Snapshot saved: {}", filename);
        } else {
            std::println(stderr, "  -> Error: Failed to save snapshot {}", filename);
        }
    }

    void clear() {
        std::ranges::fill(pixels, 0);
    }
};

class allocator_tester {
public:
    struct config {
        std::string test_name;
        std::uint32_t map_size;
        int max_fill_attempts; // Max fill attempts
        std::pair<std::uint32_t, std::uint32_t> size_range; // {min, max}
    };

private:
    config config_;
    mo_yanxi::allocator2d_checked<> alloc_;
    canvas canvas_;
    std::vector<block_info> active_blocks_;
    std::mt19937 rng_;
    std::uniform_int_distribution<std::uint32_t> size_dist_;
    std::uniform_int_distribution<int> color_dist_;

public:
    allocator_tester(config config)
        : config_(std::move(config)),
          alloc_(mo_yanxi::math::vector2<std::uint32_t>{config_.map_size, config_.map_size}),
          canvas_(config_.map_size, config_.map_size),
          rng_(std::random_device{}()),
          size_dist_(config_.size_range.first, config_.size_range.second),
          color_dist_(50, 255)
    {
        std::println("========================================");
        std::println("Test Suite Initialized: {}", config_.test_name);
        std::println("Canvas Size: {0}x{0}, Block Size Range: [{1}, {2}]",
            config_.map_size, config_.size_range.first, config_.size_range.second);
        std::println("========================================");
    }

    void run() {
        phase_1_fill();
        phase_2_fragment();
        phase_3_refill();
        phase_4_partial_clear();
        phase_5_full_clear_and_verify();
        std::println("\nTest [{}] Completed.\n", config_.test_name);
    }

private:
    std::string get_filename(const std::string& suffix) const {
        return std::format("{}_{}", config_.test_name, suffix);
    }

    void phase_1_fill() {
        std::println("[Phase 1] Random Allocation Fill...");
        perf_profiler profiler; // [Added]
        int count = 0;

        std::size_t used_area{};
        for (int i = 0; i < config_.max_fill_attempts; ++i) {
            std::uint32_t w = size_dist_(rng_);
            std::uint32_t h = size_dist_(rng_);
            auto size = mo_yanxi::math::vector2<std::uint32_t>{w, h};

            // [Modified] Timing logic
            auto start = std::chrono::high_resolution_clock::now();
            auto result = alloc_.allocate(size);
            auto end = std::chrono::high_resolution_clock::now();
            profiler.record(end - start, result.has_value());

            if (result) {
                used_area += w * h;
                std::uint8_t r = color_dist_(rng_);
                std::uint8_t g = color_dist_(rng_);
                std::uint8_t b = color_dist_(rng_);

                active_blocks_.push_back({*result, size, r, g, b});
                canvas_.draw_rect(result->x, result->y, w, h, r, g, b);
                count++;
            }
        }
        std::println("  -> Allocated {} blocks", count);
        profiler.print_stats("Alloc Fill"); // [Added]
        std::println("  -> Area Check: {} + {} = {} ?= {}", used_area, alloc_.remain_area(), used_area +  alloc_.remain_area(), alloc_.extent().area());
        canvas_.save(get_filename("01_allocated.png"));
    }

    void phase_2_fragment() {
        std::println("[Phase 2] Randomly freeing 50% of blocks to create fragmentation...");
        perf_profiler profiler; // [Added]
        std::vector<block_info> remaining_blocks;
        std::ranges::shuffle(active_blocks_, rng_);

        std::size_t remove_count = active_blocks_.size() / 2;
        std::size_t used_area{alloc_.extent().area() - alloc_.remain_area()};
        for (std::size_t i = 0; i < active_blocks_.size(); ++i) {
            if (i < remove_count) {
                // [Modified] Timing logic
                auto start = std::chrono::high_resolution_clock::now();
                bool success = alloc_.deallocate(active_blocks_[i].pos);
                auto end = std::chrono::high_resolution_clock::now();
                profiler.record(end - start, success);

                if (!success) {
                    std::println(stderr, "  -> Error: Failed to deallocate at {},{}", active_blocks_[i].pos.x, active_blocks_[i].pos.y);
                } else {
                    auto& b = active_blocks_[i];
                    canvas_.draw_rect(b.pos.x, b.pos.y, b.size.x, b.size.y, 0, 0, 0);
                    used_area -= b.size.area();
                }
            } else {
                remaining_blocks.push_back(active_blocks_[i]);
            }
        }

        std::println("  -> Area Check: {} + {} = {} ?= {}", used_area, alloc_.remain_area(), used_area +  alloc_.remain_area(), alloc_.extent().area());
        profiler.print_stats("Dealloc Fragment"); // [Added]
        active_blocks_ = std::move(remaining_blocks);
        canvas_.save(get_filename("02_fragmented.png"));
    }

    void phase_3_refill() {
        std::println("[Phase 3] Attempting to refill gaps with smaller blocks...");
        perf_profiler profiler; // [Added]
        // Use a smaller size distribution to test gap filling capability
        std::uniform_int_distribution<std::uint32_t> small_size_dist(5, config_.size_range.first + 5);

        int success_count = 0;
        int attempts = config_.max_fill_attempts / 2;

        std::size_t used_area{alloc_.extent().area() - alloc_.remain_area()};
        auto last_area = alloc_.remain_area();

        for (int i = 0; i < attempts; ++i) {
            std::uint32_t w = small_size_dist(rng_);
            std::uint32_t h = small_size_dist(rng_);
            auto size = mo_yanxi::math::vector2<std::uint32_t>{w, h};

            // [Modified] Timing logic
            auto start = std::chrono::high_resolution_clock::now();
            auto result = alloc_.allocate(size);
            auto end = std::chrono::high_resolution_clock::now();
            profiler.record(end - start, result.has_value());

            if (result) {
                used_area += w * h;
                std::uint8_t r = 255, g = 255, b = 255;
                canvas_.draw_rect(result->x, result->y, w, h, r, g, b);
                active_blocks_.push_back({*result, size, r, g, b});
                success_count++;
                if(alloc_.remain_area() != last_area - w * h){
                    throw std::runtime_error{"Area Mismatch"};
                }
                last_area = last_area - w * h;
            }
        }

        std::println("  -> Area Check: {} + {} = {} ?= {}", used_area, alloc_.remain_area(), used_area +  alloc_.remain_area(), alloc_.extent().area());
        std::println("  -> Successfully re-allocated {} new blocks", success_count);
        profiler.print_stats("Alloc Refill"); // [Added]
        canvas_.save(get_filename("03_refilled.png"));
    }

    void phase_4_partial_clear() {
        std::println("[Phase 4] Randomly freeing 30% of blocks again...");
        perf_profiler profiler; // [Added]
        std::ranges::shuffle(active_blocks_, rng_);
        std::vector<block_info> remaining_blocks;

        std::size_t used_area{alloc_.extent().area() - alloc_.remain_area()};
        std::size_t remove_count = static_cast<std::size_t>(active_blocks_.size() * 0.3);
        for (std::size_t i = 0; i < active_blocks_.size(); ++i) {
            if (i < remove_count) {
                // [Modified] Timing logic
                auto start = std::chrono::high_resolution_clock::now();
                bool success = alloc_.deallocate(active_blocks_[i].pos);
                auto end = std::chrono::high_resolution_clock::now();
                profiler.record(end - start, success);

                if(!success){
                    throw std::runtime_error{"Bad Dealloc"};
                }

                used_area -= active_blocks_[i].size.area();

                auto& b = active_blocks_[i];
                canvas_.draw_rect(b.pos.x, b.pos.y, b.size.x, b.size.y, 0, 0, 0);
            } else {
                remaining_blocks.push_back(active_blocks_[i]);
            }
        }
        active_blocks_ = std::move(remaining_blocks);
        std::println("  -> Area Check: {} + {} = {} ?= {}", used_area, alloc_.remain_area(), used_area +  alloc_.remain_area(), alloc_.extent().area());
        profiler.print_stats("Dealloc Partial"); // [Added]
        canvas_.save(get_filename("04_partial_clear.png"));
    }

    void phase_5_full_clear_and_verify() {
        std::println("[Phase 5] Performing full clear and final verification...");
        perf_profiler profiler; // [Added]

        // Only timing the deallocations, not the move logic
        for(const auto& b : active_blocks_) {
            auto start = std::chrono::high_resolution_clock::now();
            bool success = alloc_.deallocate(b.pos);
            auto end = std::chrono::high_resolution_clock::now();
            profiler.record(end - start, success);

            if(!success) {
                 std::println(stderr, "  -> Fatal Error: Failed to release block at {},{}", b.pos.x, b.pos.y);
            }
            canvas_.draw_rect(b.pos.x, b.pos.y, b.size.x, b.size.y, 0, 0, 0);
        }
        active_blocks_.clear();

        profiler.print_stats("Dealloc Final"); // [Added]

        // Verify status
        std::uint32_t total_area = config_.map_size * config_.map_size;
        std::println("  -> Final status check:");
        std::println("  -> Expected remaining area: {}", total_area);
        std::println("  -> Actual remaining area:   {}", alloc_.remain_area());

        if (alloc_.remain_area() == total_area) {
            std::println("  -> [PASS] Memory fully reclaimed, no leaks.");

            // Perfect merge verification
            auto huge_block = alloc_.allocate({config_.map_size, config_.map_size});
            if (huge_block) {
                std::println("  -> [PASS] Perfect merge verification: Successfully allocated full map block.");
                canvas_.draw_rect(0, 0, config_.map_size, config_.map_size, 0, 255, 0); // Green
                alloc_.deallocate({});

            } else {
                std::println("  -> [FAIL] Perfect merge verification: Cannot allocate full map block (fragmentation merge issue).");
                canvas_.draw_rect(0, 0, config_.map_size, config_.map_size, 255, 0, 0); // Red
            }
        } else {
            std::println("  -> [FAIL] Memory leak detection: Area mismatch, difference {}", total_area - alloc_.remain_area());
        }

        canvas_.save(get_filename("05_full_clear.png"));
    }
};

void test_fn(){
    {
        allocator_tester::config config{
            .test_name = "Standard",
            .map_size = 2048,
            .max_fill_attempts = 10000,
            .size_range = {32, 256}
        };
        allocator_tester tester(config);
        tester.run();
    }

    {
        allocator_tester::config config{
            .test_name = "HighFragment",
            .map_size = 1024,
            .max_fill_attempts = 10000,
            .size_range = {4, 16}
        };
        allocator_tester tester(config);
        tester.run();
    }
    {
        allocator_tester::config config{
            .test_name = "Aligned",
            .map_size = 1024,
            .max_fill_attempts = 10000,
            .size_range = {16, 16}
        };
        allocator_tester tester(config);
        tester.run();
    }

}

#endif


int main() {
#ifdef ENABLE_TEST
    test_fn();
#endif

    mo_yanxi::allocator2d_checked a{{256, 256}};
    const unsigned width = 32;
    const unsigned height = 64;
    if(const std::optional<mo_yanxi::math::usize2> where = a.allocate({width, height})){
        //Do something here...

        a.deallocate(where.value());
    }

}