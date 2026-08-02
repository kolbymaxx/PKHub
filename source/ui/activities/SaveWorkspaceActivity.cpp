#include "pkhub/ui/activities/MainActivity.hpp"

#include "pkhub/app/AppContext.hpp"
#include "pkhub/core/pokemon/NatureNames.hpp"
#include "pkhub/core/pokemon/SpeciesIds.hpp"
#include "pkhub/core/pokemon/SpeciesNames.hpp"
#include "pkhub/core/safety/LegalityService.hpp"
#include "pkhub/core/safety/SafetyPolicy.hpp"
#include "pkhub/ui/SpriteService.hpp"
#include "pkhub/ui/ThemeTokens.hpp"
#include "pkhub/ui/TypeColors.hpp"
#include "pkhub/ui/UiList.hpp"
#include "pkhub/ui/views/BoxGridView.hpp"
#include "pkhub/ui/views/PokemonSlotView.hpp"

#if defined(PKHUB_HAS_BOREALIS)
#include <borealis/views/cells/cell_bool.hpp>
#include <borealis/views/cells/cell_detail.hpp>
#endif

namespace pkhub::ui {

#if defined(PKHUB_HAS_BOREALIS)

namespace {

void spaceToolbarButton(brls::View* btn) {
    btn->setMarginRight(8);
}

brls::Box* makePartyStrip(IBoxProvider* provider) {
    auto* strip = new brls::Box(brls::Axis::COLUMN);
    strip->setPadding(10, 8, 8, 8);
    strip->setMarginTop(10);
    strip->setCornerRadius(14);
    strip->setBackgroundColor(theme::partyStrip());
    strip->setBorderThickness(1.0f);
    strip->setBorderColor(theme::accentDim());

    auto* label = new brls::Label();
    label->setFontSize(14);
    label->setTextColor(theme::textMuted());
    label->setText("Party");
    strip->addView(label);

    auto* row = new brls::Box(brls::Axis::ROW);
    row->setPaddingTop(6);
    row->setJustifyContent(brls::JustifyContent::FLEX_START);
    strip->addView(row);

    if (!provider) {
        return strip;
    }
    for (std::size_t i = 0; i < provider->party().size(); ++i) {
        auto* slot = new PokemonSlotView();
        slot->setWidth(78);
        slot->setHeight(92);
        slot->setPokemon(&provider->party().slot(i));
        const std::size_t idx = i;
        slot->registerClickAction([provider, idx](brls::View*) {
            brls::Application::pushActivity(
                new brls::Activity(buildEditor(provider, true, 0, idx)));
            return true;
        });
        row->addView(slot);
    }
    return strip;
}

brls::Box* makeSummaryHero(const Pokemon& mon) {
    auto* hero = new brls::Box(brls::Axis::ROW);
    hero->setAlignItems(brls::AlignItems::CENTER);
    hero->setPadding(14, 10, 18, 10);
    hero->setMarginBottom(10);
    hero->setCornerRadius(16);
    hero->setBackgroundColor(theme::bgWallpaper());
    hero->setBorderThickness(1.2f);
    hero->setBorderColor(mon.isShiny ? nvgRGBA(230, 190, 70, 160) : theme::accentDim());

    auto* spriteFrame = new brls::Box(brls::Axis::COLUMN);
    spriteFrame->setWidth(108);
    spriteFrame->setHeight(108);
    spriteFrame->setCornerRadius(54);
    spriteFrame->setBackgroundColor(nvgRGBA(20, 40, 44, 230));
    spriteFrame->setJustifyContent(brls::JustifyContent::CENTER);
    spriteFrame->setAlignItems(brls::AlignItems::CENTER);

    auto* sprite = new brls::Image();
    sprite->setWidth(92);
    sprite->setHeight(92);
    sprite->setScalingType(brls::ImageScalingType::FIT);
    const auto path = SpriteService::spritePath(mon);
    if (!path.empty()) {
        sprite->setImageFromRes(path);
    }
    spriteFrame->addView(sprite);
    hero->addView(spriteFrame);

    auto* meta = new brls::Box(brls::Axis::COLUMN);
    meta->setPaddingLeft(18);
    meta->setGrow(1.f);

    auto* name = new brls::Label();
    name->setFontSize(28);
    name->setTextColor(theme::text());
    name->setText(pokemonDisplayName(mon) + (mon.isShiny ? "  ✦" : ""));
    meta->addView(name);

    auto* sub = new brls::Label();
    sub->setFontSize(16);
    sub->setTextColor(theme::textMuted());
    {
        const uint16_t nat = nationalDexId(mon);
        std::string line = "Lv " + std::to_string(mon.level) + "  ·  #" + std::to_string(nat);
        const char* speciesName = speciesEnglishName(nat);
        if (speciesName && *speciesName) {
            line += "  ";
            line += speciesName;
        }
        sub->setText(line);
    }
    meta->addView(sub);

    const char* nature = natureEnglishName(mon.nature);
    if (nature && *nature) {
        auto* natLine = new brls::Label();
        natLine->setFontSize(14);
        natLine->setTextColor(theme::textFaint());
        natLine->setText(std::string("Nature  ") + nature);
        meta->addView(natLine);
    }

    if (mon.teraType != PokemonType::None) {
        auto* pill = new brls::Box(brls::Axis::ROW);
        pill->setMarginTop(8);
        pill->setPadding(4, 10, 4, 10);
        pill->setCornerRadius(8);
        pill->setBackgroundColor(pokemonTypeColor(mon.teraType));
        auto* t = new brls::Label();
        t->setFontSize(13);
        t->setTextColor(nvgRGB(20, 24, 22));
        t->setText(std::string("Tera ") + pokemonTypeEnglishName(mon.teraType));
        pill->addView(t);
        meta->addView(pill);
    } else if (mon.nativeGeneration == Generation::Gen9) {
        auto* sv = new brls::Label();
        sv->setFontSize(13);
        sv->setTextColor(theme::textFaint());
        sv->setText("SV internal id " + std::to_string(mon.species));
        meta->addView(sv);
    }

    hero->addView(meta);
    return hero;
}

}  // namespace

brls::View* buildSaveWorkspace(IBoxProvider* provider, const std::string& title) {
    auto* body = new brls::Box(brls::Axis::COLUMN);
    body->setPadding(12, 14, 12, 14);
    body->setGrow(1.f);
    body->setBackgroundColor(theme::bg());

    auto* toolbar = new brls::Box(brls::Axis::ROW);
    toolbar->setPaddingBottom(10);
    toolbar->setAlignItems(brls::AlignItems::CENTER);

    auto* prev = new brls::Button();
    prev->setText("◀");
    auto* next = new brls::Button();
    next->setText("▶");
    auto* saveBtn = new brls::Button();
    saveBtn->setText("Save");
    spaceToolbarButton(prev);
    spaceToolbarButton(next);

    auto* boxLabel = new brls::Label();
    boxLabel->setFontSize(16);
    boxLabel->setTextColor(theme::textMuted());
    boxLabel->setText("PC Boxes");
    boxLabel->setMarginRight(12);
    boxLabel->setGrow(1.f);

    toolbar->addView(prev);
    toolbar->addView(next);
    toolbar->addView(boxLabel);
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
    body->addView(makePartyStrip(provider));

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

    const std::string frameTitle =
        title.empty() ? "PC Boxes" : (title + " · PC");
    return makeAppletFrame(frameTitle, body);
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
        list.content->addView(makeSectionHeader("Empty slot", "Nothing stored here yet"));
        addBodyLabel(list.content, "Create / import tools are coming next.");
    } else {
        list.content->addView(makeSummaryHero(*mon));
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
            auto* tera = makeDetailCell("Tera type", pokemonTypeEnglishName(mon->teraType));
            tera->registerClickAction([mon, tera](brls::View*) {
                int t = int(mon->teraType);
                if (t < 0 || t > 18) {
                    t = 0;
                } else {
                    t = (t + 1) % 19;
                }
                mon->teraType = static_cast<PokemonType>(t);
                tera->setDetailText(pokemonTypeEnglishName(mon->teraType));
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

    return makeAppletFrame("Summary", list.scroll);
}

#endif

}  // namespace pkhub::ui
