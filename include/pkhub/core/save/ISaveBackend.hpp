#pragma once

#include <cstddef>
#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "pkhub/core/pokemon/GameId.hpp"
#include "pkhub/core/pokemon/Pokemon.hpp"

namespace pkhub {

constexpr std::size_t kDefaultBoxSlots = 30;
constexpr std::size_t kPartySlots = 6;

class Box {
public:
    explicit Box(std::size_t capacity = kDefaultBoxSlots);

    std::size_t size() const { return slots_.size(); }
    Pokemon& slot(std::size_t index);
    const Pokemon& slot(std::size_t index) const;

    const std::string& name() const { return name_; }
    void setName(std::string name);

    std::size_t occupiedCount() const;

private:
    std::string name_;
    std::vector<Pokemon> slots_;
};

class Party {
public:
    Party();

    std::size_t size() const { return slots_.size(); }
    Pokemon& slot(std::size_t index);
    const Pokemon& slot(std::size_t index) const;
    std::size_t occupiedCount() const;

private:
    std::vector<Pokemon> slots_;
};

/// Anything that exposes boxes for viewing / transfer (saves and Hub).
class IBoxProvider {
public:
    virtual ~IBoxProvider() = default;

    virtual std::string displayName() const = 0;
    virtual Generation generation() const = 0;
    virtual GameId gameId() const = 0;

    virtual std::size_t boxCount() const = 0;
    virtual Box& box(std::size_t index) = 0;
    virtual const Box& box(std::size_t index) const = 0;

    virtual Party& party() = 0;
    virtual const Party& party() const = 0;

    virtual bool isReadOnly() const { return false; }
};

enum class SaveOpenResult {
    Ok,
    NotFound,
    Unsupported,
    Corrupt,
    PermissionDenied,
    IoError,
};

struct SaveOpenStatus {
    SaveOpenResult result = SaveOpenResult::NotFound;
    std::string message;
};

/**
 * Common save backend interface.
 * SwitchSaveBackend and RawSaveBackend both implement this.
 */
class ISaveBackend : public IBoxProvider {
public:
    ~ISaveBackend() override = default;

    virtual SaveOpenStatus open() = 0;
    virtual void close() = 0;
    virtual bool isOpen() const = 0;

    /// Reload in-memory state from storage.
    virtual SaveOpenStatus reload() = 0;

    /// Persist in-memory state. Caller must backup first.
    virtual SaveOpenStatus commit() = 0;

    virtual bool isDirty() const = 0;
    virtual void markClean() = 0;

    /// Optional bag/inventory — empty if unsupported.
    virtual bool hasInventory() const { return false; }
};

using SaveBackendPtr = std::unique_ptr<ISaveBackend>;

}  // namespace pkhub
