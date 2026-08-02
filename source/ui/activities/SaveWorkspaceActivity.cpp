#include "pkhub/ui/activities/MainActivity.hpp"

#include "pkhub/app/AppContext.hpp"
#include "pkhub/core/pokemon/NatureNames.hpp"
#include "pkhub/core/pokemon/SpeciesIds.hpp"
#include "pkhub/core/pokemon/SpeciesNames.hpp"
#include "pkhub/core/safety/LegalityService.hpp"
#include "pkhub/core/safety/SafetyPolicy.hpp"
#include "pkhub/ui/SpriteService.hpp"
#include "pkhub/ui/UiList.hpp"
#include "pkhub/ui/views/BoxGridView.hpp"

#if defined(PKHUB_HAS_BOREALIS)
#include <borealis/views/cells/cell_bool.hpp>
#include <borealis/views/cells/cell_detail.hpp>
#endif

namespace pkhub::ui {

#if defined(PKHUB_HAS_BOREALIS)

brls::View* buildSaveWorkspace(IBoxProvider* provider, const std::string& title) {
    auto* body = new brls::Box(brls::Axis::COLUMN);
    body->setPadding(16);
    body->setGrow(1.f);
    body->setBackgroundColor(nvgRGB(10, 16, 20));

    auto* toolbar = new brls::Box(brls::Axis::ROW);
    toolbar->setPaddingBottom(12);
    toolbar->setAlignItems(brls::AlignItems::CENTER);
    auto* prev = new brls::Button();
    prev->setText("◀ Box");
    auto* next = new brls::Button();
    next->setText("Box ▶");
    auto* partyBtn = new brls::Button();
    partyBtn->setText("Party");
    auto* saveBtn = new brls::Button();
    saveBtn->setText("Save");
    toolbar->addView(prev);
    toolbar->addView(next);
    toolbar->addView(partyBtn);
    toolbar->addView(saveBtn);
    body->addView(toolbar);

    auto* grid = new BoxGridView();
    if (provider) {
        grid->bindProvider(provider, 0);
        grid->onSlotActivated = [provider, grid](std::size_t slot) {
            brls::Application::pushActivity(new brls::Activity(
                buildEditor(provider, false, grid->currentBoxIndex(), slot)));
        };
    }
    body->addView(grid);

    prev->registerClickAction([provider, grid](brls::View*) {
        if (!provider || provider->boxCount() == 0) {
            return true;
        }
        const auto i = (grid->currentBoxIndex() + provider->boxCount() - 1) % provider->boxCount();
        grid->setBoxIndex(i);
        return true;
    });
    next->registerClickAction([provider, grid](brls::View*) {
        if (!provider || provider->boxCount() == 0) {
            return true;
        }
        const auto i = (grid->currentBoxIndex() + 1) % provider->boxCount();
        grid->setBoxIndex(i);
        return true;
    });
    partyBtn->registerClickAction([provider](brls::View*) {
        if (!provider) {
            return true;
        }
        auto list = makeScrollList();
        list.content->addView(makeSectionHeader("Party"));
        for (std::size_t i = 0; i < provider->party().size(); ++i) {
            const auto& mon = provider->party().slot(i);
            const std::string detail =
                mon.empty() ? "Empty"
                            : (pokemonDisplayName(mon) + "  Lv" +
                               std::to_string(mon.level) +
                               (mon.isShiny ? "  ✦" : ""));
            addClickableDetail(list.content, "Slot " + std::to_string(i + 1), detail,
                               [provider, i](brls::View*) {
                                   brls::Application::pushActivity(new brls::Activity(
                                       buildEditor(provider, true, 0, i)));
                                   return true;
                               });
        }
        brls::Application::pushActivity(
            new brls::Activity(makeAppletFrame("Party", list.scroll)));
        return true;
    });
    saveBtn->registerClickAction([](brls::View*) {
        auto& ctx = AppContext::instance();
        SafetyPolicy policy;
        const auto decision = policy.evaluate(SafetyAction::WriteSave);
        auto st = ctx.session().saveAll(ctx.session().hasBackend(), ctx.hub().isDirty());
        brls::Application::notify(st.result == SaveOpenResult::Ok
                                      ? ("Saved. " + decision.message)
                                      : st.message);
        return true;
    });

    return makeAppletFrame(title.empty() ? "Save Workspace" : title, body);
}

brls::View* buildEditor(IBoxProvider* provider,
                        bool fromParty,
                        std::size_t boxIndex,
                        std::size_t slotIndex) {
    Pokemon* mon = nullptr;
    if (provider) {
        if (fromParty) {
            if (slotIndex < provider->party().size()) {
                mon = &provider->party().slot(slotIndex);
            }
        } else if (boxIndex < provider->boxCount()) {
            auto& box = provider->box(boxIndex);
            if (slotIndex < box.size()) {
                mon = &box.slot(slotIndex);
            }
        }
    }

    auto list = makeScrollList();
    if (!mon || mon->empty()) {
        list.content->addView(makeSectionHeader("Empty"));
        addClickableDetail(list.content, "Empty slot",
                           "Create tools / import coming next", nullptr);
    } else {
        auto* hero = new brls::Box(brls::Axis::ROW);
        hero->setAlignItems(brls::AlignItems::CENTER);
        hero->setPadding(8, 12, 16, 12);
        auto* sprite = new brls::Image();
        sprite->setWidth(72);
        sprite->setHeight(72);
        sprite->setScalingType(brls::ImageScalingType::FIT);
        const auto path = SpriteService::spritePath(*mon);
        if (!path.empty()) {
            sprite->setImageFromRes(path);
        }
        hero->addView(sprite);
        auto* meta = new brls::Box(brls::Axis::COLUMN);
        meta->setPaddingLeft(14);
        auto* name = new brls::Label();
        name->setFontSize(24);
        name->setTextColor(nvgRGB(235, 245, 240));
        name->setText(pokemonDisplayName(*mon) + (mon->isShiny ? "  ✦" : ""));
        meta->addView(name);
        auto* sub = new brls::Label();
        sub->setFontSize(14);
        sub->setTextColor(nvgRGBA(150, 180, 165, 230));
        {
            const uint16_t nat = nationalDexId(*mon);
            const char* speciesName = speciesEnglishName(nat);
            std::string line = "Lv " + std::to_string(mon->level) + "  ·  #" +
                               std::to_string(nat);
            if (speciesName && *speciesName) {
                line += " ";
                line += speciesName;
            }
            if (mon->nativeGeneration == Generation::Gen9) {
                line += "  ·  SV#" + std::to_string(mon->species);
            }
            sub->setText(line);
        }
        meta->addView(sub);
        hero->addView(meta);
        list.content->addView(hero);

        list.content->addView(makeSectionHeader("Identity"));
        addClickableDetail(list.content, "Species",
                           std::string(speciesEnglishName(nationalDexId(*mon))) + " (#" +
                               std::to_string(nationalDexId(*mon)) + ")",
                           nullptr);

        auto* shiny = new brls::BooleanCell();
        shiny->init("Shiny", mon->isShiny, [mon](bool on) { mon->isShiny = on; });
        list.content->addView(shiny);

        auto* level = makeDetailCell("Level", std::to_string(mon->level));
        level->registerClickAction([mon, level](brls::View*) {
            mon->level = uint8_t(mon->level >= 100 ? 1 : mon->level + 1);
            level->setDetailText(std::to_string(mon->level));
            return true;
        });
        list.content->addView(level);

        auto natureLabel = [](const Pokemon& m) -> std::string {
            const char* n = natureEnglishName(m.nature);
            if (n && *n) {
                return n;
            }
            return std::to_string(m.nature);
        };
        auto* nature = makeDetailCell("Nature", natureLabel(*mon));
        nature->registerClickAction([mon, nature, natureLabel](brls::View*) {
            const int n = ((mon->nature < 0 ? 0 : int(mon->nature)) + 1) % 25;
            mon->nature = int8_t(n);
            nature->setDetailText(natureLabel(*mon));
            return true;
        });
        list.content->addView(nature);

        auto* ability = makeDetailCell("Ability ID", std::to_string(mon->abilityId));
        ability->registerClickAction([mon, ability](brls::View*) {
            mon->abilityId = uint16_t(mon->abilityId + 1);
            ability->setDetailText(std::to_string(mon->abilityId));
            return true;
        });
        list.content->addView(ability);

        auto* abilitySlot =
            makeDetailCell("Ability slot", std::to_string(mon->abilitySlot));
        abilitySlot->registerClickAction([mon, abilitySlot](brls::View*) {
            const int s = ((mon->abilitySlot < 0 ? 0 : int(mon->abilitySlot)) + 1) % 4;
            mon->abilitySlot = int8_t(s);
            abilitySlot->setDetailText(std::to_string(mon->abilitySlot));
            return true;
        });
        list.content->addView(abilitySlot);

        list.content->addView(makeSectionHeader("Stats"));
        addClickableDetail(
            list.content, "IVs HP/Atk/Def/SpA/SpD/Spe",
            std::to_string(mon->ivs.hp) + "/" + std::to_string(mon->ivs.atk) + "/" +
                std::to_string(mon->ivs.def) + "/" + std::to_string(mon->ivs.spa) + "/" +
                std::to_string(mon->ivs.spd) + "/" + std::to_string(mon->ivs.spe),
            nullptr);

        addClickableDetail(list.content, "Set IVs 31", "Max all individual values",
                           [mon](brls::View*) {
                               mon->ivs = Stats{31, 31, 31, 31, 31, 31};
                               brls::Application::notify("IVs set to 31");
                               return true;
                           });

        addClickableDetail(
            list.content, "EVs HP/Atk/Def/SpA/SpD/Spe",
            std::to_string(mon->evs.hp) + "/" + std::to_string(mon->evs.atk) + "/" +
                std::to_string(mon->evs.def) + "/" + std::to_string(mon->evs.spa) + "/" +
                std::to_string(mon->evs.spd) + "/" + std::to_string(mon->evs.spe),
            nullptr);

        addClickableDetail(list.content, "Clear EVs", "Reset effort values to 0",
                           [mon](brls::View*) {
                               mon->evs = Stats{};
                               brls::Application::notify("EVs cleared");
                               return true;
                           });

        list.content->addView(makeSectionHeader("Moves"));
        for (int i = 0; i < 4; ++i) {
            auto* mv = makeDetailCell("Move " + std::to_string(i + 1),
                                      std::to_string(mon->moves[std::size_t(i)]));
            mv->registerClickAction([mon, i, mv](brls::View*) {
                mon->moves[std::size_t(i)] = uint16_t(mon->moves[std::size_t(i)] + 1);
                mv->setDetailText(std::to_string(mon->moves[std::size_t(i)]));
                return true;
            });
            list.content->addView(mv);
        }

        if (mon->nativeGeneration == Generation::Gen9 || mon->teraType != PokemonType::None) {
            list.content->addView(makeSectionHeader("Scarlet / Violet"));
            auto* tera = makeDetailCell("Tera type", std::to_string(int(mon->teraType)));
            tera->registerClickAction([mon, tera](brls::View*) {
                int t = int(mon->teraType);
                if (t < 0 || t > 18) {
                    t = 0;
                } else {
                    t = (t + 1) % 19;
                }
                mon->teraType = static_cast<PokemonType>(t);
                tera->setDetailText(std::to_string(int(mon->teraType)));
                return true;
            });
            list.content->addView(tera);
        }

        SafetyPolicy policy;
        LegalityService legality;
        const auto report = legality.evaluate(*mon, provider->gameId());
        const auto gate = policy.evaluate(SafetyAction::EditPokemon, &report);
        if (gate.gate != SafetyGate::Allow) {
            list.content->addView(makeSectionHeader("Safety"));
            addClickableDetail(list.content, gate.title, gate.message, nullptr);
        }
    }

    return makeAppletFrame("Editor", list.scroll);
}

#endif

}  // namespace pkhub::ui
