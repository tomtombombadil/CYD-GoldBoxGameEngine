// ecl_strings.cpp <script blocks...>: every packed string (code 0x80) at any byte, unpacked
// - grep the output for what the scripts say (garbage where a 0x80 is no string)
// g++ -std=c++17 -O1 -Isrc -o ecl_strings tools/devkit/tools/ecl_strings.cpp src/engine/*.cpp
#include "engine/ecl_vm.h"
#include <cstdio>
#include <vector>
int main(int c,char**v){ for(int a=1;a<c;++a){ FILE*f=fopen(v[a],"rb"); std::vector<uint8_t> d; fseek(f,0,2); d.resize(ftell(f)); fseek(f,0,0); fread(d.data(),1,d.size(),f); fclose(f);
 for(size_t i=0;i+2<d.size();++i) if(d[i]==0x80 && d[i+1]>=6 && i+2+d[i+1]<=d.size()){ char o[400]; ecl::unpack_string(&d[i+2],d[i+1],o,sizeof o); printf("%s @%zx: %s\n",v[a],i,o);} } }
