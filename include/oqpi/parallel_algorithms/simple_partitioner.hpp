#pragma once

#include <atomic>
#include <cstdint>

#include "oqpi/parallel_algorithms/base_partitioner.hpp"


namespace oqpi {

    //----------------------------------------------------------------------------------------------
    // Partitioner dividing a set of indices into fixed size batches and giving one batch to each 
    // worker.
    //
    struct simple_partitioner
        : public base_partitioner
    {
        simple_partitioner(int32_t firstIndex, int32_t lastIndex, int32_t maxBatches)
            : base_partitioner      (firstIndex, lastIndex, maxBatches)
            , nbElementsPerBatch_   ((elementCount_ >= maxBatches) ? (elementCount_ / batchCount_) : 1)
            , remainder_            ((elementCount_ >= maxBatches) ? (elementCount_ % batchCount_) : 0)
            , batchIndex_           (0)
        {}

        simple_partitioner(int32_t elementsCount, int32_t maxBatches)
            : simple_partitioner(0, elementsCount, maxBatches)
        {}

        simple_partitioner(const simple_partitioner &other)
            : base_partitioner      (other)
            , nbElementsPerBatch_   (other.nbElementsPerBatch_)
            , remainder_            (other.remainder_)
            , batchIndex_           (other.batchIndex_.load())
        {}

        inline bool getNextValidRange(int32_t &fromIndex, int32_t &toIndex)
        {
            const auto batchIndex = batchIndex_++;
            if (batchIndex >= batchCount_)
            {
                return false;
            }
            
            fromIndex = firstIndexOfBatch(batchIndex);
            toIndex   = lastIndexOfBatch(batchIndex);

            return true;
        }

    private:
        inline int32_t firstIndexOfBatch(int32_t batchIndex) const
        {
            return (batchIndex > 0) ? lastIndexOfBatch(batchIndex - 1) : 0;
        }

        inline int32_t lastIndexOfBatch(int32_t batchIndex) const
        {
            const auto offset = (batchIndex >= remainder_) ? remainder_ : batchIndex + 1;
            return firstIndex_ + (batchIndex + 1) * nbElementsPerBatch_ + offset;
        }

    private:
        const int32_t           nbElementsPerBatch_;
        const int32_t           remainder_;
        std::atomic<int32_t>    batchIndex_;
    };

} /*oqpi*/
