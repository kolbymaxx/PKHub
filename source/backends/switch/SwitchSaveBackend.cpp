#include "pkhub/backends/SwitchSaveBackend.hpp"

#include "pkhub/backends/switch/SwitchSaveParser.hpp"

#include <string>

namespace pkhub {

SwitchSaveBackend::SwitchSaveBackend(GameId game,
                                     uint64_t titleId,
                                     SwitchUserId userId,
                                     SaveAccessMode accessMode)
    : game_(game), titleId_(titleId), userId_(userId), accessMode_(accessMode) {}

SaveOpenStatus SwitchSaveBackend::open() {
    SwitchMountRequest req;
    req.titleId = titleId_;
    req.mode = accessMode_;
    req.user = userId_;
    req.saveFileName = "main";

    auto mounted = mount_.mountAndRead(req);
    modeUsed_ = mounted.modeUsed;
    mountMessage_ = mounted.message;

    if (mounted.status != MountStatus::Ok) {
        open_ = false;
        SaveOpenResult code = SaveOpenResult::IoError;
        if (mounted.status == MountStatus::NotFound) {
            code = SaveOpenResult::NotFound;
        } else if (mounted.status == MountStatus::PermissionDenied ||
                   mounted.status == MountStatus::TitleMismatch ||
                   mounted.status == MountStatus::UserRequired) {
            code = SaveOpenResult::PermissionDenied;
        } else if (mounted.status == MountStatus::NotAvailable) {
            code = SaveOpenResult::Unsupported;
        }
        return {code, mounted.message};
    }

    raw_ = std::move(mounted.data);
    saveFileName_ = mounted.saveFileName;

    auto parsed = parseSwitchSave(game_, raw_);
    if (!parsed.ok) {
        mount_.unmount();
        return {SaveOpenResult::Corrupt, parsed.message};
    }
    boxes_ = std::move(parsed.boxes);
    party_ = std::move(parsed.party);
    parseImplemented_ = parsed.parseImplemented;
    open_ = true;
    dirty_ = false;

    std::string msg = mounted.message;
    if (!parsed.message.empty()) {
        msg += " | " + parsed.message;
    }
    mountMessage_ = msg;
    return {SaveOpenResult::Ok, msg};
}

void SwitchSaveBackend::close() {
    mount_.unmount();
    boxes_.clear();
    party_ = Party{};
    raw_.clear();
    open_ = false;
    dirty_ = false;
    parseImplemented_ = false;
}

SaveOpenStatus SwitchSaveBackend::reload() {
    close();
    return open();
}

SaveOpenStatus SwitchSaveBackend::commit() {
    if (!open_) {
        return {SaveOpenResult::NotFound, "Not open"};
    }

    std::vector<uint8_t> out;
    std::string err;
    if (!serializeSwitchSave(game_, raw_, party_, boxes_, out, &err)) {
        return {SaveOpenResult::IoError, err};
    }

    SwitchMountRequest req;
    req.titleId = titleId_;
    req.mode = modeUsed_;
    req.user = userId_;
    req.saveFileName = saveFileName_;

    auto written = mount_.write(req, modeUsed_, saveFileName_, out);
    if (written.status != MountStatus::Ok) {
        return {SaveOpenResult::IoError, written.message};
    }
    raw_ = std::move(out);
    dirty_ = false;
    return {SaveOpenResult::Ok, written.message};
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
