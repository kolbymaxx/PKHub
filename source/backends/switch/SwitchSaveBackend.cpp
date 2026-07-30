#include "pkhub/backends/SwitchSaveBackend.hpp"

#include <string>

namespace pkhub {

SwitchSaveBackend::SwitchSaveBackend(GameId game,
                                     uint64_t titleId,
                                     SwitchUserId userId,
                                     SaveAccessMode accessMode)
    : game_(game), titleId_(titleId), userId_(userId), accessMode_(accessMode) {
    (void)userId_;
    (void)titleId_;
}

SaveOpenStatus SwitchSaveBackend::open() {
    // Phase 1 mount order when accessMode_ == Auto:
    //  1) Title override (most reliable full save access)
    //  2) fsOpen_SaveData + user picker
    // Device libnx mount lands in a follow-up; desktop uses empty placeholder boxes.
    (void)accessMode_;

    boxes_.assign(32, Box{kDefaultBoxSlots});
    for (std::size_t i = 0; i < boxes_.size(); ++i) {
        boxes_[i].setName("Box " + std::to_string(i + 1));
    }
    party_ = Party{};
    open_ = true;
    dirty_ = false;

#if defined(__SWITCH__)
    return {SaveOpenResult::Ok,
            "Mounted skeleton — prefer title override; fsOpen_SaveData parse TBD"};
#else
    return {SaveOpenResult::Ok,
            "Desktop placeholder boxes for " + std::string(gameDisplayName(game_)) +
                " (Switch mount requires device build)"};
#endif
}

void SwitchSaveBackend::close() {
    boxes_.clear();
    open_ = false;
    dirty_ = false;
}

SaveOpenStatus SwitchSaveBackend::reload() {
    close();
    return open();
}

SaveOpenStatus SwitchSaveBackend::commit() {
    if (!open_) {
        return {SaveOpenResult::NotFound, "Not open"};
    }
    // TODO(phase1): serialize + fs write
    dirty_ = false;
    return {SaveOpenResult::Ok, "SwitchSaveBackend commit stub"};
}

std::string SwitchSaveBackend::displayName() const {
    return gameDisplayName(game_);
}

Generation SwitchSaveBackend::generation() const {
    return generationFor(game_);
}

GameId SwitchSaveBackend::gameId() const {
    return game_;
}

std::size_t SwitchSaveBackend::boxCount() const {
    return boxes_.size();
}

Box& SwitchSaveBackend::box(std::size_t index) {
    dirty_ = true;
    return boxes_.at(index);
}

const Box& SwitchSaveBackend::box(std::size_t index) const {
    return boxes_.at(index);
}

Party& SwitchSaveBackend::party() {
    dirty_ = true;
    return party_;
}

const Party& SwitchSaveBackend::party() const {
    return party_;
}

}  // namespace pkhub
