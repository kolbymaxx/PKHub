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
    auto* saveBtn = new brls::Button();
    saveBtn->setText("Save");
    toolbar->addView(prev);
    toolbar->addView(next);
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
        list->addView(new brls::ListItem("Empty slot", "Creation tools come next"));
    } else {
        list->addView(new brls::ListItem("Species", std::to_string(mon->species)));
        auto* shiny = new brls::ListItem("Shiny");
        shiny->setChecked(mon->isShiny);
        shiny->registerClickAction([mon, shiny](brls::View*) {
            mon->isShiny = !mon->isShiny;
            shiny->setChecked(mon->isShiny);
            return true;
        });
        list->addView(shiny);
        list->addView(new brls::ListItem("Level", std::to_string(mon->level)));
        list->addView(new brls::ListItem(
            "IVs",
            std::to_string(mon->ivs.hp) + "/" + std::to_string(mon->ivs.atk) + "/" +
                std::to_string(mon->ivs.def) + "/" + std::to_string(mon->ivs.spa) + "/" +
                std::to_string(mon->ivs.spd) + "/" + std::to_string(mon->ivs.spe)));

        auto* lvlUp = new brls::ListItem("Level +1");
        lvlUp->registerClickAction([mon](brls::View*) {
            if (mon->level < 100) {
                ++mon->level;
            }
            brls::Application::notify("Level " + std::to_string(mon->level));
            return true;
        });
        list->addView(lvlUp);

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
