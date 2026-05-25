#pragma once

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <new>
#include <type_traits>
#include <utility>
#include <vector>

class CommandQueue
{
public:
    CommandQueue() = default;
    ~CommandQueue()
    {
        Clear();
    }

    CommandQueue(const CommandQueue&) = delete;
    CommandQueue& operator=(const CommandQueue&) = delete;

    CommandQueue(CommandQueue&&) noexcept = default;
    CommandQueue& operator=(CommandQueue&& other) noexcept
    {
        if (this != &other)
        {
            Clear();
            m_Blocks = std::move(other.m_Blocks);
        }
        return *this;
    }

    template <typename F>
    void Submit(F&& f)
    {
        using FuncType = std::decay_t<F>;
        static_assert(alignof(FuncType) <= kBlockAlignment,
                      "Command captures require an alignment larger than the command queue block alignment.");

        Block& block = GetWritableBlock(sizeof(CommandHeader) + alignof(FuncType) + sizeof(FuncType));
        size_t headerOffset = AlignUp(block.used, alignof(CommandHeader));
        size_t dataOffset = AlignUp(headerOffset + sizeof(CommandHeader), alignof(FuncType));
        size_t endOffset = dataOffset + sizeof(FuncType);

        if (endOffset > block.capacity)
        {
            Block& freshBlock = AddBlock(sizeof(CommandHeader) + alignof(FuncType) + sizeof(FuncType));
            headerOffset = AlignUp(freshBlock.used, alignof(CommandHeader));
            dataOffset = AlignUp(headerOffset + sizeof(CommandHeader), alignof(FuncType));
            endOffset = dataOffset + sizeof(FuncType);
            return SubmitIntoBlock<FuncType>(freshBlock, headerOffset, dataOffset, endOffset, std::forward<F>(f));
        }

        SubmitIntoBlock<FuncType>(block, headerOffset, dataOffset, endOffset, std::forward<F>(f));
    }

    void Execute()
    {
        ForEachCommand([](CommandHeader& header, uint8_t* data) { header.execute(data); });
    }

    void Clear()
    {
        ForEachCommand([](CommandHeader& header, uint8_t* data) { header.destroy(data); });
        for (auto& block : m_Blocks)
        {
            block->used = 0;
        }
    }

    bool IsEmpty() const
    {
        return std::all_of(m_Blocks.begin(), m_Blocks.end(), [](const auto& block) { return block->used == 0; });
    }

    void Merge(CommandQueue& other)
    {
        for (auto& block : other.m_Blocks)
        {
            if (block && block->used > 0)
            {
                m_Blocks.push_back(std::move(block));
            }
        }
        other.m_Blocks.clear();
    }

private:
    static constexpr size_t kDefaultBlockSize = 64 * 1024;
    static constexpr size_t kBlockAlignment = 64;

    static size_t AlignUp(size_t value, size_t alignment)
    {
        return (value + alignment - 1) & ~(alignment - 1);
    }

    struct CommandHeader
    {
        void (*execute)(uint8_t*);
        void (*destroy)(uint8_t*);
        size_t commandSize;
        size_t dataOffset;
    };

    struct Block
    {
        explicit Block(size_t requestedCapacity)
            : capacity(AlignUp((std::max)(requestedCapacity, kDefaultBlockSize), kBlockAlignment)),
              data(static_cast<uint8_t*>(::operator new(capacity, std::align_val_t(kBlockAlignment))))
        {
        }

        ~Block()
        {
            ::operator delete(data, std::align_val_t(kBlockAlignment));
        }

        Block(const Block&) = delete;
        Block& operator=(const Block&) = delete;

        size_t capacity = 0;
        size_t used = 0;
        uint8_t* data = nullptr;
    };

    Block& AddBlock(size_t minimumCapacity)
    {
        m_Blocks.push_back(std::make_unique<Block>(minimumCapacity));
        return *m_Blocks.back();
    }

    Block& GetWritableBlock(size_t requiredBytes)
    {
        if (m_Blocks.empty())
        {
            return AddBlock(requiredBytes);
        }

        Block& block = *m_Blocks.back();
        if (block.capacity - block.used < requiredBytes)
        {
            return AddBlock(requiredBytes);
        }
        return block;
    }

    template <typename FuncType, typename F>
    void SubmitIntoBlock(Block& block, size_t headerOffset, size_t dataOffset, size_t endOffset, F&& f)
    {
        new (block.data + dataOffset) FuncType(std::forward<F>(f));

        auto* header = reinterpret_cast<CommandHeader*>(block.data + headerOffset);
        header->execute = [](uint8_t* data) {
            auto* func = reinterpret_cast<FuncType*>(data);
            (*func)();
        };
        header->destroy = [](uint8_t* data) {
            auto* func = reinterpret_cast<FuncType*>(data);
            func->~FuncType();
        };
        header->dataOffset = dataOffset - headerOffset;
        header->commandSize = endOffset - headerOffset;
        block.used = endOffset;
    }

    template <typename Fn>
    void ForEachCommand(Fn&& fn)
    {
        for (auto& block : m_Blocks)
        {
            if (!block)
                continue;

            size_t offset = 0;
            while (offset < block->used)
            {
                offset = AlignUp(offset, alignof(CommandHeader));
                if (offset >= block->used)
                    break;

                auto* header = reinterpret_cast<CommandHeader*>(block->data + offset);
                uint8_t* data = block->data + offset + header->dataOffset;
                fn(*header, data);
                offset += header->commandSize;
            }
        }
    }

    std::vector<std::unique_ptr<Block>> m_Blocks;
};
