#pragma once
#include <cstdint>

// =============================================================================
// Linear Allocator for Buffer Management
// =============================================================================

class LinearAllocator {
public:
    explicit LinearAllocator(size_t size);
    ~LinearAllocator() = default;

    // Prevent copying
    LinearAllocator(const LinearAllocator&) = delete;
    LinearAllocator& operator=(const LinearAllocator&) = delete;

    // Allocation methods
    uint32_t Allocate(size_t size);
    void Reset();

    // Query methods
    size_t GetUsedSpace() const { return m_offset; }
    size_t GetAvailableSpace() const { return m_size - m_offset; }
    size_t GetTotalSize() const { return m_size; }

    // Utility
    bool CanAllocate(size_t size) const;

private:
    static size_t AlignUp(size_t size, size_t alignment);

    size_t m_size;
    size_t m_offset;
    static constexpr size_t DEFAULT_ALIGNMENT = 256;
};
