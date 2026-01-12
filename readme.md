# Allocator 2D
This library is designed for image atlas with **deallocation** demands (space reuse like temporary icons, glyphs...) and image fragments varying in **two** dimensions.

* Provide simple **dynamic** rectangle area allocation/deallocation, written in **C++20**.
* Single header/module interface.
* Support Memory Allocator for internal containers.
* Not thread safe.
* Never rotate the region.
* Never move the region after allocating.
* No _strong exception guarantee_ (regularly it should only throw `std::bad_alloc`).
* The code is in dir `./include/mo_yanxi`.

### Code Sample:
```c++
void foo(){
    mo_yanxi::allocator2d a{{256, 256}};
    const unsigned width = 32;
    const unsigned height = 64;
    if(const std::optional<mo_yanxi::math::usize2> where = a.allocate({width, height})){
        //Do something here...

        a.deallocate(where.value());
    }
}
```

## Strategy (Quad Split)

The allocator employs a quad-split strategy to manage 2D rectangular space effectively.

1.  **Search**: It looks for a free node (rectangle) that fits the requested dimensions. Prioritizes best-fit on one axis.
2.  **Allocation**: The requested area is placed at the **bottom-left** corner of the chosen node.
3.  **Split**: The remaining L-shaped area is split into up to three new free rectangles (siblings).

<table border="0" cellspacing="0" cellpadding="5" style="border-collapse:collapse; text-align:center;">
  <tr>
    <td style="border: 2px dashed #888; width:150px; height:100px;">
      <strong>Top-Left (Free)</strong><br>
      <span style="font-size:0.8em; color:#555;">Sibling 3</span>
    </td>
    <td style="border: 2px dashed #888; width:250px; height:100px;">
      <strong>Top-Right (Free)</strong><br>
      <span style="font-size:0.8em; color:#555;">Sibling 2 (Corner)</span>
    </td>
  </tr>
  <tr>
    <td style="border: 2px solid #333; width:150px; height:150px;">
      <strong>Allocated</strong><br>
      <span style="font-size:0.8em;">(User Request)</span>
    </td>
    <td style="border: 2px dashed #888; width:250px; height:150px;">
      <strong>Bottom-Right (Free)</strong><br>
      <span style="font-size:0.8em; color:#555;">Sibling 1</span>
    </td>
  </tr>
</table>

### Merging & Deallocation
*   **Strict Merge Rule**: A node can only be merged back into its parent if **all** of its generated siblings are also free (idle).
*   **Fragmentation Risk**: If a small fragment (e.g., "Sibling 2") is still in use, the other siblings cannot be merged back to the parent. This can prevent large regions from reforming, as seen in the "Standard Test S3" benchmark.

> **Tip:** To save space and minimize fragmentation, try allocating large components first.

## Technical Details

### Coordinate System
The allocator uses a standard Cartesian coordinate system:
*   **Origin (0, 0)**: Bottom-Left corner.
*   **+X**: Right.
*   **+Y**: Up.

### Fragment Threshold
To improve performance, the allocator distinguishes between "Large Nodes" and "Fragments".
*   Nodes smaller than the `fragment_threshold` (default: ~1.5% of total area) are stored in separate search trees.
*   This prevents small gaps from polluting the main search space, ensuring faster lookup times for significant allocations.

## Benchmark 
1. Try to allocate many rects on a clean area.
2. Deallocate some of them
3. Allocate small fragments (color in white)
4. Deallocate all (not displayed, no leak anyway)
* You can play with the test with code in main.cpp

<table>
    <thead>
        <tr>
            <th align="center">Test Case</th>
            <th align="center">Initial Allocate</th>
            <th align="center">Partial Deallocate</th>
            <th align="center">Allocate Fragments (White)</th>
        </tr>
    </thead>
    <tbody>
