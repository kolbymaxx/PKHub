#include "pkhub/backends/RawSaveBackend.hpp"

#include <fstream>
#include <iterator>
#include <string>

namespace pkhub {

RawSaveBackend::RawSaveBackend(std::string path) : path_(std::move(path)) {}

RawSaveFormat detectRawFormat(const std::string& path, const std::vector<uint8_t>& data) {
    (void)data;
    const auto dot = path.find_last_of('.');
    if (dot == std::string::npos) {
        return RawSaveFormat::Unknown;
    }
    const auto ext = path.substr(dot + 1);
    if (ext == "sav" || ext == "srm") {
        // Size-based GBA vs NDS refinement comes in Phase 1.
        if (data.size() == 0x20000 || data.size() == 0x20010) {
            return RawSaveFormat::GbaSav;
        }
        return RawSaveFormat::NdsSav;
    }
    if (ext == "dsv") {
        return RawSaveFormat::NdsDsv;
    }
    return RawSaveFormat::Unknown;
}

SaveOpenStatus RawSaveBackend::open() {
    std::ifstream in(path_, std::ios::binary);
    if (!in) {
        return {SaveOpenResult::NotFound, "Cannot open " + path_};
    }
    raw_.assign(std::istreambuf_iterator<char>(in), std::istreambuf_iterator<char>());
    format_ = detectRawFormat(path_, raw_);
    if (format_ == RawSaveFormat::Unknown) {
        return {SaveOpenResult::Unsupported, "Unrecognized save format"};
    }

    // Skeleton empty boxes — parsers fill later.
    const std::size_t boxCount = (format_ == RawSaveFormat::GbaSav) ? 14 : 24;
    boxes_.assign(boxCount, Box{kDefaultBoxSlots});
    for (std::size_t i = 0; i < boxes_.size(); ++i) {
        boxes_[i].setName("Box " + std::to_string(i + 1));
    }
    party_ = Party{};
    open_ = true;
    dirty_ = false;
    return {SaveOpenResult::Ok, "RawSaveBackend skeleton open"};
}

void RawSaveBackend::close() {
    raw_.clear();
    boxes_.clear();
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
    // TODO(phase1): write raw_ back to path_
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
