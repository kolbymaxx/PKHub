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

namespace {

void spaceToolbarButton(brls::View* btn) {
    btn->setMarginRight(10);
}

brls::Box* makePartyRow(const Pokemon& mon, std::size_t index) {
    auto* row = new brls::Box(brls::Axis::ROW);
    row->setAlignItems(brls::AlignItems::CENTER);
    row->setPadding(10, 8, 10, 8);
    row->setCornerRadius(10);
    row->setBackgroundColor(nvgRGBA(18, 28, 34, 210));
    row->setBorderThickness(1.0f);
    row->setBorderColor(mon.empty() ? nvgRGBA(70, 90, 85, 50)
                                    : (mon.isShiny ? nvgRGBA(230, 190, 70, 140)
                                                   : nvgRGBA(90, 160, 135, 90)));
    row->setMarginBottom(8);

    auto* sprite = new brls::Image();
    sprite->setWidth(48);
    sprite->setHeight(48);
    sprite->setScalingType(brls::ImageScalingType::FIT);
    sprite->setCornerRadius(8);
    if (!mon.empty()) {
        const auto path = SpriteService::spritePath(mon);
        if (!path.empty()) {
            sprite->setImageFromRes(path);
        }
    }
    row->addView(sprite);

    auto* col = new brls::Box(brls::Axis::COLUMN);
    col->setPaddingLeft(14);
    col->setGrow(1.f);

    auto* title = new brls::Label();
    title->setFontSize(18);
    title->setTextColor(nvgRGB(230, 242, 236));
    if (mon.empty()) {
        title->setText("Slot " + std::to_string(index + 1));
    } else {
        title->setText(pokemonDisplayName(mon) + (mon.isShiny ? "  ✦" : ""));
    }
    col->addView(title);

    auto* sub = new brls::Label();
    sub->setFontSize(13);
    sub->setTextColor(nvgRGBA(130, 180, 160, 220));
    if (mon.empty()) {
        sub->setText("Empty");
    } else {
        sub->setText("Lv " + std::to_string(mon.level) + "  ·  #" +
                     std::to_string(nationalDexId(mon)));
    }
    col->addView(sub);
    row->addView(col);
    return row;
}

}  // namespace

brls::View* buildSaveWorkspace(IBoxProvider* provider, const std::string& title) {
    auto* body = new brls::Box(brls::Axis::COLUMN);
    body->setPadding(16);
    body->setGrow(1.f);
    body->setBackgroundColor(nvgRGB(10, 16, 20));

    auto* toolbar = new brls::Box(brls::Axis::ROW);
    toolbar->setPaddingBottom(14);
    toolbar->setAlignItems(brls::AlignItems::CENTER);
    auto* prev = new brls::Button();
    prev->setText("◀ Box");
    auto* next = new brls::Button();
    next->setText("Box ▶");
    auto* partyBtn = new brls::Button();
    partyBtn->setText("Party");
    auto* saveBtn = new brls::Button();
    saveBtn->setText("Save");
    spaceToolbarButton(prev);
    spaceToolbarButton(next);
    spaceToolbarButton(partyBtn);
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
        list.content->addView(makeSectionHeader("Party", "Tap a slot to edit"));
        for (std::size_t i = 0; i < provider->party().size(); ++i) {
            const auto& mon = provider->party().slot(i);
            auto* row = makePartyRow(mon, i);
            row->registerClickAction([provider, i](brls::View*) {
                brls::Application::pushActivity(
                    new brls::Activity(buildEditor(provider, true, 0, i)));
                return true;
            });
            list.content->addView(row);
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
        list.content->addView(makeSectionHeader("Empty slot"));
        addBodyLabel(list.content, "Create / import tools are coming next.");
    } else {
        auto* hero = new brls::Box(brls::Axis::ROW);
        hero->setAlignItems(brls::AlignItems::CENTER);
        hero->setPadding(10, 8, 18, 8);
        hero->setCornerRadius(14);
        hero->setBackgroundColor(nvgRGBA(18, 28, 34, 230));
        hero->setBorderThickness(1.0f);
        hero->setBorderColor(mon->isShiny ? nvgRGBA(230, 190, 70, 150)
                                          : nvgRGBA(78, 176, 148, 90));
        hero->setMarginBottom(8);

        auto* spriteFrame = new brls::Box(brls::Axis::COLUMN);
        spriteFrame->setWidth(84);
        spriteFrame->setHeight(84);
        spriteFrame->setCornerRadius(12);
        spriteFrame->setBackgroundColor(nvgRGB(24, 36, 44));
        spriteFrame->setJustifyContent(brls::JustifyContent::CENTER);
        spriteFrame->setAlignItems(brls::AlignItems::CENTER);

        auto* sprite = new brls::Image();
        sprite->setWidth(72);
        sprite->setHeight(72);
        sprite->setScalingType(brls::ImageScalingType::FIT);
        const auto path = SpriteService::spritePath(*mon);
        if (!path.empty()) {
            sprite->setImageFromRes(path);
        }
        spriteFrame->addView(sprite);
        hero->addView(spriteFrame);

        auto* meta = new brls::Box(brls::Axis::COLUMN);
        meta->setPaddingLeft(16);
        auto* name = new brls::Label();
        name->setFontSize(26);
        name->setTextColor(nvgRGB(235, 248, 242));
        name->setText(pokemonDisplayName(*mon) + (mon->isShiny ? "  ✦" : ""));
        meta->addView(name);

        auto* sub = new brls::Label();
        sub->setFontSize(15);
        sub->setTextColor(nvgRGBA(130, 185, 165, 235));
        {
            const uint16_t nat = nationalDexId(*mon);
            std::string line = "Lv " + std::to_string(mon->level) + "  ·  #" +
                               std::to_string(nat);
            const char* speciesName = speciesEnglishName(nat);
            if (speciesName && *speciesName) {
                line += " ";
                line += speciesName;
            }
            sub->setText(line);
        }
        meta->addView(sub);

        if (mon->nativeGeneration == Generation::Gen9) {
            auto* sv = new brls::Label();
            sv->setFontSize(13);
            sv->setTextColor(nvgRGBA(110, 150, 135, 200));
            sv->setText("SV internal id " + std::to_string(mon->species));
            meta->addView(sv);
        }
        hero->addView(meta);
        list.content->addView(hero);

        list.content->addView(makeSectionHeader("Edit"));

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
            list.content, "IVs",
            std::to_string(mon->ivs.hp) + "/" + std::to_string(mon->ivs.atk) + "/" +
                std::to_string(mon->ivs.def) + "/" + std::to_string(mon->ivs.spa) + "/" +
                std::to_string(mon->ivs.spd) + "/" + std::to_string(mon->ivs.spe),
            nullptr);

        addClickableDetail(list.content, "Set IVs 31", "Max all",
                           [mon](brls::View*) {
                               mon->ivs = Stats{31, 31, 31, 31, 31, 31};
                               brls::Application::notify("IVs set to 31");
                               return true;
                           });

        addClickableDetail(
            list.content, "EVs",
            std::to_string(mon->evs.hp) + "/" + std::to_string(mon->evs.atk) + "/" +
                std::to_string(mon->evs.def) + "/" + std::to_string(mon->evs.spa) + "/" +
                std::to_string(mon->evs.spd) + "/" + std::to_string(mon->evs.spe),
            nullptr);

        addClickableDetail(list.content, "Clear EVs", "Reset to 0",
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
            list.content->addView(makeSectionHeader("Safety", gate.message));
        }
    }

    return makeAppletFrame("Editor", list.scroll);
}

#endif

}  // namespace pkhub::ui
