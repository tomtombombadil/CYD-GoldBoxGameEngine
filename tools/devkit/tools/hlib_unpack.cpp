// hlib_unpack.cpp <file.GLB|TLB> <out dir>: every entry of an HLIB library, unpacked, as
// <out>/eNNN.bin (2026-11-15: the Dark Queen's scripts searched for journal mentions)
// g++ -std=c++17 -O1 -Isrc -o hlib_unpack tools/devkit/tools/hlib_unpack.cpp src/engine/*.cpp
#include "engine/hlib.h"
#include <cstdio>
#include <vector>
#include <cstring>
int main(int c,char**v){ FILE*f=fopen(v[1],"rb"); std::vector<uint8_t> d; fseek(f,0,2); d.resize(ftell(f)); fseek(f,0,0); fread(d.data(),1,d.size(),f);
 dax::MemorySource src(d.data(),d.size()); hlib::Library lib; if(hlib::open(src,0,d.size(),lib)!=hlib::Status::Ok) return 1;
 for(int i=lib.first_item();i<lib.count;++i){ uint32_t pos,len; if(!hlib::entry(src,lib,i,pos,len)) continue; hlib::EntryReader r(src,pos,len); std::vector<uint8_t> o; int b; while((b=r.next())>=0) o.push_back(b);
  char nm[64]; snprintf(nm,64,"%s/e%03d.bin",v[2],hlib::id_of(src,lib,i)); FILE*w=fopen(nm,"wb"); fwrite(o.data(),1,o.size(),w); fclose(w);} }
