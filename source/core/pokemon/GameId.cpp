#include "pkhub/core/pokemon/GameId.hpp"
#include "pkhub/platform/TitleIds.hpp"

namespace pkhub {

Generation generationFor(GameId id) {
    switch (id) {
        case GameId::Ruby:
        case GameId::Sapphire:
        case GameId::Emerald:
        case GameId::FireRed:
        case GameId::LeafGreen:
            return Generation::Gen3;
        case GameId::Diamond:
        case GameId::Pearl:
        case GameId::Platinum:
        case GameId::HeartGold:
        case GameId::SoulSilver:
            return Generation::Gen4;
        case GameId::Black:
        case GameId::White:
        case GameId::Black2:
        case GameId::White2:
            return Generation::Gen5;
        case GameId::X:
        case GameId::Y:
        case GameId::OmegaRuby:
        case GameId::AlphaSapphire:
            return Generation::Gen6;
        case GameId::Sun:
        case GameId::Moon:
        case GameId::UltraSun:
        case GameId::UltraMoon:
            return Generation::Gen7;
        case GameId::Sword:
        case GameId::Shield:
        case GameId::BrilliantDiamond:
        case GameId::ShiningPearl:
        case GameId::LegendsArceus:
            return Generation::Gen8;
        case GameId::Scarlet:
        case GameId::Violet:
        case GameId::LegendsZA:
            return Generation::Gen9;
        default:
            return Generation::Unknown;
    }
}

const char* gameDisplayName(GameId id) {
    switch (id) {
        case GameId::Ruby: return "Pokémon Ruby";
        case GameId::Sapphire: return "Pokémon Sapphire";
        case GameId::Emerald: return "Pokémon Emerald";
        case GameId::FireRed: return "Pokémon FireRed";
        case GameId::LeafGreen: return "Pokémon LeafGreen";
        case GameId::Diamond: return "Pokémon Diamond";
        case GameId::Pearl: return "Pokémon Pearl";
        case GameId::Platinum: return "Pokémon Platinum";
        case GameId::HeartGold: return "Pokémon HeartGold";
        case GameId::SoulSilver: return "Pokémon SoulSilver";
        case GameId::Black: return "Pokémon Black";
        case GameId::White: return "Pokémon White";
        case GameId::Black2: return "Pokémon Black 2";
        case GameId::White2: return "Pokémon White 2";
        case GameId::X: return "Pokémon X";
        case GameId::Y: return "Pokémon Y";
        case GameId::OmegaRuby: return "Pokémon Omega Ruby";
        case GameId::AlphaSapphire: return "Pokémon Alpha Sapphire";
        case GameId::Sun: return "Pokémon Sun";
        case GameId::Moon: return "Pokémon Moon";
        case GameId::UltraSun: return "Pokémon Ultra Sun";
        case GameId::UltraMoon: return "Pokémon Ultra Moon";
        case GameId::Sword: return "Pokémon Sword";
        case GameId::Shield: return "Pokémon Shield";
        case GameId::BrilliantDiamond: return "Pokémon Brilliant Diamond";
        case GameId::ShiningPearl: return "Pokémon Shining Pearl";
        case GameId::LegendsArceus: return "Pokémon Legends: Arceus";
        case GameId::Scarlet: return "Pokémon Scarlet";
        case GameId::Violet: return "Pokémon Violet";
        case GameId::LegendsZA: return "Pokémon Legends: Z-A";
        default: return "Unknown Game";
    }
}

uint64_t switchTitleId(GameId id) {
    switch (id) {
        case GameId::Sword: return title_ids::Sword;
        case GameId::Shield: return title_ids::Shield;
        case GameId::BrilliantDiamond: return title_ids::BrilliantDiamond;
        case GameId::ShiningPearl: return title_ids::ShiningPearl;
        case GameId::LegendsArceus: return title_ids::LegendsArceus;
        case GameId::Scarlet: return title_ids::Scarlet;
        case GameId::Violet: return title_ids::Violet;
        case GameId::LegendsZA: return title_ids::LegendsZA;
        default: return 0;
    }
}

}  // namespace pkhub
