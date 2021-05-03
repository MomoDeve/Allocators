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
    size_t currentByteSize = 0;
    std::vector<MeshEntry> meshEntries;

    [[nodiscard]] size_t AllocateInMeshEntry(typename std::vector<MeshEntry>::iterator meshEntry, size_t byteSize)
    {
        size_t offset = meshEntry->Offset;
        size_t size = meshEntry->Size;
        if (byteSize < meshEntry->Size)
        {
            meshEntry->Size = byteSize;
            meshEntry->IsFree = false;
            this->meshEntries.insert(meshEntry + 1, MeshEntry{ offset + byteSize, size - byteSize, true });
        }
        return offset;
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
    void Init(size_t initialByteSize, std::function<void(size_t)> onResize)
    {
        this->currentByteSize = initialByteSize;
        this->meshEntries.push_back(MeshEntry{ 0, initialByteSize, true });
        this->onResize = std::move(onResize);
        this->onResize(initialByteSize);
    }

    void Resize(size_t newByteSize)
    {
        this->meshEntries.push_back(MeshEntry{ this->currentByteSize, newByteSize - this->currentByteSize, true });
        this->currentByteSize = newByteSize;
        this->Merge(this->meshEntries.end() - 1);
        this->onResize(newByteSize);
    }

    [[nodiscard]] size_t Allocate(size_t byteSize)
    {
        for (auto meshEntry = this->meshEntries.begin(); meshEntry != this->meshEntries.end(); meshEntry++)
        {
            if (meshEntry->IsFree && byteSize <= meshEntry->Size)
            {
                return this->AllocateInMeshEntry(meshEntry, byteSize);
            }
        }
        this->Resize(2 * (this->currentByteSize + byteSize));
        return this->AllocateInMeshEntry(this->meshEntries.end() - 1, byteSize);
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
