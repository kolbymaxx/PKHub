#include "pkhub/backends/UnsupportedSaveBackend.hpp"

#include <stdexcept>

namespace pkhub {

UnsupportedSaveBackend::UnsupportedSaveBackend(GameId game, std::string reason)
    : game_(game), reason_(std::move(reason)) {}

SaveOpenStatus UnsupportedSaveBackend::open() {
    return {SaveOpenResult::Unsupported, reason_};
}

SaveOpenStatus UnsupportedSaveBackend::reload() {
    return open();
}

SaveOpenStatus UnsupportedSaveBackend::commit() {
    return {SaveOpenResult::Unsupported, reason_};
}

std::string UnsupportedSaveBackend::displayName() const {
    return gameDisplayName(game_);
}

Generation UnsupportedSaveBackend::generation() const {
    return generationFor(game_);
}

GameId UnsupportedSaveBackend::gameId() const {
    return game_;
}

Box& UnsupportedSaveBackend::box(std::size_t) {
    throw std::logic_error("UnsupportedSaveBackend has no boxes");
}

const Box& UnsupportedSaveBackend::box(std::size_t) const {
    throw std::logic_error("UnsupportedSaveBackend has no boxes");
}

Party& UnsupportedSaveBackend::party() {
    throw std::logic_error("UnsupportedSaveBackend has no party");
}

const Party& UnsupportedSaveBackend::party() const {
    throw std::logic_error("UnsupportedSaveBackend has no party");
}

}  // namespace pkhub
