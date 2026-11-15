// jbook.cpp <journal.pdf>: runs journal::make_book on a PDF and prints the counts and
// the Introduction / Journal Entry 1 sections (2026-11-15: the journal weak spots),
// then the journal entry / tale / proclamation numbers missing up to the highest.
// g++ -std=c++17 -O1 -Isrc -o jbook tools/devkit/tools/jbook.cpp src/engine/*.cpp
#include "engine/journal.h"
#include "engine/dax.h"
#include <cstdio>
#include <cstring>
#include <vector>
#include <string>
struct MemOut : journal::Output { std::vector<uint8_t> b; bool write_at(uint32_t pos,const uint8_t*d,size_t n) override{ if(b.size()<pos+n)b.resize(pos+n); memcpy(b.data()+pos,d,n); return true;} };
int main(int c, char**v){ FILE*f=fopen(v[1],"rb"); std::vector<uint8_t> d; int ch; fseek(f,0,2); d.resize(ftell(f)); fseek(f,0,0); fread(d.data(),1,d.size(),f);
 dax::MemorySource src(d.data(),d.size()); MemOut book,sect; static journal::Section s[journal::kMaxSections]; journal::BookInfo info;
 bool ok=journal::make_book(src,book,sect,s,journal::kMaxSections,info,nullptr,nullptr);
 printf("ok %d pages %d sections %d J%d T%d P%d I%d\n",ok,info.pages,info.sections,info.count[0],info.count[1],info.count[2],info.count[3]);
 for(const char* k="JTP";*k;++k){ int hi=0; bool have[1000]={}; for(int i=0;i<info.sections;++i) if(s[i].kind==*k&&s[i].number<1000){have[s[i].number]=true; if(s[i].number>hi)hi=s[i].number;}
   if(!hi) continue; printf("%c 1-%d missing:",*k,hi); for(int n=1;n<=hi;++n) if(!have[n]) printf(" %d",n); printf("\n"); }
 if(c>2 && v[2][0]=='a'){ for(int i=0;i<info.sections;++i) printf("%c%d %u\n",s[i].kind,s[i].number,(unsigned)s[i].len); return 0; }
 if(c>2) return 0;
 for(int i=0;i<info.sections;++i) if(s[i].kind=='I'||(s[i].kind=='J'&&s[i].number==1)) printf("%c%d: %.20000s\n",s[i].kind,s[i].number,std::string((char*)sect.b.data()+s[i].at,s[i].len).c_str());
}
