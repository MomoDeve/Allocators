#include <cstdint>
#include <cstddef>
#include <vector>
#include <functional>

namespace Allocators
{
    class FreeListAllocator
    {
        struct Block
        {
            size_t Offset;
            size_t Size;
            bool IsFree;
        };

        std::function<void(size_t)> onResize;
        size_t currentSize = 0;
        std::vector<Block> blocks;

        [[nodiscard]] size_t AllocateInMeshEntry(typename std::vector<Block>::iterator meshEntry, size_t size)
        {
            size_t entryOffset = meshEntry->Offset;
            size_t entrySize = meshEntry->Size;
            meshEntry->IsFree = false;
            if (size < meshEntry->Size)
            {
                meshEntry->Size = size;
                this->blocks.insert(meshEntry + 1, Block{ entryOffset + size, entrySize - size, true });
            }
            return entryOffset;
        }

        void Merge(typename std::vector<Block>::iterator meshEntry)
        {
            typename std::vector<Block>::iterator it;
            for (it = meshEntry + 1; it != this->blocks.end() && it->IsFree; it++)
            {
                meshEntry->Size += it->Size;
            }
            this->blocks.erase(meshEntry + 1, it);

            if (meshEntry == this->blocks.begin()) return;
            for (it = meshEntry; it != this->blocks.begin(); it--)
            {
                auto& prevEntry = *(it - 1);
                if (!prevEntry.IsFree) break;
                meshEntry->Size += prevEntry.Size;
                meshEntry->Offset = prevEntry.Offset;
            }
            this->blocks.erase(it, meshEntry);
        }
    public:
        void Init(size_t initialSize, std::function<void(size_t)> onResize)
        {
            this->currentSize = initialSize;
            this->blocks.push_back(Block{ 0, initialSize, true });
            this->onResize = std::move(onResize);
            this->onResize(initialSize);
        }

        void Resize(size_t newSize)
        {
            this->blocks.push_back(Block{ this->currentSize, newSize - this->currentSize, true });
            this->currentSize = newSize;
            this->Merge(this->blocks.end() - 1);
            this->onResize(newSize);
        }

        [[nodiscard]] size_t Allocate(size_t size)
        {
            for (auto meshEntry = this->blocks.begin(); meshEntry != this->blocks.end(); meshEntry++)
            {
                if (meshEntry->IsFree && size <= meshEntry->Size)
                {
                    return this->AllocateInMeshEntry(meshEntry, size);
                }
            }
            this->Resize(2 * (this->currentSize + size));
            return this->AllocateInMeshEntry(this->blocks.end() - 1, size);
        }

        void Deallocate(size_t offset)
        {
            for (auto meshEntry = this->blocks.begin(); meshEntry != this->blocks.end(); meshEntry++)
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
}