#include "pkhub/core/hub/HubStorage.hpp"
#include "pkhub/core/pokemon/Pokemon.hpp"
#include "pkhub/services/TransferService.hpp"

#include <cassert>
#include <cstdio>

int main() {
    pkhub::HubStorage hub;
    auto st = hub.openOrCreate();
    assert(st.result == pkhub::SaveOpenResult::Ok);
    assert(hub.boxCount() >= 1);

    pkhub::Pokemon mon;
    mon.species = 25;
    mon.level = 50;
    mon.isShiny = true;
    hub.box(0).slot(0) = mon;

    pkhub::HubStorage hub2;
    hub2.openOrCreate();

    pkhub::TransferService xfer;
    pkhub::SlotRef src{&hub, false, 0, 0};
    pkhub::SlotRef dst{&hub2, false, 0, 1};
    auto r = xfer.transfer(src, dst, pkhub::TransferMode::Move);
    assert(r.ok);
    assert(hub.box(0).slot(0).empty());
    assert(hub2.box(0).slot(1).species == 25);

    std::puts("pkhub_smoke OK");
    return 0;
}
