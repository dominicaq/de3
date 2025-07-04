#include "LinearAllocator.h"
#include <algorithm>

LinearAllocator::LinearAllocator(size_t size)
    : m_size(size)
    , m_offset(0)
{
}

uint32_t LinearAllocator::Allocate(size_t size) {
    size_t alignedSize = AlignUp(size, DEFAULT_ALIGNMENT);

    if (m_offset + alignedSize > m_size) {
        return UINT32_MAX; // Out of space
    }

    uint32_t result = static_cast<uint32_t>(m_offset);
    m_offset += alignedSize;
    return result;
}

void LinearAllocator::Reset() {
    m_offset = 0;
}

bool LinearAllocator::CanAllocate(size_t size) const {
    size_t alignedSize = AlignUp(size, DEFAULT_ALIGNMENT);
    return (m_offset + alignedSize) <= m_size;
}

size_t LinearAllocator::AlignUp(size_t size, size_t alignment) {
    return (size + alignment - 1) & ~(alignment - 1);
}
