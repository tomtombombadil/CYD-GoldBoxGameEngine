// opscan <game number> <op hex> <ECL file ...>: every instruction with that
// opcode in every block of the given ECL DAX files, decoded with the game's
// own OpSet (profile::first(game)->ecl_ops) by walking each block from its
// start - a linear walk, so text read as code may add a stray hit (check its
// neighbours with the playsim's DISASM). Prints block, offset and operands
// (code:value). DAX files only (the Dark Queen's ECL.GLB: hlib_unpack first).
// Build: tools/devkit/build.sh opscan "" tools/devkit/src/opscan.cpp
// (the build script links the engine and the host platform layer).
#include "engine/dax.h"
#include "engine/ecl.h"
#include "engine/games.h"
#include "engine/profile.h"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <vector>

uint32_t g_now = 0;     // (the host platform layer's clock - unused here)

struct FileSrc : dax::ByteSource {
    std::vector<uint8_t> d;
    size_t read_at(uint32_t pos, uint8_t* out, size_t n) override
    {
        if (pos >= d.size()) return 0;
        if (pos + n > d.size()) n = d.size() - pos;
        memcpy(out, d.data() + pos, n);
        return n;
    }
    uint32_t size() const override { return static_cast<uint32_t>(d.size()); }
};

int main(int argc, char** argv)
{
    if (argc < 4) {
        fprintf(stderr, "opscan <game number 1-9> <op hex> <ECL*.DAX ...>\n");
        return 1;
    }
    const auto game = static_cast<games::Game>(atoi(argv[1]));
    const int want = static_cast<int>(strtol(argv[2], nullptr, 16));
    const profile::Profile* p = profile::first(game);
    if (!p || !p->ecl_ops) {
        fprintf(stderr, "no OpSet for game %s\n", argv[1]);
        return 1;
    }
    const ecl::OpSet& set = *p->ecl_ops;
    for (int a = 3; a < argc; ++a) {
        FILE* f = fopen(argv[a], "rb");
        if (!f) continue;
        FileSrc s;
        int c;
        while ((c = fgetc(f)) != EOF) s.d.push_back(static_cast<uint8_t>(c));
        fclose(f);
        dax::Index idx;
        if (dax::read_index(s, idx) != dax::Status::Ok) {
            printf("%s: no index\n", argv[a]);
            continue;
        }
        for (int i = 0; i < idx.count; ++i) {
            std::vector<uint8_t> buf(idx.entries[i].raw_size + 16);
            const uint32_t n = dax::load_block(s, idx, idx.entries[i], buf.data());
            if (n <= 2) continue;
            const uint8_t* code = set.no_prefix ? buf.data() : buf.data() + 2;
            const uint32_t len = set.no_prefix ? n : n - 2;
            for (uint32_t at = 0; at < len;) {
                ecl::Insn in;
                if (!ecl::decode(code, len, at, set, in) || in.next <= at) {
                    ++at;
                    continue;
                }
                if (in.op == want) {
                    printf("%s #%d %04X op %02X:", argv[a], idx.entries[i].id, 0x8000 + in.at, in.op);
                    for (int k = 0; k < in.count && k < ecl::kMaxOps; ++k)
                        printf(" %X:%04X", in.ops[k].code, in.ops[k].word());
                    printf("\n");
                }
                at = in.next;
            }
        }
    }
    return 0;
}
