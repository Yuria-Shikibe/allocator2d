# Evaluation of `mo_yanxi::allocator2d`

## 1. Overview
`mo_yanxi::allocator2d` is a header-only C++20 library for dynamic 2D rectangle allocation and deallocation. Unlike traditional texture packers (like MaxRects) which focus on packing existing items as tightly as possible once, this library focuses on the lifecycle of 2D memory: allocating and freeing blocks dynamically, similar to `malloc`/`free` but for 2D surfaces.

## 2. Functionality & Usage
*   **Interface**: Simple `allocate(size)` and `deallocate(position)` API.
*   **Integration**: Easy to integrate (single header/module).
*   **Dependencies**: Minimal (Standard Library).
*   **Design**: Uses a "Quad Split" strategy. When a block is allocated, the remaining space in the node is split into up to 4 new free rectangles (Top-Left, Top-Right, Bottom-Left, Bottom-Right relative to the allocated block).
*   **Deallocation**: Supports coalescing. When a block is freed, it attempts to merge with its "siblings" in the quad-split tree to reform larger free blocks.

## 3. Performance Analysis
Based on the provided benchmarks and local testing:
*   **Allocation**: Reasonably fast. It uses `std::map`/`multimap` to find suitable free blocks, implying logarithmic complexity with respect to the number of free fragments.
    *   *Standard Test*: ~16µs/op.
    *   *Aligned Test*: ~11µs/op.
*   **Deallocation**: Fast for individual blocks (~3-8µs), but can become expensive when a massive merge cascade occurs.
    *   *Aligned Test Final Dealloc*: ~86µs/op. This indicates that merging a highly fragmented grid of aligned blocks back into a single large block is computationally intensive, likely due to the recursive nature of checking and merging 4 siblings at multiple levels.
*   **Fragmentation**: The library handles fragmentation well, successfully refilling freed space in the "Refill" test phases.

## 4. Code Quality
*   **Modern C++**: Uses C++20 features appropriately.
*   **Readability**: The code is structured but the "Quad Split" logic with `split_point` and recursive merging is complex.
*   **Safety**: Includes a checked version (`allocator2d_checked`) for leak detection, which is very useful for debugging.
*   **Robustness**: Not thread-safe (as stated), which is standard for such allocators.

## 5. Comparison with Alternatives

| Feature | `mo_yanxi::allocator2d` | MaxRects (e.g., `stb_rect_pack`) | Guillotine | Buddy Allocator (Quadtree) |
| :--- | :--- | :--- | :--- | :--- |
| **Primary Goal** | Dynamic Alloc/Dealloc | Tightest Packing (Offline) | Simple/Fast Packing | Fast Alloc/Dealloc |
| **Deallocation** | **Yes** (Coalescing) | Difficult/Slow | Partial/Difficult | **Yes** (Fast Coalescing) |
| **Packing Efficiency** | Good (Variable size) | **Best** | Moderate | Poor (Powers of 2 usually) |
| **Algorithm** | Quad Split (Dynamic) | MaxRects Heuristic | Split to 2 | Fixed Grid / Quadtree |
| **Complexity** | High (Merge logic) | High (Fragment analysis) | Low | Low (Bitwise logic) |

### Detailed Comparison:

1.  **Vs. MaxRects**:
    *   **MaxRects** is the gold standard for generating static sprite sheets because it minimizes wasted space. However, implementing real-time `deallocate` for MaxRects is extremely hard because you have to recalculate the "maximal rectangles" formed by the new free space, which is computationally expensive.
    *   **Conclusion**: Use `mo_yanxi` for dynamic runtimes (e.g., UI rendering, font atlases) where icons/glyphs are loaded and unloaded. Use MaxRects for build-time asset generation.

2.  **Vs. Guillotine**:
    *   **Guillotine** is similar but splits free space into 2 rectangles instead of 4.
    *   **mo_yanxi** splits into 4. The author notes this might be suboptimal compared to splitting into 2. Splitting into 4 creates more nodes (fragmentation tracking overhead) but might offer more "shape" options for future fits.
    *   **Conclusion**: Guillotine might be slightly lighter weight, but `mo_yanxi` provides a complete implementation with deallocation support out-of-the-box.

3.  **Vs. Buddy Allocator**:
    *   A classic **Buddy Allocator** works on a fixed grid (usually powers of 2). Merging is $O(1)$ or $O(\log N)$ and very fast because siblings are implicit by address/index.
    *   **mo_yanxi** allows arbitrary sizes, so it doesn't waste space if you allocate a 33x33 box (a Buddy allocator might round up to 64x64). The trade-off is slower merge times and explicit tree traversal.
    *   **Conclusion**: `mo_yanxi` is more memory-efficient for irregular sizes than a Buddy allocator, but slower.

## 6. Conclusion
The `mo_yanxi::allocator2d` library is a solid choice for **dynamic 2D packing** requirements in C++ projects. It fills the niche between static packers (MaxRects) and rigid grid allocators (Buddy). It is well-suited for systems like:
*   Dynamic Font/Glyph Caches.
*   UI Texture Atlases (where widgets appear/disappear).
*   Streaming Tile Systems.

Its main trade-off is the overhead of the Quad Split structure, which can be slower during heavy deallocation/merging scenarios compared to simpler schemes, but it offers good packing density for arbitrary sizes.
