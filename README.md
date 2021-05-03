# Allocators
collection of useful header-only allocators

### FreeListAllocator
```cpp
class FreeListAllocator
{
    void Init(size_t initialSize, std::function<void(size_t)> onResize);
    void Resize(size_t newSize);
    size_t Allocate(size_t size);
    size_t Deallocate(size_t offset);
};
```
`FreeListAllocator` does not manage memory explicitly. Instead it operates on abstract continuous memory chunk using `size` and `offset`. This allows using allocator not only for heap allocations, but also for managing GPU memory (for example vertex buffer in popular graphic APIs)
