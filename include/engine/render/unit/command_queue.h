#pragma once

#include <vector>
#include <cstdint>
#include <utility>
#include <new>

// Flat, zero-allocation-per-command bump allocator for rendering commands
class CommandQueue {
public:
    CommandQueue() {
        m_Buffer.reserve(1024 * 64); // Pre-allocate 64KB for commands
    }

    template <typename F>
    void Submit(F&& f) {
        using FuncType = std::decay_t<F>;
        constexpr size_t funcSize = sizeof(FuncType);
        constexpr size_t align = alignof(FuncType);
        
        // Ensure alignment for the header
        size_t offset = m_Buffer.size();
        size_t headerPad = (alignof(CommandHeader) - (offset % alignof(CommandHeader))) % alignof(CommandHeader);
        offset += headerPad;

        // Ensure alignment for the function object
        size_t dataOffset = offset + sizeof(CommandHeader);
        size_t funcPad = (align - (dataOffset % align)) % align;
        dataOffset += funcPad;

        size_t totalSize = (dataOffset + funcSize) - m_Buffer.size();

        m_Buffer.resize(m_Buffer.size() + totalSize);

        // Placement new the lambda into the buffer
        new (m_Buffer.data() + dataOffset) FuncType(std::forward<F>(f));

        // Write header
        CommandHeader* header = reinterpret_cast<CommandHeader*>(m_Buffer.data() + offset);
        header->execute = [](const uint8_t* data) {
            auto* func = reinterpret_cast<const FuncType*>(data);
            (*func)();
        };
        header->destroy = [](uint8_t* data) {
            auto* func = reinterpret_cast<FuncType*>(data);
            func->~FuncType();
        };
        header->dataOffset = static_cast<uint16_t>(dataOffset - offset);
        header->commandSize = static_cast<uint32_t>(totalSize - headerPad);
    }

    void Execute() {
        size_t offset = 0;
        while (offset < m_Buffer.size()) {
            size_t pad = (alignof(CommandHeader) - (offset % alignof(CommandHeader))) % alignof(CommandHeader);
            offset += pad;

            if (offset >= m_Buffer.size()) break;

            CommandHeader* header = reinterpret_cast<CommandHeader*>(m_Buffer.data() + offset);
            const uint8_t* data = m_Buffer.data() + offset + header->dataOffset;
            
            header->execute(data);
            
            offset += header->commandSize;
        }
    }

    void Clear() {
        size_t offset = 0;
        while (offset < m_Buffer.size()) {
            size_t pad = (alignof(CommandHeader) - (offset % alignof(CommandHeader))) % alignof(CommandHeader);
            offset += pad;

            if (offset >= m_Buffer.size()) break;

            CommandHeader* header = reinterpret_cast<CommandHeader*>(m_Buffer.data() + offset);
            uint8_t* data = m_Buffer.data() + offset + header->dataOffset;
            
            header->destroy(data);
            
            offset += header->commandSize;
        }
        m_Buffer.clear();
    }

    bool IsEmpty() const { return m_Buffer.empty(); }

    void Merge(CommandQueue& other) {
        if (other.m_Buffer.empty()) return;
        
        // Memory copy because lambdas captured here are trivially relocatable
        // (Render commands mostly capture pointers and basic types).
        // If a command captures std::shared_ptr or std::string, moving via memcpy is NOT standards-compliant, 
        // but it works under typical ABI because addresses of the captured variables themselves don't change relative to the buffer.
        m_Buffer.insert(m_Buffer.end(), other.m_Buffer.begin(), other.m_Buffer.end());
        
        // Notice we don't call clear on other because that would invoke destructors!
        // We just clear the vector without destroying elements since we moved them.
        other.m_Buffer.clear(); 
    }

private:
    struct CommandHeader {
        void (*execute)(const uint8_t*);
        void (*destroy)(uint8_t*);
        uint32_t commandSize;
        uint16_t dataOffset;
        uint16_t padding;
    };

    std::vector<uint8_t> m_Buffer;
};