<tr>
            <td align="center"><strong>Standard</strong></td>
            <td align="center">
                <img src="readme_assets/Standard_01_allocated.png" width="100%" alt="Standard Initial"/>
            </td>
            <td align="center">
                <img src="readme_assets/Standard_02_fragmented.png" width="100%" alt="Standard Partial"/>
            </td>
            <td align="center">
                <img src="readme_assets/Standard_03_refilled.png" width="100%" alt="Standard Refilled"/>
            </td>
        </tr>
        <tr>
            <td align="center"><strong>HighFragment</strong></td>
            <td align="center">
                <img src="readme_assets/HighFragment_01_allocated.png" width="100%" alt="HighFragment Initial"/>
            </td>
            <td align="center">
                <img src="readme_assets/HighFragment_02_fragmented.png" width="100%" alt="HighFragment Partial"/>
            </td>
            <td align="center">
                <img src="readme_assets/HighFragment_03_refilled.png" width="100%" alt="HighFragment Refilled"/>
            </td>
        </tr>
        <tr>
            <td align="center"><strong>Aligned</strong></td>
            <td align="center">
                <img src="readme_assets/Aligned_01_allocated.png" width="100%" alt="Aligned Initial"/>
            </td>
            <td align="center">
                <img src="readme_assets/Aligned_02_fragmented.png" width="100%" alt="Aligned Partial"/>
            </td>
            <td align="center">
                <img src="readme_assets/Aligned_03_refilled.png" width="100%" alt="Aligned Refilled"/>
            </td>
        </tr>
    </tbody>
</table>

---

* Using Clang 20.1, -O2, `std::allocator`

| Test Suite       | Phase            | Total (ns/op) | Success (ns/op) | Fail (ns/op) |
|:-----------------|:-----------------|:--------------|:----------------|:-------------|
| **Standard**     | Alloc Fill       | 1706.15       | 2502.52         | 1683.38      |
|                  | Dealloc Fragment | 1452.52       | 1452.52         | -            |
|                  | Alloc Refill     | 2939.96       | 1697.63         | 5073.53      |
|                  | Dealloc Partial  | 757.63        | 757.63          | -            |
|                  | Dealloc Final    | 1801.39       | 1801.39         | -            |
| **HighFragment** | Alloc Fill       | 2682.84       | 2152.11         | 4646.15      |
|                  | Dealloc Fragment | 683.56        | 683.56          | -            |
|                  | Alloc Refill     | 2363.76       | 2204.31         | 2707.00      |
|                  | Dealloc Partial  | 647.03        | 647.03          | -            |
|                  | Dealloc Final    | 2187.15       | 2187.15         | -            |
| **Aligned**      | Alloc Fill       | 466.72        | 1105.13         | 23.81        |
|                  | Dealloc Fragment | 1863.92       | 1863.92         | -            |
|                  | Alloc Refill     | 932.92        | 1656.76         | 379.24       |
|                  | Dealloc Partial  | 821.28        | 821.28          | -            |
|                  | Dealloc Final    | 20674.69      | 20674.69        | -            |

## Interface

### Allocate
* Input the extent you need, return an point to the bottom left (x to right and y to above) of the allocated region. Or `nullopt` if there cannot find such space.
* The allocated area is never moved.

### Deallocate
* Input the position obtained from `allocate`. 
* Return `false` if the point is not a root of an allocated area in this allocator. Generally this is an error similar to _double free_ or `delete` a pointer that is not obtained from `operator new`, i.e. it should be treated as an assertion error.

### Copy Constructor/Assign Operator
* The copy constructor is defined as protected, you can derive the allocator and expose it to using this functionality.

### Move Constructor/Assign Operator
* Allocator after being moved is empty, you should reassign an allocator to it if you want use it after move.

## Leak Check
* `mo_yanxi::allocator2d_checked` Provides leak check. It checks that if the remain area is not equal to the total extent on destruct. If leaks, it calls `MO_YANXI_ALLOCATOR_2D_LEAK_BEHAVIOR(*this)` or print a simple error message and call `std::terminate` by default.

## Misc
* Macro `MO_YANXI_ALLOCATOR_2D_USE_STD_MODULE` specifies whether to use std module.
* Macro `MO_YANXI_ALLOCATOR_2D_HAS_MATH_VECTOR2` is used by the author with other libs, currently kept here.
