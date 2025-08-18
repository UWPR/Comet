#ifndef COMBINATION_ITERATOR_HPP
#define COMBINATION_ITERATOR_HPP

#include <vector>
#include <functional>

namespace AScoreProCpp {

            /**
             * Class generates all unique subsets from the input set of the specified size.
             */
    class CombinationIterator {
    public:
        /**
         * Generates and processes all subsets of the given size from the input set.
         * Instead of returning an IEnumerable like in C#, we use a callback function
         * to process each combination.
         *
         * @param set The input set of elements to select from
         * @param sampleSize The size of the subsets to generate
         * @param callback Function to call for each generated combination
         * @tparam T The type of the elements in the set
         */
        template<typename T>
        void Generate(const std::vector<T>& set, int sampleSize,
            const std::function<void(const std::vector<T>&)>& callback) {
            std::vector<T> subset;
            subset.reserve(sampleSize);
            MakeCombinations(set, 0, sampleSize, subset, callback);
        }

    private:
        /**
         * Recursive function to generate all possible subsets of the specified size.
         * Calls the callback function when the subset reaches the specified size.
         *
         * @param set Input set
         * @param i Current index in the input set
         * @param sampleSize Input specifying the sample size
         * @param subset The currently running subset
         * @param callback Function to call for each generated combination
         */
        template<typename T>
        void MakeCombinations(const std::vector<T>& set, int i, int sampleSize,
            std::vector<T>& subset,
            const std::function<void(const std::vector<T>&)>& callback) {
            if (subset.size() == sampleSize) {
                callback(subset);
                return;
            }

            if (i >= set.size()) {
                return;
            }

            // Include current element
            subset.push_back(set[i]);
            MakeCombinations(set, i + 1, sampleSize, subset, callback);

            // Exclude current element
            subset.pop_back();
            MakeCombinations(set, i + 1, sampleSize, subset, callback);
        }
    };
} // namespace MPToolkit

#endif // COMBINATION_ITERATOR_HPP