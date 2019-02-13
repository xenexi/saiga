//
// Created by Peter Eichinger on 2019-01-21.
//

#pragma once

#include "saiga/core/util/easylogging++.h"
#include "saiga/core/util/threadName.h"
#include "saiga/vulkan/Queue.h"

#include "BufferChunkAllocator.h"
#include "ChunkAllocation.h"
#include "FitStrategy.h"
#include "MemoryLocation.h"

#include <atomic>
#include <mutex>
#include <set>
#include <thread>
#include <vector>

#include <condition_variable>

namespace Saiga::Vulkan::Memory
{
struct OperationPenalties
{
    float target_small_hole     = 100.0f;
    float source_create_hole    = 200.0f;
    float source_not_last_alloc = 100.0f;
    float source_not_last_chunk = 400.0f;
    float same_chunk            = 500.0f;
};

template <typename T>
class Defragger
{
   private:
    struct DefragOperation
    {
        T* source;
        vk::DeviceMemory targetMemory;
        FreeListEntry target;
        float weight;


        bool operator<(const DefragOperation& second) const { return this->weight < second.weight; }
    };

    bool enabled;
    BufferChunkAllocator* allocator;
    std::multiset<DefragOperation> defrag_operations;


    std::atomic_bool running, quit;

    std::mutex start_mutex, running_mutex, invalidate_mutex;
    std::condition_variable start_condition;
    std::thread worker;

    void worker_func();

    std::set<vk::DeviceMemory> invalidate_set;

    // Defrag thread functions
    float get_operation_penalty(ConstChunkIterator<T> target_chunk, ConstFreeIterator<T> target_location,
                                ConstChunkIterator<T> source_chunk, ConstAllocationIterator<T> source_location) const;

    void apply_invalidations();

    void run();
    // end defrag thread functions
   public:
    OperationPenalties penalties;
    Defragger(BufferChunkAllocator* _allocator)
        : enabled(false),
          allocator(_allocator),
          defrag_operations(),
          running(false),
          quit(false),
          worker(&Defragger::worker_func, this)
    {
    }

    Defragger(const Defragger& other) = delete;
    Defragger& operator=(const Defragger& other) = delete;

    virtual ~Defragger() { exit(); }

    void exit()
    {
        running = false;
        std::unique_lock<std::mutex> lock(running_mutex);

        {
            std::lock_guard<std::mutex> lock(start_mutex);
            running = false;
            quit    = true;
        }
        start_condition.notify_one();
        worker.join();
    }

    void start();
    void stop();

    void setEnabled(bool enable) { enabled = enable; }

    void invalidate(vk::DeviceMemory memory);
    void invalidate(T* location);

