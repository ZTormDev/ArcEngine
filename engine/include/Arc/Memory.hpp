#pragma once

#include <cstddef>
#include <cstdint>
#include <new>
#include <utility>

namespace Arc
{
    // Utility to align a pointer forward
    inline std::uintptr_t alignForward(std::uintptr_t address, std::size_t alignment)
    {
        return (address + (alignment - 1)) & ~(alignment - 1);
    }

    inline void* alignForward(void* address, std::size_t alignment)
    {
        return reinterpret_cast<void*>(alignForward(reinterpret_cast<std::uintptr_t>(address), alignment));
    }

    // Base Allocator Interface
    class Allocator
    {
    public:
        virtual ~Allocator() = default;
        virtual void* allocate(std::size_t size, std::size_t alignment = alignof(std::max_align_t)) = 0;
        virtual void deallocate(void* ptr) = 0;
        virtual void reset() = 0;
    };

    // Linear / Arena Allocator
    class LinearAllocator final : public Allocator
    {
    public:
        LinearAllocator(std::size_t size, void* start);
        ~LinearAllocator() override;

        LinearAllocator(const LinearAllocator&) = delete;
        LinearAllocator& operator=(const LinearAllocator&) = delete;

        void* allocate(std::size_t size, std::size_t alignment = alignof(std::max_align_t)) override;
        void deallocate(void* ptr) override;
        void reset() override;

        [[nodiscard]] std::size_t getUsedMemory() const { return m_offset; }
        [[nodiscard]] std::size_t getSize() const { return m_size; }

    private:
        void* m_start = nullptr;
        std::size_t m_size = 0;
        std::size_t m_offset = 0;
    };

    // Stack Allocator
    class StackAllocator final : public Allocator
    {
    public:
        using Marker = std::size_t;

        StackAllocator(std::size_t size, void* start);
        ~StackAllocator() override;

        StackAllocator(const StackAllocator&) = delete;
        StackAllocator& operator=(const StackAllocator&) = delete;

        void* allocate(std::size_t size, std::size_t alignment = alignof(std::max_align_t)) override;
        void deallocate(void* ptr) override;
        void reset() override;

        [[nodiscard]] Marker getMarker() const;
        void freeToMarker(Marker marker);

        [[nodiscard]] std::size_t getUsedMemory() const { return m_offset; }
        [[nodiscard]] std::size_t getSize() const { return m_size; }

    private:
        void* m_start = nullptr;
        std::size_t m_size = 0;
        std::size_t m_offset = 0;
    };

    // Pool Allocator
    class PoolAllocator final : public Allocator
    {
    public:
        PoolAllocator(std::size_t objectSize, std::size_t alignment, std::size_t size, void* start);
        ~PoolAllocator() override;

        PoolAllocator(const PoolAllocator&) = delete;
        PoolAllocator& operator=(const PoolAllocator&) = delete;

        void* allocate(std::size_t size, std::size_t alignment = alignof(std::max_align_t)) override;
        void deallocate(void* ptr) override;
        void reset() override;

        [[nodiscard]] std::size_t getUsedMemory() const { return m_usedMemory; }
        [[nodiscard]] std::size_t getSize() const { return m_size; }

    private:
        void* m_start = nullptr;
        std::size_t m_size = 0;
        std::size_t m_objectSize = 0;
        std::size_t m_alignment = 0;
        void** m_freeList = nullptr;
        std::size_t m_usedMemory = 0;
    };

    // Frame Allocator (Global/Thread-local reset-per-frame allocator)
    class FrameAllocator
    {
    public:
        static void initialize(std::size_t size);
        static void shutdown();
        static void* allocate(std::size_t size, std::size_t alignment = alignof(std::max_align_t));
        static void reset();

        template<typename T, typename... Args>
        static T* make(Args&&... args)
        {
            void* mem = allocate(sizeof(T), alignof(T));
            if (mem == nullptr)
            {
                return nullptr;
            }
            return new (mem) T(std::forward<Args>(args)...);
        }
    };
}
