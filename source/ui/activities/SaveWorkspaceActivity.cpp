#include "pkhub/ui/activities/MainActivity.hpp"

#include "pkhub/app/AppContext.hpp"
#include "pkhub/core/safety/LegalityService.hpp"
#include "pkhub/core/safety/SafetyPolicy.hpp"
#include "pkhub/ui/views/BoxGridView.hpp"

namespace pkhub::ui {

#if defined(PKHUB_HAS_BOREALIS)

brls::View* buildSaveWorkspace(IBoxProvider* provider, const std::string& title) {
    auto* root = new brls::AppletFrame();
    root->setTitle(title.empty() ? "Save Workspace" : title);

    auto* body = new brls::Box(brls::Axis::COLUMN);
    body->setPadding(16);
    body->setGrow(1.f);

    auto* toolbar = new brls::Box(brls::Axis::ROW);
    auto* prev = new brls::Button();
    prev->setText("Prev Box");
    auto* next = new brls::Button();
    next->setText("Next Box");
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
        auto* list = new brls::List();
        for (std::size_t i = 0; i < provider->party().size(); ++i) {
            const auto& mon = provider->party().slot(i);
            auto* item = new brls::ListItem(
                "Slot " + std::to_string(i + 1),
                mon.empty() ? "Empty"
                            : ("Species " + std::to_string(mon.species) + " Lv" +
                               std::to_string(mon.level)));
            item->registerClickAction([provider, i](brls::View*) {
                brls::Application::pushActivity(
                    new brls::Activity(buildEditor(provider, true, 0, i)));
                return true;
            });
            list->addView(item);
        }
        auto* frame = new brls::AppletFrame();
        frame->setTitle("Party");
        frame->setContentView(list);
        brls::Application::pushActivity(new brls::Activity(frame));
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

    root->setContentView(body);
    return root;
}

brls::View* buildEditor(IBoxProvider* provider,
                        bool fromParty,
                        std::size_t boxIndex,
                        std::size_t slotIndex) {
    auto* root = new brls::AppletFrame();
    root->setTitle("Editor");

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

    auto* list = new brls::List();
    if (!mon || mon->empty()) {
        list->addView(new brls::ListItem("Empty slot", "Create tools / import coming next"));
    } else {
        list->addView(new brls::ListItem(
            "Species",
            std::to_string(mon->species) +
                (mon->nativeGeneration == Generation::Gen9 ? " (SV internal id)" : "")));

        auto* shiny = new brls::ListItem("Shiny");
        shiny->setChecked(mon->isShiny);
        shiny->registerClickAction([mon, shiny](brls::View*) {
            mon->isShiny = !mon->isShiny;
            shiny->setChecked(mon->isShiny);
            return true;
        });
        list->addView(shiny);

        auto* level = new brls::ListItem("Level", std::to_string(mon->level));
        level->registerClickAction([mon, level](brls::View*) {
            mon->level = uint8_t(mon->level >= 100 ? 1 : mon->level + 1);
            level->setDescription(std::to_string(mon->level));
            return true;
        });
        list->addView(level);

        auto* nature = new brls::ListItem("Nature", std::to_string(mon->nature));
        nature->registerClickAction([mon, nature](brls::View*) {
            const int n = ((mon->nature < 0 ? 0 : int(mon->nature)) + 1) % 25;
            mon->nature = int8_t(n);
            nature->setDescription(std::to_string(mon->nature));
            return true;
        });
        list->addView(nature);

        auto* ability = new brls::ListItem("Ability ID", std::to_string(mon->abilityId));
        ability->registerClickAction([mon, ability](brls::View*) {
            mon->abilityId = uint16_t(mon->abilityId + 1);
            ability->setDescription(std::to_string(mon->abilityId));
            return true;
        });
        list->addView(ability);

        auto* abilitySlot = new brls::ListItem("Ability slot", std::to_string(mon->abilitySlot));
        abilitySlot->registerClickAction([mon, abilitySlot](brls::View*) {
            const int s = ((mon->abilitySlot < 0 ? 0 : int(mon->abilitySlot)) + 1) % 4;
            mon->abilitySlot = int8_t(s);
            abilitySlot->setDescription(std::to_string(mon->abilitySlot));
            return true;
        });
        list->addView(abilitySlot);

        list->addView(new brls::ListItem(
            "IVs HP/Atk/Def/SpA/SpD/Spe",
            std::to_string(mon->ivs.hp) + "/" + std::to_string(mon->ivs.atk) + "/" +
                std::to_string(mon->ivs.def) + "/" + std::to_string(mon->ivs.spa) + "/" +
                std::to_string(mon->ivs.spd) + "/" + std::to_string(mon->ivs.spe)));

        auto* maxIvs = new brls::ListItem("Set IVs 31");
        maxIvs->registerClickAction([mon](brls::View*) {
            mon->ivs = Stats{31, 31, 31, 31, 31, 31};
            brls::Application::notify("IVs set to 31");
            return true;
        });
        list->addView(maxIvs);

        list->addView(new brls::ListItem(
            "EVs HP/Atk/Def/SpA/SpD/Spe",
            std::to_string(mon->evs.hp) + "/" + std::to_string(mon->evs.atk) + "/" +
                std::to_string(mon->evs.def) + "/" + std::to_string(mon->evs.spa) + "/" +
                std::to_string(mon->evs.spd) + "/" + std::to_string(mon->evs.spe)));

        auto* clearEvs = new brls::ListItem("Clear EVs");
        clearEvs->registerClickAction([mon](brls::View*) {
            mon->evs = Stats{};
            brls::Application::notify("EVs cleared");
            return true;
        });
        list->addView(clearEvs);

        for (int i = 0; i < 4; ++i) {
            auto* mv = new brls::ListItem("Move " + std::to_string(i + 1),
                                         std::to_string(mon->moves[std::size_t(i)]));
            mv->registerClickAction([mon, i, mv](brls::View*) {
                mon->moves[std::size_t(i)] = uint16_t(mon->moves[std::size_t(i)] + 1);
                mv->setDescription(std::to_string(mon->moves[std::size_t(i)]));
                return true;
            });
            list->addView(mv);
        }

        if (mon->nativeGeneration == Generation::Gen9 || mon->teraType != PokemonType::None) {
            auto* tera = new brls::ListItem("Tera type", std::to_string(int(mon->teraType)));
            tera->registerClickAction([mon, tera](brls::View*) {
                int t = int(mon->teraType);
                if (t < 0 || t > 18) {
                    t = 0;
                } else {
                    t = (t + 1) % 19;
                }
                mon->teraType = static_cast<PokemonType>(t);
                tera->setDescription(std::to_string(int(mon->teraType)));
                return true;
            });
            list->addView(tera);
        }

        SafetyPolicy policy;
        LegalityService legality;
        const auto report = legality.evaluate(*mon, provider->gameId());
        const auto gate = policy.evaluate(SafetyAction::EditPokemon, &report);
        if (gate.gate != SafetyGate::Allow) {
            list->addView(new brls::ListItem(gate.title, gate.message));
        }
    }

    root->setContentView(list);
    return root;
}

#endif

}  // namespace pkhub::ui
