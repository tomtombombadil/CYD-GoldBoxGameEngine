// Unpack an EXEPACK program's image (scratch tool): unpack <exe> <out>
#include "engine/exepack.h"
#include "engine/dax.h"
#include <cstdio>
#include <vector>
int main(int, char** a)
{
    FILE* f = fopen(a[1], "rb");
    std::vector<uint8_t> v;
    int c;
    while ((c = fgetc(f)) != EOF) v.push_back(static_cast<uint8_t>(c));
    fclose(f);
    dax::MemorySource m(v.data(), static_cast<uint32_t>(v.size()));
    exepack::Info in;
    auto s = exepack::parse(m, in);
    printf("parse %s image %u plain %d cs %04X ip %04X load_start %X\n", exepack::status_text(s), in.image_size, in.plain, in.cs, in.ip, in.load_start);
    std::vector<uint8_t> o(in.image_size);
    s = exepack::read(m, in, 0, o.data(), o.size());
    printf("read %s\n", exepack::status_text(s));
    FILE* w = fopen(a[2], "wb");
    fwrite(o.data(), 1, o.size(), w);
    fclose(w);
}
