#pragma once
#include <uescript.h>

/**
 * @brief Kind of a hack. Some engine functions like to free parameters themselves, so this is a lazy solution to avoid
		  double-free.
*/
class AllocationTracker final
{
    enum Status : bool
    {
        ACTIVE,
        FREED,
    };

public:
    void AddAllocation(void* ptr)
    {
        std::scoped_lock write_lock(m_Mutex);
        m_Allocations.emplace(ptr, ACTIVE);
    }

    void RemoveAllocation(void* ptr)
    {
        std::scoped_lock write_lock(m_Mutex);
        m_Allocations.erase(ptr);
    }

    void SetFreed(void* ptr)
    {
        std::scoped_lock write_lock(m_Mutex);
        const auto& it = m_Allocations.find(ptr);
        if (it == m_Allocations.end())
            return;
        it->second = FREED;
    }

    bool HasBeenFreed(void* ptr) const
    {
        std::shared_lock read_lock(m_Mutex);
        const auto& it = m_Allocations.find(ptr);
        if (it == m_Allocations.end())
            return false;
        return it->second == FREED;
    }

private:
    struct Hasher
    {
        size_t operator()(void* key) const
        {
            return reinterpret_cast<size_t>(key);
        }
    };

    mutable std::shared_mutex m_Mutex{};
    std::unordered_map<void*, Status, Hasher> m_Allocations{};
};

inline std::unique_ptr<AllocationTracker> g_AllocationTracker{nullptr};
