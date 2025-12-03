# Allocator 2D

* Provide simple **dynamic** rectangle area allocation/deallocation.
* Support Memory Allocator for internal containers.
* Not thread safe.
* Never rotate the area.
* No _strong exception guarantee_ .(regularly it should only throw `std::bad_alloc`) 
* The code is in di `./mo_yanxi`.

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

## Overview
1. Try to allocate many rects on a clean area.
2. Deallocate some of them
3. Allocate small fragments(color in white)
4. Deallocate some small fragments 
5. Deallocate all (not displayed, no leak anyway)
* You can play with the test with code in main.cpp

2048*2048 Test

| S1                                                                             | S2                                                                               | S3                                                                           | S4                                                                                     |
|-------------------------------------------------------------------------------|---------------------------------------------------------------------------------|-----------------------------------------------------------------------------|---------------------------------------------------------------------------------------|
| ![HighFragment_01_allocated.png](readme_assets/HighFragment_01_allocated.png) | ![HighFragment_02_fragmented.png](readme_assets/HighFragment_02_fragmented.png) | ![HighFragment_03_refilled.png](readme_assets/HighFragment_03_refilled.png) | ![HighFragment_04_partial_clear.png](readme_assets/HighFragment_04_partial_clear.png) |

---

1024*1024 Test

| S1                                                                     | S2                                                                       | S3                                                                   | S4                                                                             |
|-----------------------------------------------------------------------|-------------------------------------------------------------------------|---------------------------------------------------------------------|-------------------------------------------------------------------------------|
| ![Standard_01_allocated.png](readme_assets/Standard_01_allocated.png) | ![Standard_02_fragmented.png](readme_assets/Standard_02_fragmented.png) | ![Standard_03_refilled.png](readme_assets/Standard_03_refilled.png) | ![Standard_04_partial_clear.png](readme_assets/Standard_04_partial_clear.png) |








## Strategy (Quad Split)
* Try fit into small blocks. 
* Try to use area with thin siding, even if there exist area that has area smaller than it.
* Split the area to four part if there are remained area. ~~To be honest I think split to two parts may be better, but just remain it here...~~

#### If you want to save space, try allocating large components first.

## Interface

### Allocate
* Input the extent you need, return an point to the bottom left(x to right and y to above) of the allocated region. Or `nullopt` if there cannot find such space. 
* The allocated area is never moved.

### Deallocate
* Return false if the point is not a root of an allocated area in this allocator. Generally this is an error similar to _double free_ or `delete` a pointer that is not obtained from `operator new`, i.e. it should be treated as an assertion error.

## Leak Check
* `mo_yanxi::allocator2d_checked` Provides leak check. It checks that if the remain area is not equal to the total extent on destruct. If leaks, it calls `MO_YANXI_ALLOCATOR_2D_LEAK_BEHAVIOR(*this)` or print a simple error message and call `std::terminate` by default.

## Misc
* Marco `MO_YANXI_ALLOCATOR_2D_USE_STD_MODULE` specifies whether to use std module.
* Marco `MO_YANXI_ALLOCATOR_2D_HAS_MATH_VECTOR2` is used by the author with other libs, currently remained here.


