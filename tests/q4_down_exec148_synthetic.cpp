#include "../src/q4_down_exec148_materializer.h"
#include <array>
#include <cstring>
#include <iostream>
#include <vector>
using namespace arcllm_exec148;
static void sm(uint8_t*b,const std::array<uint8_t,8>&s,const std::array<uint8_t,8>&m){for(unsigned j=0;j<4;++j){b[4+j]=uint8_t((b[4+j]&0xc0u)|(s[j]&63u));b[8+j]=uint8_t((b[8+j]&0xc0u)|(m[j]&63u));}for(unsigned j=4;j<8;++j){b[j]=uint8_t((b[j]&63u)|((s[j]>>4u)<<6u));b[4+j]=uint8_t((b[4+j]&63u)|((m[j]>>4u)<<6u));b[8+j]=uint8_t(((m[j]&15u)<<4u)|(s[j]&15u));}}
static void sq(uint8_t*b,unsigned k,uint8_t q){unsigned g=k/64,w=k%64,o=16+g*32+(w%32);if(w<32)b[o]=uint8_t((b[o]&0xf0u)|(q&15u));else b[o]=uint8_t((b[o]&15u)|((q&15u)<<4u));}
static bool one(int id,uint8_t mode,bool edge){std::array<uint8_t,144>a{};std::array<uint8_t,148>e{};a[0]=uint8_t(0x11+id);a[1]=uint8_t(0xa0+id);a[2]=uint8_t(0x22+id);a[3]=uint8_t(0xb0+id);std::array<uint8_t,8>s{},m{};for(unsigned j=0;j<8;++j){s[j]=edge?uint8_t((j&1)?63:0):uint8_t((j*9+id)%64);m[j]=edge?uint8_t((j&1)?0:63):uint8_t((j*7+3+id)%64);}sm(a.data(),s,m);for(unsigned k=0;k<256;++k)sq(a.data(),k,mode==0?0:mode==1?15:mode==2?uint8_t((k&1)?15:0):uint8_t((k*13+id)%16));materialize_block(a.data(),e.data());std::array<uint8_t,276>x{},y{};validator_source_tuple(a.data(),x);validator_exec_tuple(e.data(),y);return x==y&&std::memcmp(a.data(),e.data(),4)==0;}
int main(){for(int i=0;i<8;++i)if(!one(i,uint8_t(i%4),(i&1)!=0))return 2;std::vector<uint8_t>s(kSourceRowBytes*2u),e(kExecRowBytes*2u);for(size_t i=0;i<s.size();++i)s[i]=uint8_t((i*37u+11u)&255u);materialize_tensor(s.data(),e.data(),2u);auto v=validate_tensor(s.data(),e.data(),2u);if(!v.exact||v.blocks_checked!=148u)return 3;Sha256 h;const char*abc="abc";h.update(reinterpret_cast<const uint8_t*>(abc),3);if(Sha256::hex(h.final())!="ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad")return 4;std::cout<<"EXEC148_SYNTHETIC=PASS blocks="<<v.blocks_checked<<" sha="<<v.exec_sha256<<"\n";return 0;}
