#include "pkhub/core/hub/HubStorage.hpp"

#include <cstddef>
#include <string>

namespace pkhub {

HubStorage::HubStorage(HubConfig config) : config_(std::move(config)) {}

HubStorage::~HubStorage() {
    close();
}

SaveOpenStatus HubStorage::openOrCreate() {
    boxes_.clear();
    boxes_.reserve(config_.defaultBoxCount);
    for (std::size_t i = 0; i < config_.defaultBoxCount; ++i) {
        Box box(config_.slotsPerBox);
        box.setName("Box " + std::to_string(i + 1));
        boxes_.push_back(std::move(box));
    }
    party_ = Party{};
    open_ = true;
    dirty_ = false;
    // Phase 1 follow-up: load hub.pkhub from config_.rootPath if present.
    return {SaveOpenResult::Ok, "Hub ready (in-memory skeleton)"};
}

SaveOpenStatus HubStorage::reload() {
    if (!open_) {
        return {SaveOpenResult::NotFound, "Hub is not open"};
    }
    return openOrCreate();
}

SaveOpenStatus HubStorage::commit() {
    if (!open_) {
        return {SaveOpenResult::NotFound, "Hub is not open"};
    }
    // Phase 1 follow-up: serialize to sdmc:/switch/PKHub/hub/hub.pkhub
    dirty_ = false;
    return {SaveOpenResult::Ok, "Hub commit stub OK"};
}

void HubStorage::close() {
    boxes_.clear();
    open_ = false;
    dirty_ = false;
}

std::string HubStorage::displayName() const {
    return "Hub Storage";
}

Generation HubStorage::generation() const {
    return Generation::Unknown;  // multi-gen container
}

GameId HubStorage::gameId() const {
    return GameId::Unknown;
}

std::size_t HubStorage::boxCount() const {
    return boxes_.size();
}

Box& HubStorage::box(std::size_t index) {
    dirty_ = true;
    return boxes_.at(index);
}

const Box& HubStorage::box(std::size_t index) const {
    return boxes_.at(index);
}

Party& HubStorage::party() {
    dirty_ = true;
    return party_;
}

const Party& HubStorage::party() const {
    return party_;
}

SaveOpenStatus HubStorage::addBox(const std::string& name) {
    if (!open_) {
        return {SaveOpenResult::NotFound, "Hub is not open"};
    }
    Box box(config_.slotsPerBox);
    box.setName(name.empty() ? ("Box " + std::to_string(boxes_.size() + 1)) : name);
    boxes_.push_back(std::move(box));
    dirty_ = true;
    return {SaveOpenResult::Ok, {}};
}

SaveOpenStatus HubStorage::removeBox(std::size_t index) {
    if (!open_ || index >= boxes_.size()) {
        return {SaveOpenResult::NotFound, "Invalid box"};
    }
    if (boxes_[index].occupiedCount() > 0) {
        return {SaveOpenResult::IoError, "Box is not empty"};
    }
    boxes_.erase(boxes_.begin() + static_cast<std::ptrdiff_t>(index));
    dirty_ = true;
    return {SaveOpenResult::Ok, {}};
}

}  // namespace pkhub
