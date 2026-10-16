#include "retroshelf/engine_c_api.h"

#include <cassert>
#include <cstring>

int main() {
    assert(retroshelf_engine_abi_version() == 1U);
    const char *message = retroshelf_engine_status_message();
    assert(message != nullptr);
    assert(std::strstr(message, "not implemented") != nullptr);
    assert(retroshelf_engine_can_boot_games() == 0);
    return 0;
}
