#include "pkhub/backends/RawSaveBackend.hpp"

#include "pkhub/backends/SaveProbe.hpp"
#include "pkhub/backends/raw/GbaGen3.hpp"
#include "pkhub/core/fs/Paths.hpp"

#include <fstream>
#include <iterator>
#include <string>

namespace pkhub {

RawSaveBackend::RawSaveBackend(std::string path) : path_(std::move(path)) {}

RawSaveFormat detectRawFormat(const std::string& path, const std::vector<uint8_t>& data) {
    return probeSaveBytes(path, data).format;
}

SaveOpenStatus RawSaveBackend::open() {
    const std::string resolved = fs::resolvePath(path_);
    std::ifstream in(resolved, std::ios::binary);
    if (!in) {
        return {SaveOpenResult::NotFound, "Cannot open " + resolved};
    }
    raw_.assign(std::istreambuf_iterator<char>(in), std::istreambuf_iterator<char>());
    const SaveProbeResult probe = probeSaveBytes(path_, raw_);
    format_ = probe.format;
    if (format_ == RawSaveFormat::Unknown) {
        return {SaveOpenResult::Unsupported,
                probe.unsupportedReason.empty() ? "Unrecognized save format"
                                                : probe.unsupportedReason};
    }

    if (format_ == RawSaveFormat::GbaSav) {
        const auto parsed = gba::parseSave(raw_);
        if (!parsed.ok) {
            return {SaveOpenResult::Corrupt, parsed.message};
        }
        game_ = parsed.game;
        party_ = parsed.party;
        boxes_ = parsed.boxes;
        open_ = true;
        dirty_ = false;
        return {SaveOpenResult::Ok, parsed.message};
    }

    // Other formats: skeleton empty boxes until their parsers land.
    const std::size_t boxCount = 24;
    boxes_.assign(boxCount, Box{kDefaultBoxSlots});
    for (std::size_t i = 0; i < boxes_.size(); ++i) {
        boxes_[i].setName("Box " + std::to_string(i + 1));
    }
    party_ = Party{};
    game_ = probe.game;
    open_ = true;
    dirty_ = false;
    return {SaveOpenResult::Ok, "RawSaveBackend open (parser pending for this format)"};
}

void RawSaveBackend::close() {
    raw_.clear();
    boxes_.clear();
    party_ = Party{};
    game_ = GameId::Unknown;
    open_ = false;
    dirty_ = false;
}

SaveOpenStatus RawSaveBackend::reload() {
    close();
    return open();
}

SaveOpenStatus RawSaveBackend::commit() {
    if (!open_) {
        return {SaveOpenResult::NotFound, "Not open"};
    }
    if (format_ == RawSaveFormat::GbaSav) {
        std::string err;
        if (!gba::writeSave(raw_, game_, party_, boxes_, &err)) {
            return {SaveOpenResult::IoError,
                    err.empty() ? "GBA write-back failed" : err};
        }
        const std::string resolved = fs::resolvePath(path_);
        std::ofstream out(resolved, std::ios::binary | std::ios::trunc);
        if (!out) {
            return {SaveOpenResult::IoError, "Cannot write " + resolved};
        }
        out.write(reinterpret_cast<const char*>(raw_.data()),
                  static_cast<std::streamsize>(raw_.size()));
        if (!out) {
            return {SaveOpenResult::IoError, "Failed writing " + resolved};
        }
        dirty_ = false;
        return {SaveOpenResult::Ok, "GBA save written"};
    }
    dirty_ = false;
    return {SaveOpenResult::Ok, "RawSaveBackend commit stub"};
}

std::string RawSaveBackend::displayName() const {
    return path_;
}

Generation RawSaveBackend::generation() const {
    return generationFor(game_);
}

GameId RawSaveBackend::gameId() const {
    return game_;
}

std::size_t RawSaveBackend::boxCount() const {
    return boxes_.size();
}

Box& RawSaveBackend::box(std::size_t index) {
    dirty_ = true;
    return boxes_.at(index);
}

const Box& RawSaveBackend::box(std::size_t index) const {
    return boxes_.at(index);
}

Party& RawSaveBackend::party() {
    dirty_ = true;
    return party_;
}

const Party& RawSaveBackend::party() const {
    return party_;
}

}  // namespace pkhub
