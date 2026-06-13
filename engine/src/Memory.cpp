#include "Arc/Memory.hpp"

#include <cassert>
#include <cstdlib>
#include <mutex>
#include <new>

namespace Arc
{
    // LinearAllocator implementation
    LinearAllocator::LinearAllocator(std::size_t size, void* start)
        : m_start(start), m_size(size), m_offset(0)
    {
        assert(start != nullptr && "Start address cannot be null");
        assert(size > 0 && "Size must be greater than zero");
    }

    LinearAllocator::~LinearAllocator()
    {
        m_start = nullptr;
        m_size = 0;
        m_offset = 0;
    }

    void* LinearAllocator::allocate(std::size_t size, std::size_t alignment)
    {
        std::uintptr_t currentAddress = reinterpret_cast<std::uintptr_t>(m_start) + m_offset;
        std::uintptr_t alignedAddress = alignForward(currentAddress, alignment);
        std::size_t newOffset = (alignedAddress - reinterpret_cast<std::uintptr_t>(m_start)) + size;

        if (newOffset > m_size)
        {
            return nullptr; // Out of memory
        }

        m_offset = newOffset;
        return reinterpret_cast<void*>(alignedAddress);
    }

    void LinearAllocator::deallocate(void* ptr)
    {
        // Linear allocators do not support individual deallocations
        (void)ptr;
    }

    void LinearAllocator::reset()
    {
        m_offset = 0;
    }

    // StackAllocator implementation
    StackAllocator::StackAllocator(std::size_t size, void* start)
        : m_start(start), m_size(size), m_offset(0)
    {
        assert(start != nullptr && "Start address cannot be null");
        assert(size > 0 && "Size must be greater than zero");
    }

    StackAllocator::~StackAllocator()
    {
        m_start = nullptr;
        m_size = 0;
        m_offset = 0;
    }

    void* StackAllocator::allocate(std::size_t size, std::size_t alignment)
    {
        std::uintptr_t currentAddress = reinterpret_cast<std::uintptr_t>(m_start) + m_offset;
        std::uintptr_t alignedAddress = alignForward(currentAddress, alignment);
        std::size_t newOffset = (alignedAddress - reinterpret_cast<std::uintptr_t>(m_start)) + size;

        if (newOffset > m_size)
        {
            return nullptr; // Out of memory
        }

        m_offset = newOffset;
        return reinterpret_cast<void*>(alignedAddress);
    }

    void StackAllocator::deallocate(void* ptr)
    {
        // Deallocate does nothing individually, but can roll back to a marker
        (void)ptr;
    }

    void StackAllocator::reset()
    {
        m_offset = 0;
    }

    StackAllocator::Marker StackAllocator::getMarker() const
    {
        return m_offset;
    }

    void StackAllocator::freeToMarker(Marker marker)
    {
        assert(marker <= m_offset && "Cannot free to a future marker");
        m_offset = marker;
    }

    // PoolAllocator implementation
    PoolAllocator::PoolAllocator(std::size_t objectSize, std::size_t alignment, std::size_t size, void* start)
        : m_start(start), m_size(size), m_objectSize(objectSize), m_alignment(alignment)
    {
        assert(start != nullptr && "Start address cannot be null");
        assert(size >= objectSize && "Total size must be at least as big as one object");
        assert(objectSize >= sizeof(void*) && "Object size must be at least sizeof(void*) to store the free list pointer");

        reset();
    }

    PoolAllocator::~PoolAllocator()
    {
        m_start = nullptr;
        m_size = 0;
        m_freeList = nullptr;
    }

    void* PoolAllocator::allocate(std::size_t size, std::size_t alignment)
    {
        (void)size;
        (void)alignment;
        assert(size <= m_objectSize && "Allocation size exceeds pool object size");
        assert(alignment <= m_alignment && "Allocation alignment exceeds pool alignment");

        if (m_freeList == nullptr)
        {
            return nullptr; // Pool is full
        }

        void* ptr = m_freeList;
        m_freeList = reinterpret_cast<void**>(*m_freeList);
        m_usedMemory += m_objectSize;
        return ptr;
    }

    void PoolAllocator::deallocate(void* ptr)
    {
        if (ptr == nullptr)
        {
            return;
        }

        // Return block to the free list
        void** next = reinterpret_cast<void**>(ptr);
        *next = m_freeList;
        m_freeList = next;
        m_usedMemory -= m_objectSize;
    }

    void PoolAllocator::reset()
    {
        m_usedMemory = 0;

        std::uintptr_t startAddress = reinterpret_cast<std::uintptr_t>(m_start);
        std::uintptr_t alignedStart = alignForward(startAddress, m_alignment);
        std::size_t adjustment = alignedStart - startAddress;

        if (adjustment > m_size)
        {
            m_freeList = nullptr;
            return;
        }

        std::size_t remainingSize = m_size - adjustment;
        std::size_t numObjects = remainingSize / m_objectSize;

        if (numObjects == 0)
        {
            m_freeList = nullptr;
            return;
        }

        // Initialize free list
        m_freeList = reinterpret_cast<void**>(alignedStart);
        void** current = m_freeList;
        for (std::size_t i = 0; i < numObjects - 1; ++i)
        {
            std::uintptr_t nextAddress = reinterpret_cast<std::uintptr_t>(current) + m_objectSize;
            *current = reinterpret_cast<void*>(nextAddress);
            current = reinterpret_cast<void**>(nextAddress);
        }
        *current = nullptr; // Last element points to nullptr
    }

    // FrameAllocator global state
    namespace
    {
        void* s_frameBuffer = nullptr;
        LinearAllocator* s_frameAllocator = nullptr;
        std::mutex s_frameMutex;
    }

    void FrameAllocator::initialize(std::size_t size)
    {
        std::lock_guard<std::mutex> lock(s_frameMutex);
        if (s_frameBuffer == nullptr)
        {
            s_frameBuffer = std::malloc(size);
            s_frameAllocator = new LinearAllocator(size, s_frameBuffer);
        }
    }

    void FrameAllocator::shutdown()
    {
        std::lock_guard<std::mutex> lock(s_frameMutex);
        if (s_frameAllocator != nullptr)
        {
            delete s_frameAllocator;
            s_frameAllocator = nullptr;
        }
        if (s_frameBuffer != nullptr)
        {
            std::free(s_frameBuffer);
            s_frameBuffer = nullptr;
        }
    }

    void* FrameAllocator::allocate(std::size_t size, std::size_t alignment)
    {
        std::lock_guard<std::mutex> lock(s_frameMutex);
        assert(s_frameAllocator != nullptr && "FrameAllocator is not initialized");
        return s_frameAllocator->allocate(size, alignment);
    }

    void FrameAllocator::reset()
    {
        std::lock_guard<std::mutex> lock(s_frameMutex);
        if (s_frameAllocator != nullptr)
        {
            s_frameAllocator->reset();
        }
    }
}