    void find_defrag_ops();
    void perform_defrag();
};


template <typename T>
void Defragger<T>::start()
{
    if (!enabled || running)
    {
        return;
    }
    defrag_operations.clear();
    {
        std::lock_guard<std::mutex> lock(start_mutex);
        running = true;
    }
    start_condition.notify_one();
}

template <typename T>
void Defragger<T>::stop()
{
    if (!running)
    {
        return;
    }
    running = false;
    {
        std::unique_lock<std::mutex> lock(running_mutex);
    }
}


template <typename T>
void Defragger<T>::worker_func()
{
    Saiga::setThreadName("Defragger");
    while (true)
    {
        std::unique_lock<std::mutex> lock(start_mutex);
        start_condition.wait(lock, [this] { return running || quit; });
        if (quit)
        {
            return;
        }
        std::unique_lock<std::mutex> running_lock(running_mutex);
        if (allocator->chunks.empty())
        {
            continue;
        }

        apply_invalidations();

        run();

        running = false;
    }
}

template <typename T>
void Defragger<T>::run()
{
    find_defrag_ops();
    perform_defrag();
}

template <typename T>
void Defragger<T>::find_defrag_ops()
{
    auto& chunks = allocator->chunks;

    for (auto chunk_iter = chunks.rbegin(); running && chunk_iter != chunks.rend(); ++chunk_iter)
    {
        auto& allocs = chunk_iter->allocations;
        for (auto alloc_iter = allocs.rbegin(); running && alloc_iter != allocs.rend(); ++alloc_iter)
        {
            auto& source = **alloc_iter;

            if (source.is_static())
            {
                continue;
            }
            auto begin = chunks.begin();
            auto end   = (chunk_iter).base();  // Conversion from reverse to normal iterator moves one back
            //
            auto new_place = allocator->strategy->findRange(begin, end, source.size);

            if (new_place.first != end)
            {
                const auto target_iter = new_place.second;
                const auto& target     = *target_iter;

                auto current_chunk = (chunk_iter + 1).base();


                if (current_chunk != new_place.first || target.offset < (**alloc_iter).offset)
                {
                    auto weight =
                        get_operation_penalty(new_place.first, target_iter, current_chunk, (alloc_iter + 1).base());

                    defrag_operations.insert(DefragOperation{&source, new_place.first->chunk->memory, target, weight});
                }
            }
        }
    }
}

template <typename T>
void Defragger<T>::perform_defrag()
{
    {
        using namespace std::chrono_literals;
    }
    for (auto op = defrag_operations.begin(); running && op != defrag_operations.end(); ++op)
    {
        if (allocator->memory_is_free(op->targetMemory, op->target))
        {
            LOG(INFO) << "DEFRAG" << *(op->source) << "->" << op->targetMemory << "," << op->target.offset << " "
                      << op->target.size;

            T* reserve_space = allocator->reserve_space(op->targetMemory, op->target, op->source->size);
            auto defrag_cmd  = allocator->queue->commandPool.createAndBeginOneTimeBuffer();


            copy_buffer(defrag_cmd, reserve_space, op->source);
            // op->source->copy_to(defrag_cmd, reserve_space);

            defrag_cmd.end();

            allocator->queue->submitAndWait(defrag_cmd);

            allocator->queue->commandPool.freeCommandBuffer(defrag_cmd);

            allocator->move_allocation(reserve_space, op->source);
        }
    }
}


template <typename T>
float Defragger<T>::get_operation_penalty(ConstChunkIterator<T> target_chunk, ConstFreeIterator<T> target_location,
                                          ConstChunkIterator<T> source_chunk,
                                          ConstAllocationIterator<T> source_location) const
{
    auto& source_ptr = *source_location;
    float weight     = 0;


    if (target_chunk == source_chunk)
    {
        // move inside a chunk should be done after moving to others
        weight += penalties.same_chunk;
    }

    // if the move creates a hole that is smaller than the memory chunk itself -> add weight
    if (target_location->size != source_ptr->size && (target_location->size - source_ptr->size < source_ptr->size))
    {
        weight += penalties.target_small_hole * (1 - (static_cast<float>(source_ptr->size) / target_location->size));
    }

    // If move creates a hole at source -> add weight
    auto next = std::next(source_location);
    if (source_location != source_chunk->allocations.cbegin() && next != source_chunk->allocations.cend())
    {
        auto& next_ptr = *next;
        auto& prev     = *std::prev(source_location);

        if (source_ptr->offset == prev->offset + prev->size &&
            source_ptr->offset + source_ptr->size == next_ptr->offset)
        {
            weight += penalties.source_create_hole;
        }
    }

    // Penalty if allocation is not the last allocation in chunk
    if (next != source_chunk->allocations.cend())
    {
        weight += penalties.source_not_last_alloc;
    }

    if (std::next(source_chunk) != allocator->chunks.cend())
    {
        weight += penalties.source_not_last_chunk;
    }

    return weight;
}

template <typename T>
void Defragger<T>::apply_invalidations()
{
    std::unique_lock<std::mutex> invalidate_lock(invalidate_mutex);
    if (!defrag_operations.empty() && !invalidate_set.empty())
    {
        auto ops_iter = defrag_operations.begin();

        while (ops_iter != defrag_operations.end())
        {
            auto target_mem = ops_iter->targetMemory;
            if (invalidate_set.find(target_mem) != invalidate_set.end())
            {
                ops_iter = defrag_operations.erase(ops_iter);
            }
            else
            {
                ++ops_iter;
            }
        }
        invalidate_set.clear();
    }
}

template <typename T>
void Defragger<T>::invalidate(vk::DeviceMemory memory)
{
    std::unique_lock<std::mutex> invalidate_lock(invalidate_mutex);
    invalidate_set.insert(memory);
}

template <typename T>
void Defragger<T>::invalidate(T* location)
{
    std::unique_lock<std::mutex> invalidate_lock(invalidate_mutex);
    for (auto op_iter = defrag_operations.begin(); op_iter != defrag_operations.end();)
    {
        if (op_iter->source == location)
        {
            op_iter = defrag_operations.erase(op_iter);
        }
        else
        {
            ++op_iter;
        }
    }
}

}  // namespace Saiga::Vulkan::Memory