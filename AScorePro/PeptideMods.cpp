#include "PeptideMods.h"
#include <algorithm>

namespace AScoreProCpp {

    void PeptideMods::add(int position, const PeptideMod& mod) {
        if (mods_.find(position) == mods_.end()) {
            mods_[position] = std::vector<PeptideMod>();
        }

        PeptideMod modCopy = mod.clone();
        modCopy.setPosition(position);
        mods_[position].push_back(modCopy);
    }

    std::vector<PeptideMod> PeptideMods::getMods(int pos) const {
        auto it = mods_.find(pos);
        if (it != mods_.end()) {
            return it->second;
        }
        return std::vector<PeptideMod>();
    }

    PeptideMods PeptideMods::clone() const {
        PeptideMods copy;
        copy.mods_ = this->mods_;
        return copy;
    }

    bool PeptideMods::empty() const {
        // Check if the map is empty
        if (mods_.empty()) {
            return true;
        }

        // Check if all vectors in the map are empty
        for (const auto& pair : mods_) {
            if (!pair.second.empty()) {
                return false;
            }
        }

        // All vectors are empty
        return true;
    }

    size_t PeptideMods::size() const {
        size_t count = 0;

        // Count total number of modifications across all positions
        for (const auto& pair : mods_) {
            count += pair.second.size();
        }

        return count;
    }

    // Iterator implementation
    PeptideMods::Iterator::Iterator(std::unordered_map<int, std::vector<PeptideMod>>* mods, bool begin)
        : mods_(mods), currentPosIndex_(0), currentModIndex_(0), end_(!begin) {

        if (begin && mods_ && !mods_->empty()) {
            // Get all position keys and sort them
            positions_.reserve(mods_->size());
            for (const auto& pair : *mods_) {
                positions_.push_back(pair.first);
            }
            std::sort(positions_.begin(), positions_.end());

            // If the first position has an empty vector, find the next valid position
            if (mods_->at(positions_[0]).empty()) {
                findNextMod();
            }
        }
    }

    const PeptideMod& PeptideMods::Iterator::operator*() const {
        return mods_->at(positions_[currentPosIndex_])[currentModIndex_];
    }

    PeptideMods::Iterator& PeptideMods::Iterator::operator++() {
        ++currentModIndex_;
        findNextMod();
        return *this;
    }

    bool PeptideMods::Iterator::operator!=(const Iterator& other) const {
        if (end_ && other.end_) {
            return false;
        }
        if (end_ || other.end_) {
            return true;
        }
        return currentPosIndex_ != other.currentPosIndex_ ||
            currentModIndex_ != other.currentModIndex_;
    }

    void PeptideMods::Iterator::findNextMod() {
        // If we've reached the end of the current position's mods
        if (currentModIndex_ >= mods_->at(positions_[currentPosIndex_]).size()) {
            ++currentPosIndex_;
            currentModIndex_ = 0;

            // If we've reached the end of all positions
            if (currentPosIndex_ >= positions_.size()) {
                end_ = true;
                return;
            }

            // If the current position has an empty vector, continue searching
            if (mods_->at(positions_[currentPosIndex_]).empty()) {
                findNextMod();
            }
        }
    }

    PeptideMods::Iterator PeptideMods::begin() {
        return Iterator(&mods_, true);
    }

    PeptideMods::Iterator PeptideMods::end() {
        return Iterator(&mods_, false);
    }

    // Const iterator implementation
    PeptideMods::ConstIterator::ConstIterator(const std::unordered_map<int, std::vector<PeptideMod>>* mods, bool begin)
        : mods_(mods), currentPosIndex_(0), currentModIndex_(0), end_(!begin) {

        if (begin && mods_ && !mods_->empty()) {
            // Get all position keys and sort them
            positions_.reserve(mods_->size());
            for (const auto& pair : *mods_) {
                positions_.push_back(pair.first);
            }
            std::sort(positions_.begin(), positions_.end());

            // If the first position has an empty vector, find the next valid position
            if (mods_->at(positions_[0]).empty()) {
                findNextMod();
            }
        }
    }

    const PeptideMod& PeptideMods::ConstIterator::operator*() const {
        return mods_->at(positions_[currentPosIndex_])[currentModIndex_];
    }

    PeptideMods::ConstIterator& PeptideMods::ConstIterator::operator++() {
        ++currentModIndex_;
        findNextMod();
        return *this;
    }

    bool PeptideMods::ConstIterator::operator!=(const ConstIterator& other) const {
        if (end_ && other.end_) {
            return false;
        }
        if (end_ || other.end_) {
            return true;
        }
        return currentPosIndex_ != other.currentPosIndex_ ||
            currentModIndex_ != other.currentModIndex_;
    }

    void PeptideMods::ConstIterator::findNextMod() {
        // If we've reached the end of the current position's mods
        if (currentModIndex_ >= mods_->at(positions_[currentPosIndex_]).size()) {
            ++currentPosIndex_;
            currentModIndex_ = 0;

            // If we've reached the end of all positions
            if (currentPosIndex_ >= positions_.size()) {
                end_ = true;
                return;
            }

            // If the current position has an empty vector, continue searching
            if (mods_->at(positions_[currentPosIndex_]).empty()) {
                findNextMod();
            }
        }
    }

    PeptideMods::ConstIterator PeptideMods::begin() const {
        return ConstIterator(&mods_, true);
    }

    PeptideMods::ConstIterator PeptideMods::end() const {
        return ConstIterator(&mods_, false);
    }

} // namespace AScoreProCpp