#pragma once

#include <cstddef>
#include <string>

#include "pkhub/core/save/ISaveBackend.hpp"

#if defined(PKHUB_HAS_BOREALIS)
#include <borealis.hpp>
#endif

namespace pkhub::ui {

#if defined(PKHUB_HAS_BOREALIS)

class MainActivity : public brls::Activity {
public:
    CONTENT_FROM_XML_RES("activity/main.xml");
};

/// Build a workspace view (boxes + editor entry). Push with `new brls::Activity(view)`.
brls::View* buildSaveWorkspace(IBoxProvider* provider, const std::string& title);

/// Build a simple core editor for one slot.
brls::View* buildEditor(IBoxProvider* provider,
                        bool fromParty,
                        std::size_t boxIndex,
                        std::size_t slotIndex);

#else

inline void* buildSaveWorkspace(IBoxProvider*, const std::string&) { return nullptr; }
inline void* buildEditor(IBoxProvider*, bool, std::size_t, std::size_t) { return nullptr; }

#endif

}  // namespace pkhub::ui
