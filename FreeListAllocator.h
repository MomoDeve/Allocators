#include <cstdint>
#include <cstddef>
#include <vector>
#include <functional>

class FreeListAllocator
{
    struct MeshEntry
    {
        size_t Offset;
        size_t Size;
        bool IsFree;
    };

    std::function<void(size_t)> onResize;
    size_t currentSize = 0;
    std::vector<MeshEntry> meshEntries;

    [[nodiscard]] size_t AllocateInMeshEntry(typename std::vector<MeshEntry>::iterator meshEntry, size_t size)
    {
        size_t entryOffset = meshEntry->Offset;
        size_t entrySize = meshEntry->Size;
        if (size < meshEntry->Size)
        {
            meshEntry->Size = size;
            meshEntry->IsFree = false;
            this->meshEntries.insert(meshEntry + 1, MeshEntry{ entryOffset + size, entrySize - size, true });
        }
        return entryOffset;
    }

    void Merge(typename std::vector<MeshEntry>::iterator meshEntry)
    {
        typename std::vector<MeshEntry>::iterator it;
        for (it = meshEntry + 1; it != this->meshEntries.end() && it->IsFree; it++)
        {
            meshEntry->Size += it->Size;
        }
        this->meshEntries.erase(meshEntry + 1, it);

        if (meshEntry == this->meshEntries.begin()) return;
        for (it = meshEntry; it != this->meshEntries.begin(); it--)
        {
            auto& prevEntry = *(it - 1);
            if (!prevEntry.IsFree) break;
            meshEntry->Size += prevEntry.Size;
            meshEntry->Offset = prevEntry.Offset;
        }
        this->meshEntries.erase(it, meshEntry);
    }
public:
    void Init(size_t initialSize, std::function<void(size_t)> onResize)
    {
        this->currentSize = initialSize;
        this->meshEntries.push_back(MeshEntry{ 0, initialSize, true });
        this->onResize = std::move(onResize);
        this->onResize(initialSize);
    }

    void Resize(size_t newSize)
    {
        this->meshEntries.push_back(MeshEntry{ this->currentSize, newSize - this->currentSize, true });
        this->currentSize = newSize;
        this->Merge(this->meshEntries.end() - 1);
        this->onResize(newSize);
    }

    [[nodiscard]] size_t Allocate(size_t size)
    {
        for (auto meshEntry = this->meshEntries.begin(); meshEntry != this->meshEntries.end(); meshEntry++)
        {
            if (meshEntry->IsFree && size <= meshEntry->Size)
            {
                return this->AllocateInMeshEntry(meshEntry, size);
            }
        }
        this->Resize(2 * (this->currentSize + size));
        return this->AllocateInMeshEntry(this->meshEntries.end() - 1, size);
    }
    
    void Deallocate(size_t offset)
    {
        for (auto meshEntry = this->meshEntries.begin(); meshEntry != this->meshEntries.end(); meshEntry++)
        {
            if (meshEntry->Offset == offset)
            {
                meshEntry->IsFree = true;
                this->Merge(meshEntry);
                break;
            }
        }
    }
};
