// The memory log on a PC: nothing (the PC's heap isn't the board's; the
// sweep's memloop / HEAPLOG and the playsim's heapprof measure the PC side)
#include "platform/memlog.h"

#include <cstdio>
#include <cstdlib>

namespace memlog {

void begin(const char*, const char*) {}
// (MEMLOG=1: the lines to stderr - make_room's drops, "NOT ENOUGH MEMORY")
void note(const char* line)
{
    if (getenv("MEMLOG")) fprintf(stderr, "[memlog] %s\n", line);
}
void tick(bool) {}
// (plenty: the game's make_room drops nothing here unless LOWMEM=kb
// pretends the largest block is that small - to try the recovery)
unsigned free_kb() { return 999; }
unsigned largest_kb()
{
    static const unsigned v = getenv("LOWMEM") ? static_cast<unsigned>(atoi(getenv("LOWMEM"))) : 999;
    return v;
}

} // namespace memlog
