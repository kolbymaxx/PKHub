#include "pkhub/core/hub/HubStorage.hpp"

#include "pkhub/core/fs/Paths.hpp"
#include "pkhub/core/hub/HubFormat.hpp"
#include "pkhub/core/hub/HubPokemonCodec.hpp"

#include "json.hpp"

#include <cstdio>
#include <fstream>
#include <string>
#include <vector>

namespace pkhub {
namespace {

using json = nlohmann::json;

void writeU16Le(std::vector<uint8_t>& out, uint16_t v) {
    out.push_back(static_cast<uint8_t>(v & 0xFF));
    out.push_back(static_cast<uint8_t>((v >> 8) & 0xFF));
}

void writeU32Le(std::vector<uint8_t>& out, uint32_t v) {
    out.push_back(static_cast<uint8_t>(v & 0xFF));
    out.push_back(static_cast<uint8_t>((v >> 8) & 0xFF));
    out.push_back(static_cast<uint8_t>((v >> 16) & 0xFF));
    out.push_back(static_cast<uint8_t>((v >> 24) & 0xFF));
}

bool readAllFile(const std::string& path, std::vector<uint8_t>& out) {
    std::ifstream in(fs::resolvePath(path), std::ios::binary);
    if (!in) {
        return false;
    }
    in.seekg(0, std::ios::end);
    const auto size = in.tellg();
    if (size < 0) {
        return false;
    }
    out.resize(static_cast<std::size_t>(size));
    in.seekg(0, std::ios::beg);
    if (size > 0) {
        in.read(reinterpret_cast<char*>(out.data()), size);
        if (!in) {
            return false;
        }
    }
    return true;
}

bool writeAllFile(const std::string& path, const std::vector<uint8_t>& data) {
    std::ofstream out(fs::resolvePath(path), std::ios::binary | std::ios::trunc);
    if (!out) {
        return false;
    }
    if (!data.empty()) {
        out.write(reinterpret_cast<const char*>(data.data()),
                  static_cast<std::streamsize>(data.size()));
    }
    return static_cast<bool>(out);
}

uint16_t readU16At(const uint8_t* p) {
    return static_cast<uint16_t>(p[0] | (static_cast<uint16_t>(p[1]) << 8));
}

uint32_t readU32At(const uint8_t* p) {
    return static_cast<uint32_t>(p[0]) | (static_cast<uint32_t>(p[1]) << 8) |
           (static_cast<uint32_t>(p[2]) << 16) | (static_cast<uint32_t>(p[3]) << 24);
}

std::vector<Pokemon> slotsFromBox(const Box& box) {
    std::vector<Pokemon> slots;
    slots.reserve(box.size());
    for (std::size_t i = 0; i < box.size(); ++i) {
        slots.push_back(box.slot(i));
    }
    return slots;
}

std::vector<Pokemon> slotsFromParty(const Party& party) {
    std::vector<Pokemon> slots;
    slots.reserve(party.size());
    for (std::size_t i = 0; i < party.size(); ++i) {
        slots.push_back(party.slot(i));
    }
    return slots;
}

}  // namespace

HubStorage::HubStorage(HubConfig config) : config_(std::move(config)) {}

HubStorage::~HubStorage() {
    close();
}

std::string HubStorage::joinPath(const std::string& relative) const {
    if (relative.empty()) {
        return config_.rootPath;
    }
    if (!config_.rootPath.empty() && config_.rootPath.back() == '/') {
        return config_.rootPath + relative;
    }
    return config_.rootPath + "/" + relative;
}

std::string HubStorage::boxesDirPath() const {
    return joinPath(hub_format::kDataDirName);
}

std::string HubStorage::hubJsonPath() const {
    return joinPath(hub_format::kHubJsonName);
}

std::string HubStorage::holdingPkboxPath() const {
    return joinPath(hub_format::kHoldingPkboxName);
}

std::string HubStorage::boxPkboxPath(std::size_t index) const {
    char name[32];
    std::snprintf(name, sizeof(name), "%04zu.pkbox", index);
    return boxesDirPath() + "/" + name;
}

void HubStorage::resetInMemoryDefaults() {
    boxes_.clear();
    boxes_.reserve(config_.defaultBoxCount);
    for (std::size_t i = 0; i < config_.defaultBoxCount; ++i) {
        Box box(config_.slotsPerBox);
        box.setName("Box " + std::to_string(i + 1));
        boxes_.push_back(std::move(box));
    }
    party_ = Party{};
}

SaveOpenStatus HubStorage::openOrCreate() {
    if (!fs::createDirectories(config_.rootPath) || !fs::createDirectories(boxesDirPath())) {
        return {SaveOpenResult::IoError, "Failed to create hub directories"};
    }

    if (fs::fileExists(hubJsonPath())) {
        auto st = loadFromDisk();
        if (st.result != SaveOpenResult::Ok) {
            return st;
        }
        open_ = true;
        dirty_ = false;
        return {SaveOpenResult::Ok, "Hub loaded"};
    }

    resetInMemoryDefaults();
    open_ = true;
    dirty_ = true;
    auto st = writeToDisk();
    if (st.result != SaveOpenResult::Ok) {
        open_ = false;
        dirty_ = false;
        boxes_.clear();
        return st;
    }
    dirty_ = false;
    return {SaveOpenResult::Ok, "Hub created"};
}

SaveOpenStatus HubStorage::reload() {
    if (!open_) {
        return {SaveOpenResult::NotFound, "Hub is not open"};
    }
    if (!fs::fileExists(hubJsonPath())) {
        return {SaveOpenResult::NotFound, "hub.json missing"};
    }
    auto st = loadFromDisk();
    if (st.result == SaveOpenResult::Ok) {
        dirty_ = false;
    }
    return st;
}

SaveOpenStatus HubStorage::commit() {
    if (!open_) {
        return {SaveOpenResult::NotFound, "Hub is not open"};
    }
    auto st = writeToDisk();
    if (st.result == SaveOpenResult::Ok) {
        dirty_ = false;
    }
    return st;
}

void HubStorage::close() {
    boxes_.clear();
    party_ = Party{};
    open_ = false;
    dirty_ = false;
}

SaveOpenStatus HubStorage::loadFromDisk() {
    auto st = loadHubJson();
    if (st.result != SaveOpenResult::Ok) {
        return st;
    }

    for (std::size_t i = 0; i < boxes_.size(); ++i) {
        const auto path = boxPkboxPath(i);
        std::vector<Pokemon> slots;
        if (fs::fileExists(path)) {
            st = loadPkboxFile(path, slots);
            if (st.result != SaveOpenResult::Ok) {
                return {st.result, "Box " + std::to_string(i) + ": " + st.message};
            }
        } else {
            slots.assign(boxes_[i].size(), Pokemon{});
        }
        if (slots.size() != boxes_[i].size()) {
            // Resize box to match file slot count when metadata disagrees.
            Box resized(slots.size());
            resized.setName(boxes_[i].name());
            boxes_[i] = std::move(resized);
        }
        for (std::size_t s = 0; s < slots.size() && s < boxes_[i].size(); ++s) {
            boxes_[i].slot(s) = std::move(slots[s]);
        }
    }

    party_ = Party{};
    if (fs::fileExists(holdingPkboxPath())) {
        std::vector<Pokemon> holding;
        st = loadPkboxFile(holdingPkboxPath(), holding);
        if (st.result != SaveOpenResult::Ok) {
            return {st.result, "holding.pkbox: " + st.message};
        }
        for (std::size_t s = 0; s < holding.size() && s < party_.size(); ++s) {
            party_.slot(s) = std::move(holding[s]);
        }
    }

    return {SaveOpenResult::Ok, "Hub loaded from disk"};
}

SaveOpenStatus HubStorage::writeToDisk() {
    if (!fs::createDirectories(config_.rootPath) || !fs::createDirectories(boxesDirPath())) {
        return {SaveOpenResult::IoError, "Failed to create hub directories"};
    }

    auto st = writeHubJson();
    if (st.result != SaveOpenResult::Ok) {
        return st;
    }

    for (std::size_t i = 0; i < boxes_.size(); ++i) {
        st = writePkboxFile(boxPkboxPath(i), slotsFromBox(boxes_[i]));
        if (st.result != SaveOpenResult::Ok) {
            return {st.result, "Box " + std::to_string(i) + ": " + st.message};
        }
    }

    st = writePkboxFile(holdingPkboxPath(), slotsFromParty(party_));
    if (st.result != SaveOpenResult::Ok) {
        return {st.result, "holding.pkbox: " + st.message};
    }

    return {SaveOpenResult::Ok, "Hub committed"};
}

SaveOpenStatus HubStorage::loadHubJson() {
    std::ifstream in(fs::resolvePath(hubJsonPath()));
    if (!in) {
        return {SaveOpenResult::IoError, "Failed to open hub.json"};
    }

    json doc;
    try {
        in >> doc;
    } catch (const json::exception& ex) {
        return {SaveOpenResult::Corrupt, std::string("hub.json parse error: ") + ex.what()};
    }

    const int formatVersion = doc.value("formatVersion", 0);
    if (formatVersion != hub_format::kJsonFormatVersion) {
        return {SaveOpenResult::Unsupported,
                "Unsupported hub.json formatVersion " + std::to_string(formatVersion)};
    }

    boxes_.clear();
    if (!doc.contains("boxes") || !doc["boxes"].is_array() || doc["boxes"].empty()) {
        resetInMemoryDefaults();
        return {SaveOpenResult::Ok, {}};
    }

    for (const auto& entry : doc["boxes"]) {
        const std::size_t slotCount =
            entry.value("slotCount", static_cast<int>(config_.slotsPerBox));
        Box box(slotCount > 0 ? slotCount : config_.slotsPerBox);
        std::string name = entry.value("name", std::string{});
        if (name.empty()) {
            const int id = entry.value("id", static_cast<int>(boxes_.size()));
            name = "Box " + std::to_string(id + 1);
        }
        box.setName(std::move(name));
        boxes_.push_back(std::move(box));
    }

    return {SaveOpenResult::Ok, {}};
}

SaveOpenStatus HubStorage::writeHubJson() const {
    json boxes = json::array();
    for (std::size_t i = 0; i < boxes_.size(); ++i) {
        boxes.push_back({
            {"id", static_cast<int>(i)},
            {"name", boxes_[i].name()},
            {"notes", ""},
            {"tags", json::array()},
            {"slotCount", static_cast<int>(boxes_[i].size())},
        });
    }

    json doc = {
        {"formatVersion", hub_format::kJsonFormatVersion},
        {"appMinVersion", hub_format::kAppMinVersion},
        {"boxes", std::move(boxes)},
        {"settings", {{"defaultView", "grid"}}},
    };

    std::ofstream out(fs::resolvePath(hubJsonPath()), std::ios::trunc);
    if (!out) {
        return {SaveOpenResult::IoError, "Failed to write hub.json"};
    }
    out << doc.dump(2) << '\n';
    if (!out) {
        return {SaveOpenResult::IoError, "Failed to flush hub.json"};
    }
    return {SaveOpenResult::Ok, {}};
}

SaveOpenStatus HubStorage::loadPkboxFile(const std::string& path, std::vector<Pokemon>& slots) {
    std::vector<uint8_t> data;
    if (!readAllFile(path, data)) {
        return {SaveOpenResult::IoError, "Failed to read " + path};
    }
    if (data.size() < 12) {
        return {SaveOpenResult::Corrupt, "pkbox too small"};
    }

    const uint32_t magic = readU32At(data.data());
    if (magic != hub_format::kPkboxMagic) {
        return {SaveOpenResult::Corrupt, "bad pkbox magic"};
    }
    const uint16_t formatVersion = readU16At(data.data() + 4);
    if (formatVersion != hub_format::kBinaryVersion) {
        return {SaveOpenResult::Unsupported,
                "Unsupported pkbox formatVersion " + std::to_string(formatVersion)};
    }
    const uint16_t slotCount = readU16At(data.data() + 6);
    // flags at +8 currently unused

    slots.assign(slotCount, Pokemon{});
    std::size_t offset = 12;
    for (uint16_t i = 0; i < slotCount; ++i) {
        if (offset + 6 > data.size()) {
            return {SaveOpenResult::Corrupt, "pkbox truncated at slot header"};
        }
        const uint16_t payloadLength = readU16At(data.data() + offset);
        const uint8_t generation = data[offset + 2];
        // reserved at +3
        const uint16_t speciesHint = readU16At(data.data() + offset + 4);
        offset += 6;

        if (payloadLength == 0) {
            continue;
        }
        if (offset + payloadLength > data.size()) {
            return {SaveOpenResult::Corrupt, "pkbox truncated at payload"};
        }

        Pokemon mon;
        if (!hub_codec::decodePokemon(data.data() + offset, payloadLength, mon)) {
            return {SaveOpenResult::Corrupt, "invalid slot payload"};
        }
        if (mon.species == 0 && speciesHint != 0) {
            mon.species = speciesHint;
        }
        if (mon.nativeGeneration == Generation::Unknown && generation != 0) {
            mon.nativeGeneration = static_cast<Generation>(generation);
        }
        slots[i] = std::move(mon);
        offset += payloadLength;
    }

    return {SaveOpenResult::Ok, {}};
}

SaveOpenStatus HubStorage::writePkboxFile(const std::string& path,
                                          const std::vector<Pokemon>& slots) const {
    if (slots.size() > 0xFFFF) {
        return {SaveOpenResult::IoError, "Too many slots for pkbox"};
    }

    std::vector<uint8_t> data;
    data.reserve(12 + slots.size() * 64);
    writeU32Le(data, hub_format::kPkboxMagic);
    writeU16Le(data, hub_format::kBinaryVersion);
    writeU16Le(data, static_cast<uint16_t>(slots.size()));
    writeU32Le(data, 0);  // flags

    for (const auto& mon : slots) {
        if (mon.empty()) {
            writeU16Le(data, 0);
            data.push_back(0);  // generation
            data.push_back(0);  // reserved
            writeU16Le(data, 0);
            continue;
        }

        const auto payload = hub_codec::encodePokemon(mon);
        if (payload.size() > 0xFFFF) {
            return {SaveOpenResult::IoError, "Slot payload too large"};
        }
        writeU16Le(data, static_cast<uint16_t>(payload.size()));
        data.push_back(static_cast<uint8_t>(mon.originGeneration()));
        data.push_back(0);
        writeU16Le(data, mon.species);
        data.insert(data.end(), payload.begin(), payload.end());
    }

    if (!writeAllFile(path, data)) {
        return {SaveOpenResult::IoError, "Failed to write " + path};
    }
    return {SaveOpenResult::Ok, {}};
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
