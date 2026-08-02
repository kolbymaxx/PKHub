#include "pkhub/backends/SaveProbe.hpp"

#include "pkhub/backends/raw/GbaGen3.hpp"
#include "pkhub/backends/switch/SwishCrypto.hpp"
#include "pkhub/core/fs/Paths.hpp"

#include <algorithm>
#include <cctype>
#include <cstring>

namespace pkhub {
namespace {

std::string toLowerCopy(std::string s) {
    for (char& c : s) {
        c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    }
    return s;
}

std::string basenameOf(const std::string& path) {
    const auto pos = path.find_last_of('/');
    if (pos == std::string::npos) {
        return path;
    }
    return path.substr(pos + 1);
}

std::string stemOf(const std::string& path) {
    std::string base = basenameOf(path);
    const auto dot = base.find_last_of('.');
    if (dot == std::string::npos) {
        return base;
    }
    return base.substr(0, dot);
}

std::string extensionLower(const std::string& path) {
    const auto dot = path.find_last_of('.');
    if (dot == std::string::npos) {
        return {};
    }
    return toLowerCopy(path.substr(dot + 1));
}

bool containsToken(const std::string& hay, const char* needle) {
    return hay.find(needle) != std::string::npos;
}

bool sizeInGbaBand(std::size_t n) {
    return n >= gba::kGbaSaveSize && n <= gba::kGbaSaveSize + 0x100;
}

bool sizeInNdsBand(std::size_t n) {
    // Common DS/DSi save sizes (raw and with Desmume/melonDS footers).
    static constexpr std::size_t kSizes[] = {
        0x40000, 0x80000, 0x100000, 0x200000, 0x80000 + 122, 0x100000 + 122,
    };
    for (std::size_t s : kSizes) {
        if (n == s || (n > s && n < s + 256)) {
            return true;
        }
    }
    return n >= 0x40000 && n <= 0x200000 + 256;
}

constexpr uint32_t kBoxSwShSv = 0x0d66012c;
constexpr uint32_t kPartySv = 0x3AA1A9AD;
constexpr uint32_t kPartySwSh = 0x2985fe5d;

bool tryProbeSwish(const std::vector<uint8_t>& data, SaveProbeResult& out, GameId pathHint) {
    if (data.size() < 0x1000 || !SwishCrypto::isHashValid(data)) {
        return false;
    }
    std::vector<ScBlock> blocks;
    std::string err;
    if (!SwishCrypto::decrypt(data, blocks, &err)) {
        return false;
    }
    const bool hasBox = SwishCrypto::findBlock(blocks, kBoxSwShSv) != nullptr;
    const bool hasSv = SwishCrypto::findBlock(blocks, kPartySv) != nullptr;
    const bool hasSwSh = SwishCrypto::findBlock(blocks, kPartySwSh) != nullptr;
    if (!hasBox && !hasSv && !hasSwSh) {
        return false;
    }

    out.isSwishDump = true;
    out.confidence = 90;
    out.format = RawSaveFormat::Unknown;  // not a RetroArch raw format
    if (hasSv && !hasSwSh) {
        out.generation = Generation::Gen9;
        out.formatHint = "SV dump";
        if (pathHint == GameId::Scarlet || pathHint == GameId::Violet) {
            out.game = pathHint;
        } else {
            out.game = GameId::Scarlet;
        }
        out.displayName = std::string(gameDisplayName(out.game)) + " (dump)";
    } else if (hasSwSh && !hasSv) {
        out.generation = Generation::Gen8;
        out.formatHint = "SwSh dump";
        if (pathHint == GameId::Sword || pathHint == GameId::Shield) {
            out.game = pathHint;
        } else {
            out.game = GameId::Sword;
        }
        out.displayName = std::string(gameDisplayName(out.game)) + " (dump)";
    } else {
        out.generation = Generation::Gen8;
        out.formatHint = "Switch dump";
        out.game = pathHint != GameId::Unknown ? pathHint : GameId::Unknown;
        out.displayName = "Switch Pokémon dump";
    }
    // Official mount path is preferred; raw dump open is not wired yet.
    out.formatSupported = false;
    out.unsupportedReason =
        "Detected SwishCrypto save dump. Open this title from the Games tab "
        "(hold R while launching the game), or use an official mounted save.";
    return true;
}

}  // namespace

GameId guessGameFromPath(const std::string& path) {
    const std::string p = toLowerCopy(path);
    // More specific tokens first.
    if (containsToken(p, "legends") && (containsToken(p, "arceus") || containsToken(p, "_la") ||
                                        containsToken(p, "/la/") || containsToken(p, "pla"))) {
        return GameId::LegendsArceus;
    }
    if (containsToken(p, "z-a") || containsToken(p, "za_") || containsToken(p, "legendsza")) {
        return GameId::LegendsZA;
    }
    if (containsToken(p, "brilliant") || containsToken(p, "bdsp") || containsToken(p, "bd_")) {
        return GameId::BrilliantDiamond;
    }
    if (containsToken(p, "shining") && containsToken(p, "pearl")) {
        return GameId::ShiningPearl;
    }
    if (containsToken(p, "scarlet")) {
        return GameId::Scarlet;
    }
    if (containsToken(p, "violet")) {
        return GameId::Violet;
    }
    if (containsToken(p, "sword")) {
        return GameId::Sword;
    }
    if (containsToken(p, "shield")) {
        return GameId::Shield;
    }
    if (containsToken(p, "ultrasun") || containsToken(p, "ultra_sun") || containsToken(p, "usum") ||
        containsToken(p, "ultra sun")) {
        return GameId::UltraSun;
    }
    if (containsToken(p, "ultramoon") || containsToken(p, "ultra_moon") ||
        containsToken(p, "ultra moon")) {
        return GameId::UltraMoon;
    }
    if (containsToken(p, "omega") && containsToken(p, "ruby")) {
        return GameId::OmegaRuby;
    }
    if (containsToken(p, "alpha") && containsToken(p, "sapphire")) {
        return GameId::AlphaSapphire;
    }
    if (containsToken(p, "black2") || containsToken(p, "black_2") || containsToken(p, "b2w2") ||
        containsToken(p, "black 2")) {
        return GameId::Black2;
    }
    if (containsToken(p, "white2") || containsToken(p, "white_2") || containsToken(p, "white 2")) {
        return GameId::White2;
    }
    if (containsToken(p, "heartgold") || containsToken(p, "heart_gold") || containsToken(p, "hgss") ||
        containsToken(p, "heart gold")) {
        return GameId::HeartGold;
    }
    if (containsToken(p, "soulsilver") || containsToken(p, "soul_silver") ||
        containsToken(p, "soul silver")) {
        return GameId::SoulSilver;
    }
    if (containsToken(p, "firered") || containsToken(p, "fire_red") || containsToken(p, "frlg") ||
        containsToken(p, "fire red")) {
        return GameId::FireRed;
    }
    if (containsToken(p, "leafgreen") || containsToken(p, "leaf_green") ||
        containsToken(p, "leaf green")) {
        return GameId::LeafGreen;
    }
    if (containsToken(p, "emerald")) {
        return GameId::Emerald;
    }
    if (containsToken(p, "sapphire")) {
        return GameId::Sapphire;
    }
    if (containsToken(p, "ruby")) {
        return GameId::Ruby;
    }
    if (containsToken(p, "platinum")) {
        return GameId::Platinum;
    }
    if (containsToken(p, "diamond")) {
        return GameId::Diamond;
    }
    if (containsToken(p, "pearl")) {
        return GameId::Pearl;
    }
    if (containsToken(p, "black")) {
        return GameId::Black;
    }
    if (containsToken(p, "white")) {
        return GameId::White;
    }
    if (containsToken(p, "/sun") || containsToken(p, "pokemon_sun") || containsToken(p, "pokémon sun")) {
        return GameId::Sun;
    }
    if (containsToken(p, "/moon") || containsToken(p, "pokemon_moon")) {
        return GameId::Moon;
    }
    if (containsToken(p, "pokemon_x") || containsToken(p, "/x.sav") || containsToken(p, " poke_x")) {
        return GameId::X;
    }
    if (containsToken(p, "pokemon_y") || containsToken(p, "/y.sav")) {
        return GameId::Y;
    }
    return GameId::Unknown;
}

SaveProbeResult probeSaveBytes(const std::string& path, const std::vector<uint8_t>& data) {
    SaveProbeResult out;
    out.displayName = basenameOf(path);
    const GameId pathHint = guessGameFromPath(path);
    const std::string ext = extensionLower(path);
    const std::string stem = toLowerCopy(stemOf(path));

    // Official Switch `main` dumps (often extensionless).
    if (stem == "main" || ext.empty() || ext == "bin" || ext == "sav") {
        if (tryProbeSwish(data, out, pathHint)) {
            if (out.displayName.empty()) {
                out.displayName = basenameOf(path);
            }
            return out;
        }
    }

    // Gen 3 GBA — require section signature, not size alone.
    if ((ext == "sav" || ext == "srm" || ext.empty()) && sizeInGbaBand(data.size())) {
        const auto gba = gba::probeSave(data);
        if (gba.looksLikeGba) {
            out.format = RawSaveFormat::GbaSav;
            out.generation = Generation::Gen3;
            out.formatSupported = true;
            out.confidence = gba.validSections >= 10 ? 95 : 75;
            out.game = gba.game;
            if (out.game == GameId::Unknown && pathHint != GameId::Unknown &&
                generationFor(pathHint) == Generation::Gen3) {
                out.game = pathHint;
                out.confidence = std::min(out.confidence, 70);
            }
            if (out.game != GameId::Unknown) {
                out.formatHint = std::string("GBA · ") + gameDisplayName(out.game);
                out.displayName = gameDisplayName(out.game);
            } else {
                out.formatHint = "GBA · Gen 3";
                out.displayName = basenameOf(path);
            }
            return out;
        }
        // 128KB but no magic — not a Pokémon Gen 3 save.
        if (ext == "sav" || ext == "srm") {
            out.format = RawSaveFormat::Unknown;
            out.formatHint = "128KB (not Gen 3)";
            out.formatSupported = false;
            out.unsupportedReason =
                "File is GBA-sized but missing Gen 3 section signatures (0x08012025).";
            out.confidence = 40;
            if (pathHint != GameId::Unknown) {
                out.game = pathHint;
                out.displayName = basenameOf(path);
            }
            return out;
        }
    }

    if (ext == "dsv") {
        out.format = RawSaveFormat::NdsDsv;
        out.generation = Generation::Gen4;
        out.formatHint = "DSV";
        out.formatSupported = false;
        out.unsupportedReason = "NDS (.dsv) parser is not implemented yet.";
        out.confidence = 60;
        if (pathHint != GameId::Unknown) {
            out.game = pathHint;
            out.generation = generationFor(pathHint);
            out.formatHint = std::string("DSV · ") + gameDisplayName(pathHint);
            out.displayName = gameDisplayName(pathHint);
            out.confidence = 75;
        }
        return out;
    }

    if (ext == "sav" || ext == "srm") {
        if (sizeInNdsBand(data.size())) {
            out.format = RawSaveFormat::NdsSav;
            out.generation = Generation::Gen4;
            out.formatHint = "NDS";
            out.formatSupported = false;
            out.unsupportedReason = "NDS save parser is not implemented yet.";
            out.confidence = 55;
            if (pathHint != GameId::Unknown) {
                out.game = pathHint;
                out.generation = generationFor(pathHint);
                out.formatHint = std::string("NDS · ") + gameDisplayName(pathHint);
                out.displayName = gameDisplayName(pathHint);
                out.confidence = 80;
            }
            return out;
        }
        out.format = RawSaveFormat::Unknown;
        out.formatHint = "RAW";
        out.formatSupported = false;
        out.unsupportedReason = "Unrecognized save size for .sav/.srm.";
        out.confidence = 20;
        if (pathHint != GameId::Unknown) {
            out.game = pathHint;
            out.generation = generationFor(pathHint);
            out.displayName = basenameOf(path);
        }
        return out;
    }

    out.format = RawSaveFormat::Unknown;
    out.formatHint = "RAW";
    out.formatSupported = false;
    out.unsupportedReason = "Unrecognized save format.";
    return out;
}

}  // namespace pkhub
