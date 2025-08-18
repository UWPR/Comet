#pragma once

#include <unordered_map>
#include <vector>
#include <iterator>
#include "API.h"
#include "PeptideMod.h"

namespace AScoreProCpp {

    /**
     * Composite class to store the modification info for a peptide.
     */
    class ASCORE_API PeptideMods {
    public:
        // Forward declare the iterator classes
        class Iterator;
        class ConstIterator;

        PeptideMods() = default;

        /**
         * Add the mod to the specified position on the peptide.
         */
        void add(int position, const PeptideMod& mod);

        /**
         * Get all mods present at the specified position on the peptide.
         */
        std::vector<PeptideMod> getMods(int pos) const;

        /**
         * Create a copy of this object
         */
        PeptideMods clone() const;

        /**
         * Check if there are no modifications present.
         * @return true if there are no modifications, false otherwise.
         */
        bool empty() const;

        /**
         * Get the total number of modifications.
         * @return The number of modifications.
         */
        size_t size() const;

        // Iterator begin/end methods
        Iterator begin();
        Iterator end();
        ConstIterator begin() const;
        ConstIterator end() const;

    private:
        std::unordered_map<int, std::vector<PeptideMod>> mods_;
    };

    /**
     * Iterator implementation for PeptideMods
     */
    class ASCORE_API PeptideMods::Iterator {
    public:
        using iterator_category = std::forward_iterator_tag;
        using value_type = PeptideMod;
        using difference_type = std::ptrdiff_t;
        using pointer = PeptideMod*;
        using reference = PeptideMod&;

        Iterator(std::unordered_map<int, std::vector<PeptideMod>>* mods, bool begin);

        const PeptideMod& operator*() const;
        Iterator& operator++();
        bool operator!=(const Iterator& other) const;

    private:
        void findNextMod();

        std::unordered_map<int, std::vector<PeptideMod>>* mods_;
        std::vector<int> positions_;
        size_t currentPosIndex_;
        size_t currentModIndex_;
        bool end_;
    };

    /**
     * Const iterator implementation for PeptideMods
     */
    class ASCORE_API PeptideMods::ConstIterator {
    public:
        using iterator_category = std::forward_iterator_tag;
        using value_type = PeptideMod;
        using difference_type = std::ptrdiff_t;
        using pointer = const PeptideMod*;
        using reference = const PeptideMod&;

        ConstIterator(const std::unordered_map<int, std::vector<PeptideMod>>* mods, bool begin);

        const PeptideMod& operator*() const;
        ConstIterator& operator++();
        bool operator!=(const ConstIterator& other) const;

    private:
        void findNextMod();

        const std::unordered_map<int, std::vector<PeptideMod>>* mods_;
        std::vector<int> positions_;
        size_t currentPosIndex_;
        size_t currentModIndex_;
        bool end_;
    };

} // namespace AScoreProCpp