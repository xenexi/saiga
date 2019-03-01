#include <utility>

//
// Created by Peter Eichinger on 08.10.18.
//

#pragma once
#include "saiga/core/util/easylogging++.h"
#include "saiga/export.h"
#include "saiga/vulkan/memory/Chunk.h"

#include <mutex>
#include <optional>
#include <ostream>
#include <saiga/core/util/assert.h>
#include <vulkan/vulkan.hpp>
namespace Saiga::Vulkan::Memory
{
template <typename T>
struct SafeAccessor final
{
   private:
    std::unique_lock<std::mutex> lock1, lock2;

   public:
    explicit SafeAccessor(T& _location)
        : lock1(_location.mutex, std::defer_lock), lock2(_location.mutex, std::defer_lock)
    {
        if (_location.is_dynamic())
        {
            lock1.lock();
        }
    }

    SafeAccessor(T& _location, T& _location2)
        : lock1(_location.mutex, std::defer_lock), lock2(_location2.mutex, std::defer_lock)
    {
        if (_location.is_dynamic() && _location2.is_dynamic())
        {
            std::lock(lock1, lock2);
            return;
        }

        if (_location.is_dynamic())
        {
            lock1.lock();
        }

        if (_location2.is_dynamic())
        {
            lock2.lock();
        }
    }

    ~SafeAccessor()
    {
        if (lock1.owns_lock())
        {
            lock1.unlock();
        }
        if (lock2.owns_lock())
        {
            lock2.unlock();
        }
    }

    SafeAccessor(const SafeAccessor&) = delete;
    SafeAccessor(SafeAccessor&&)      = delete;
};

template <typename Data>
struct SAIGA_VULKAN_API BaseMemoryLocation
{
   public:
    Data data;
    vk::DeviceMemory memory;
    vk::DeviceSize offset;
    vk::DeviceSize size;
    void* mappedPointer;
    std::mutex mutex;

   private:
    bool static_mem;

   public:
    explicit BaseMemoryLocation(vk::DeviceSize _size)
        : data(nullptr), memory(nullptr), offset(0), size(_size), mappedPointer(nullptr), mutex(), static_mem(true)
    {
    }

    explicit BaseMemoryLocation(Data _data = nullptr, vk::DeviceMemory _memory = nullptr, vk::DeviceSize _offset = 0,
                                vk::DeviceSize _size = 0, void* _basePointer = nullptr)
        : data(_data), memory(_memory), offset(_offset), size(_size), mappedPointer(nullptr), mutex(), static_mem(true)
    {
        if (_basePointer)
        {
            mappedPointer = static_cast<char*>(_basePointer) + offset;
        }
    }

    BaseMemoryLocation(const BaseMemoryLocation& other) = default;

    BaseMemoryLocation& operator=(const BaseMemoryLocation& other) = default;

    BaseMemoryLocation(BaseMemoryLocation&& other) noexcept
        : data(other.data),
          memory(other.memory),
          offset(other.offset),
          size(other.size),
          mappedPointer(other.mappedPointer),
          mutex(),
          static_mem(other.static_mem)
    {
        other.make_invalid();
    }

    BaseMemoryLocation& operator=(BaseMemoryLocation&& other) noexcept
    {
        if (this != &other)
        {
            data          = other.data;
            memory        = other.memory;
            offset        = other.offset;
            size          = other.size;
            mappedPointer = other.mappedPointer;
            mutex         = std::mutex();
            static_mem    = other.static_mem;

            other.make_invalid();
        }
        return *this;
    }

    explicit operator bool() { return memory; }

   private:
    inline void make_invalid()
    {
        this->data          = nullptr;
        this->memory        = nullptr;
        this->offset        = VK_WHOLE_SIZE;
        this->size          = VK_WHOLE_SIZE;
        this->mappedPointer = nullptr;
    }
    void mappedUpload(vk::Device device, const void* data)
    {
        SAIGA_ASSERT(!mappedPointer, "Memory already mapped");
        void* target;
        vk::Result result;
        vk::DeviceMemory mem;
        {
            SafeAccessor safe(*this);
            result = device.mapMemory(memory, offset, size, vk::MemoryMapFlags(), &target);
            mem    = memory;
        }
        if (result != vk::Result::eSuccess)
        {
            LOG(FATAL) << "Could not map " << mem << vk::to_string(result);
        }
        std::memcpy(target, data, size);
        device.unmapMemory(mem);
    }


    void mappedDownload(vk::Device device, void* data)
    {
        SAIGA_ASSERT(!mappedPointer, "Memory already mapped");


        void* target;
        {
            SafeAccessor safe(*this);
            target = device.mapMemory(memory, offset, size);
        }
        std::memcpy(data, target, size);
        device.unmapMemory(memory);
    }

   public:
    inline bool is_dynamic() const { return !is_static(); }
    inline bool is_static() const { return static_mem; }

    inline void mark_dynamic()
    {
        SafeAccessor acc(*this);
        static_mem = false;
    }

    void upload(vk::Device device, const void* data)
    {
        void* pointer;

        {
            SafeAccessor safe(*this);
            pointer = mappedPointer;
        }
        if (pointer)
        {
            std::memcpy(pointer, data, size);
        }
        else
        {
            mappedUpload(device, data);
        }
    }

    void download(vk::Device device, void* data)
    {
        void* pointer;

        {
            SafeAccessor safe(*this);
            pointer = mappedPointer;
        }
        if (pointer)
        {
            std::memcpy(data, pointer, size);
        }
        else
        {
            mappedDownload(device, data);
        }
    }

    /** destroys the owned data of the data member.
     * @param device device of the data.
     */
    inline void destroy_owned_data(const vk::Device& device)
    {
        if (data)
        {
            data.destroy_owned_data(device);
            data = nullptr;
        }
    }

    bool operator==(const BaseMemoryLocation& rhs)
    {
        SafeAccessor accessor(*this, rhs);

        return std::tie(data, memory, offset, size, mappedPointer) ==
               std::tie(rhs.data, rhs.memory, rhs.offset, rhs.size, rhs.mappedPointer);
    }

    bool operator!=(const BaseMemoryLocation& rhs) { return !(rhs == *this); }


    friend std::ostream& operator<<(std::ostream& os, const BaseMemoryLocation& location)
    {
        os << "{" << location.memory << ", " << location.data << ", " << location.offset << " " << location.size;

        if (location.mappedPointer)
        {
            os << ", " << location.mappedPointer;
        }

        os << "}";
        return os;
    }

    void copy_to(BaseMemoryLocation<Data>& other)
    {
        SafeAccessor accessor(*this, other);
        other.memory = memory;
        other.offset = offset;
        other.size   = size;
        other.data   = data;
    }
};

}  // namespace Saiga::Vulkan::Memory