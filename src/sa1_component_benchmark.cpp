#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <vulkan/vulkan.h>

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <limits>
#include <numeric>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

struct Cell {
    const char* id;
    uint32_t n;
    uint32_t rows;
    uint32_t add_bias;
    uint32_t row_bytes;
    bool q6;
};

static const std::array<Cell,5> kQ4Cells = {{
    {"Q4_H3584_R3584_BIAS",3584,3584,1,2016,false},
    {"Q4_H3584_R3584_NOBIAS",3584,3584,0,2016,false},
    {"Q4_H3584_R512_BIAS",3584,512,1,2016,false},
    {"Q4_H3584_R18944_NOBIAS",3584,18944,0,2016,false},
    {"Q4_H18944_R3584_NOBIAS",18944,3584,0,10656,false}
}};
static const std::array<Cell,2> kQ6Cells = {{
    {"Q6_H3584_R512_BIAS",3584,512,1,2940,true},
    {"Q6_H18944_R3584_NOBIAS",18944,3584,0,15540,true}
}};
static const std::array<uint64_t,4> kSeeds = {{
    0x5341315046495831ull,0x5341315046495832ull,
    0x5341315046495833ull,0x5341315046495834ull
}};

static std::string esc(const std::string& s) {
    std::ostringstream o;
    for (unsigned char c : s) {
        if (c=='"' || c=='\\') { o << '\\' << char(c); }
        else if (c=='\n') o << "\\n";
        else if (c=='\r') o << "\\r";
        else if (c=='\t') o << "\\t";
        else if (c < 32) o << "?";
        else o << char(c);
    }
    return o.str();
}
static std::vector<uint8_t> read_bytes(const std::string& p) {
    std::ifstream f(p,std::ios::binary);
    if(!f) throw std::runtime_error("cannot open "+p);
    return std::vector<uint8_t>((std::istreambuf_iterator<char>(f)),{});
}
static uint64_t fnv1a64(const void* data,size_t n) {
    const auto* p=reinterpret_cast<const uint8_t*>(data);
    uint64_t h=1469598103934665603ull;
    for(size_t i=0;i<n;++i){h^=p[i];h*=1099511628211ull;}
    return h;
}
static uint64_t fnv_text(const std::string& s){return fnv1a64(s.data(),s.size());}
static std::string hex64(uint64_t v){std::ostringstream o;o<<std::hex<<std::setw(16)<<std::setfill('0')<<v;return o.str();}
static uint64_t splitmix64(uint64_t& x) {
    uint64_t z=(x+=0x9E3779B97F4A7C15ull);
    z=(z^(z>>30))*0xBF58476D1CE4E5B9ull;
    z=(z^(z>>27))*0x94D049BB133111EBull;
    return z^(z>>31);
}
static float rand_s16(uint64_t& s) {
    int16_t v=static_cast<int16_t>(static_cast<uint16_t>(splitmix64(s)&0xffffu));
    return float(v)/32768.0f;
}
static void put_u16(std::vector<uint8_t>& b,size_t off,uint16_t v){
    b.at(off)=uint8_t(v&255u);b.at(off+1)=uint8_t(v>>8);
}
static float half_to_float(uint16_t h) {
    uint32_t sign=(h>>15)&1u, exp=(h>>10)&31u, frac=h&1023u;
    if(exp==0){
        if(frac==0) return sign?-0.0f:0.0f;
        float v=std::ldexp(float(frac),-24);
        return sign?-v:v;
    }
    if(exp==31) return frac?std::numeric_limits<float>::quiet_NaN():(sign?-INFINITY:INFINITY);
    float v=std::ldexp(1.0f+float(frac)/1024.0f,int(exp)-15);
    return sign?-v:v;
}
static uint8_t u8(const std::vector<uint8_t>& w,size_t off){return w.at(off);}
static uint16_t u16(const std::vector<uint8_t>& w,size_t off){return uint16_t(u8(w,off))|(uint16_t(u8(w,off+1))<<8);}
static void scale_min_cpu(const std::vector<uint8_t>& w,size_t base,uint32_t j,uint32_t& sc,uint32_t& mn){
    if(j<4u){sc=u8(w,base+4+j)&63u;mn=u8(w,base+8+j)&63u;}
    else{
        uint32_t qj=u8(w,base+4+j+4);
        sc=(qj&15u)|((uint32_t(u8(w,base+4+j-4))>>6u)<<4u);
        mn=(qj>>4u)|((uint32_t(u8(w,base+4+j))>>6u)<<4u);
    }
}
static float q4_weight(const std::vector<uint8_t>& w,const Cell& c,uint32_t row,uint32_t k){
    size_t rb=size_t(row)*c.row_bytes;
    uint32_t ib=k>>8u, kin=k&255u;
    size_t base=rb+size_t(ib)*144u;
    float d=half_to_float(u16(w,base));
    float dm=half_to_float(u16(w,base+2));
    uint32_t sc=0,mn=0;scale_min_cpu(w,base,kin>>5u,sc,mn);
    uint32_t local64=kin&63u;
    uint8_t qb=u8(w,base+16u+(kin>>6u)*32u+(local64&31u));
    uint32_t q=local64<32u?(qb&15u):(qb>>4u);
    return d*float(sc)*float(q)-dm*float(mn);
}
static int i8_cpu(uint8_t v){return v>=128u?int(v)-256:int(v);}
static int q6_value_cpu(const std::vector<uint8_t>& w,size_t base,uint32_t k,int& scale){
    uint32_t half128=k>>7u,kk=k&127u,l=kk&31u,quarter=kk>>5u;
    size_t ql_base=base+size_t(half128)*64u;
    size_t qh_base=base+128u+size_t(half128)*32u;
    size_t sc_base=base+192u+size_t(half128)*8u;
    uint32_t is=l>>4u,low=0,high=0,sc_idx=0;
    if(quarter==0u){low=u8(w,ql_base+l)&15u;high=(u8(w,qh_base+l)>>0u)&3u;sc_idx=is+0u;}
    else if(quarter==1u){low=u8(w,ql_base+l+32u)&15u;high=(u8(w,qh_base+l)>>2u)&3u;sc_idx=is+2u;}
    else if(quarter==2u){low=u8(w,ql_base+l)>>4u;high=(u8(w,qh_base+l)>>4u)&3u;sc_idx=is+4u;}
    else{low=u8(w,ql_base+l+32u)>>4u;high=(u8(w,qh_base+l)>>6u)&3u;sc_idx=is+6u;}
    scale=i8_cpu(u8(w,sc_base+sc_idx));
    return int(low|(high<<4u))-32;
}
static float q6_weight(const std::vector<uint8_t>& w,const Cell& c,uint32_t row,uint32_t k){
    size_t rb=size_t(row)*c.row_bytes;
    uint32_t ib=k>>8u,kin=k&255u;
    size_t base=rb+size_t(ib)*210u;
    int sc=0,q=q6_value_cpu(w,base,kin,sc);
    float d=half_to_float(u16(w,base+208u));
    return d*float(sc)*float(q);
}
struct Fixture {
    std::vector<uint8_t> w;
    std::vector<float> x,bias;
};
static Fixture make_fixture(const Cell& c,uint32_t bank){
    uint64_t s=kSeeds.at(bank)^fnv_text(c.id);
    Fixture f;
    f.w.resize(size_t(c.row_bytes)*c.rows);
    for(uint32_t r=0;r<c.rows;++r){
        for(uint32_t ib=0;ib<c.n/256u;++ib){
            if(c.q6){
                size_t base=size_t(r)*c.row_bytes+size_t(ib)*210u;
                for(size_t i=0;i<208;++i)f.w[base+i]=uint8_t(splitmix64(s)&255u);
                put_u16(f.w,base+208u,0x2800u);
            }else{
                size_t base=size_t(r)*c.row_bytes+size_t(ib)*144u;
                put_u16(f.w,base,0x2800u);
                put_u16(f.w,base+2,0x2400u);
                for(size_t i=4;i<144;++i)f.w[base+i]=uint8_t(splitmix64(s)&255u);
            }
        }
    }
    f.x.resize(c.n);for(float& v:f.x)v=rand_s16(s);
    f.bias.resize(c.rows);for(float& v:f.bias)v=rand_s16(s);
    return f;
}
static std::vector<float> cpu_ref(const Cell& c,const Fixture& f){
    std::vector<float> y(c.rows);
    for(uint32_t r=0;r<c.rows;++r){
        float sum=c.add_bias?f.bias[r]:0.0f;
        for(uint32_t k=0;k<c.n;++k)sum+=(c.q6?q6_weight(f.w,c,r,k):q4_weight(f.w,c,r,k))*f.x[k];
        y[r]=sum;
    }
    return y;
}
struct Metric{double max_abs=0,rmse=0;bool finite=true;};
static Metric metric(const std::vector<float>& a,const std::vector<float>& b){
    if(a.size()!=b.size())throw std::runtime_error("metric size mismatch");
    Metric m; long double ss=0;
    for(size_t i=0;i<a.size();++i){
        if(!std::isfinite(a[i])||!std::isfinite(b[i]))m.finite=false;
        double d=std::abs(double(a[i])-double(b[i]));
        m.max_abs=(std::max)(m.max_abs,d);ss+=static_cast<long double>(d)*static_cast<long double>(d);
    }
    m.rmse=std::sqrt(static_cast<double>(ss/static_cast<long double>(a.size())));return m;
}

struct Buffer{
    VkBuffer b=VK_NULL_HANDLE;VkDeviceMemory m=VK_NULL_HANDLE;VkDeviceSize bytes=0;void* map=nullptr;
};
struct Pipe{
    VkShaderModule mod=VK_NULL_HANDLE;VkDescriptorSetLayout dsl=VK_NULL_HANDLE;
    VkPipelineLayout layout=VK_NULL_HANDLE;VkPipeline pipe=VK_NULL_HANDLE;
};
struct BindSet{VkDescriptorPool pool=VK_NULL_HANDLE;VkDescriptorSet set=VK_NULL_HANDLE;};

class VkCtx{
public:
    VkInstance inst=VK_NULL_HANDLE;VkPhysicalDevice phys=VK_NULL_HANDLE;VkDevice dev=VK_NULL_HANDLE;
    VkQueue q=VK_NULL_HANDLE;uint32_t qfam=0;VkCommandPool cp=VK_NULL_HANDLE;
    VkPhysicalDeviceProperties props{};VkPhysicalDeviceMemoryProperties mem{};
    VkPhysicalDeviceSubgroupProperties subgroup{VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SUBGROUP_PROPERTIES};
    VkPhysicalDeviceSubgroupSizeControlProperties sgp{VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SUBGROUP_SIZE_CONTROL_PROPERTIES};
    VkPhysicalDeviceSubgroupSizeControlFeatures sgf{VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SUBGROUP_SIZE_CONTROL_FEATURES};

    void init(){
        VkApplicationInfo app{VK_STRUCTURE_TYPE_APPLICATION_INFO};app.pApplicationName="ArcLLM-SA1";app.apiVersion=VK_API_VERSION_1_3;
        VkInstanceCreateInfo ici{VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO};ici.pApplicationInfo=&app;
        if(vkCreateInstance(&ici,nullptr,&inst)!=VK_SUCCESS)throw std::runtime_error("vkCreateInstance failed");
        uint32_t n=0;if(vkEnumeratePhysicalDevices(inst,&n,nullptr)!=VK_SUCCESS||!n)throw std::runtime_error("no Vulkan device");
        std::vector<VkPhysicalDevice>d(n);vkEnumeratePhysicalDevices(inst,&n,d.data());
        for(auto x:d){VkPhysicalDeviceProperties p{};vkGetPhysicalDeviceProperties(x,&p);std::string nm=p.deviceName;
            if(nm.find("Arc")!=std::string::npos&&nm.find("140V")!=std::string::npos){phys=x;props=p;break;}}
        if(!phys)throw std::runtime_error("Arc 140V Vulkan device not found");
        sgp.pNext=nullptr;subgroup.pNext=&sgp;VkPhysicalDeviceProperties2 p2{VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2};p2.pNext=&subgroup;
        vkGetPhysicalDeviceProperties2(phys,&p2);props=p2.properties;
        VkPhysicalDeviceFeatures2 f2{VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2};f2.pNext=&sgf;vkGetPhysicalDeviceFeatures2(phys,&f2);
        if(!(subgroup.supportedStages&VK_SHADER_STAGE_COMPUTE_BIT))throw std::runtime_error("compute subgroup unsupported");
        if(!(subgroup.supportedOperations&VK_SUBGROUP_FEATURE_ARITHMETIC_BIT))throw std::runtime_error("subgroup arithmetic unsupported");
        if(!sgf.subgroupSizeControl||!sgf.computeFullSubgroups)throw std::runtime_error("subgroup size-control/full-subgroups unsupported");
        if(sgp.minSubgroupSize>32u||sgp.maxSubgroupSize<32u)throw std::runtime_error("required subgroup size 32 unsupported");
        if(!(sgp.requiredSubgroupSizeStages&VK_SHADER_STAGE_COMPUTE_BIT))throw std::runtime_error("required subgroup size unsupported in compute");
        if(sgp.maxComputeWorkgroupSubgroups<4u)throw std::runtime_error("fewer than 4 compute workgroup subgroups");
        uint32_t qc=0;vkGetPhysicalDeviceQueueFamilyProperties(phys,&qc,nullptr);std::vector<VkQueueFamilyProperties> qp(qc);vkGetPhysicalDeviceQueueFamilyProperties(phys,&qc,qp.data());
        bool found=false;for(uint32_t i=0;i<qc;++i)if((qp[i].queueFlags&VK_QUEUE_COMPUTE_BIT)&&qp[i].queueCount){qfam=i;found=true;break;}
        if(!found)throw std::runtime_error("no compute queue");
        if(qp[qfam].timestampValidBits!=64u)throw std::runtime_error("timestampValidBits != 64");
        vkGetPhysicalDeviceMemoryProperties(phys,&mem);
        float pri=1.0f;VkDeviceQueueCreateInfo qci{VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO};qci.queueFamilyIndex=qfam;qci.queueCount=1;qci.pQueuePriorities=&pri;
        VkPhysicalDeviceSubgroupSizeControlFeatures enable{VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SUBGROUP_SIZE_CONTROL_FEATURES};enable.subgroupSizeControl=VK_TRUE;enable.computeFullSubgroups=VK_TRUE;
        VkDeviceCreateInfo dci{VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO};dci.pNext=&enable;dci.queueCreateInfoCount=1;dci.pQueueCreateInfos=&qci;
        if(vkCreateDevice(phys,&dci,nullptr,&dev)!=VK_SUCCESS)throw std::runtime_error("vkCreateDevice failed");
        vkGetDeviceQueue(dev,qfam,0,&q);
        VkCommandPoolCreateInfo pci{VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO};pci.flags=VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;pci.queueFamilyIndex=qfam;
        if(vkCreateCommandPool(dev,&pci,nullptr,&cp)!=VK_SUCCESS)throw std::runtime_error("vkCreateCommandPool failed");
    }
    uint32_t mem_type(uint32_t bits,VkMemoryPropertyFlags need){
        uint32_t fallback=UINT32_MAX;
        for(uint32_t i=0;i<mem.memoryTypeCount;++i)if(bits&(1u<<i)){
            auto f=mem.memoryTypes[i].propertyFlags;
            if((f&need)==need&&(f&VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT))return i;
            if((f&need)==need&&fallback==UINT32_MAX)fallback=i;
        }
        if(fallback==UINT32_MAX)throw std::runtime_error("host coherent memory type missing");return fallback;
    }
    Buffer buffer(VkDeviceSize bytes,const void* init=nullptr){
        Buffer x;x.bytes=bytes;VkBufferCreateInfo bi{VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO};bi.size=bytes;bi.usage=VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;bi.sharingMode=VK_SHARING_MODE_EXCLUSIVE;
        if(vkCreateBuffer(dev,&bi,nullptr,&x.b)!=VK_SUCCESS)throw std::runtime_error("vkCreateBuffer failed");
        VkMemoryRequirements mr{};vkGetBufferMemoryRequirements(dev,x.b,&mr);
        VkMemoryAllocateInfo ai{VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO};ai.allocationSize=mr.size;ai.memoryTypeIndex=mem_type(mr.memoryTypeBits,VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT|VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
        if(vkAllocateMemory(dev,&ai,nullptr,&x.m)!=VK_SUCCESS)throw std::runtime_error("vkAllocateMemory failed");
        if(vkBindBufferMemory(dev,x.b,x.m,0)!=VK_SUCCESS)throw std::runtime_error("vkBindBufferMemory failed");
        if(vkMapMemory(dev,x.m,0,bytes,0,&x.map)!=VK_SUCCESS)throw std::runtime_error("vkMapMemory failed");
        if(init)std::memcpy(x.map,init,size_t(bytes));else std::memset(x.map,0,size_t(bytes));return x;
    }
    void destroy(Buffer& x){if(x.map)vkUnmapMemory(dev,x.m);if(x.b)vkDestroyBuffer(dev,x.b,nullptr);if(x.m)vkFreeMemory(dev,x.m,nullptr);x={};}
    Pipe pipeline(const std::string& spv,bool candidate){
        auto code=read_bytes(spv);if(code.empty()||code.size()%4)throw std::runtime_error("invalid SPIR-V "+spv);
        Pipe p;VkShaderModuleCreateInfo sm{VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO};sm.codeSize=code.size();sm.pCode=reinterpret_cast<const uint32_t*>(code.data());
        if(vkCreateShaderModule(dev,&sm,nullptr,&p.mod)!=VK_SUCCESS)throw std::runtime_error("vkCreateShaderModule failed");
        std::array<VkDescriptorSetLayoutBinding,4> bs{};for(uint32_t i=0;i<4;++i){bs[i].binding=i;bs[i].descriptorType=VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;bs[i].descriptorCount=1;bs[i].stageFlags=VK_SHADER_STAGE_COMPUTE_BIT;}
        VkDescriptorSetLayoutCreateInfo di{VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO};di.bindingCount=uint32_t(bs.size());di.pBindings=bs.data();
        if(vkCreateDescriptorSetLayout(dev,&di,nullptr,&p.dsl)!=VK_SUCCESS)throw std::runtime_error("vkCreateDescriptorSetLayout failed");
        VkPushConstantRange pr{};pr.stageFlags=VK_SHADER_STAGE_COMPUTE_BIT;pr.size=7u*sizeof(uint32_t);
        VkPipelineLayoutCreateInfo li{VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO};li.setLayoutCount=1;li.pSetLayouts=&p.dsl;li.pushConstantRangeCount=1;li.pPushConstantRanges=&pr;
        if(vkCreatePipelineLayout(dev,&li,nullptr,&p.layout)!=VK_SUCCESS)throw std::runtime_error("vkCreatePipelineLayout failed");
        VkPipelineShaderStageRequiredSubgroupSizeCreateInfo req{VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_REQUIRED_SUBGROUP_SIZE_CREATE_INFO};req.requiredSubgroupSize=32;
        VkPipelineShaderStageCreateInfo st{VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO};st.stage=VK_SHADER_STAGE_COMPUTE_BIT;st.module=p.mod;st.pName="main";
        if(candidate){st.pNext=&req;st.flags=VK_PIPELINE_SHADER_STAGE_CREATE_REQUIRE_FULL_SUBGROUPS_BIT;}
        VkComputePipelineCreateInfo ci{VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO};ci.stage=st;ci.layout=p.layout;
        if(vkCreateComputePipelines(dev,VK_NULL_HANDLE,1,&ci,nullptr,&p.pipe)!=VK_SUCCESS)throw std::runtime_error(candidate?"candidate pipeline failed":"baseline pipeline failed");
        return p;
    }
    BindSet bind(const Pipe& p,Buffer& w,Buffer& x,Buffer& b,Buffer& y){
        BindSet z;VkDescriptorPoolSize ps{VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,4};VkDescriptorPoolCreateInfo pi{VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO};pi.maxSets=1;pi.poolSizeCount=1;pi.pPoolSizes=&ps;
        if(vkCreateDescriptorPool(dev,&pi,nullptr,&z.pool)!=VK_SUCCESS)throw std::runtime_error("descriptor pool failed");
        VkDescriptorSetAllocateInfo ai{VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO};ai.descriptorPool=z.pool;ai.descriptorSetCount=1;ai.pSetLayouts=&p.dsl;
        if(vkAllocateDescriptorSets(dev,&ai,&z.set)!=VK_SUCCESS)throw std::runtime_error("descriptor set failed");
        std::array<VkDescriptorBufferInfo,4> inf={{{w.b,0,w.bytes},{x.b,0,x.bytes},{b.b,0,b.bytes},{y.b,0,y.bytes}}};
        std::array<VkWriteDescriptorSet,4> wr{};for(uint32_t i=0;i<4;++i){wr[i].sType=VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;wr[i].dstSet=z.set;wr[i].dstBinding=i;wr[i].descriptorCount=1;wr[i].descriptorType=VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;wr[i].pBufferInfo=&inf[i];}
        vkUpdateDescriptorSets(dev,uint32_t(wr.size()),wr.data(),0,nullptr);return z;
    }
    void destroy(BindSet& z){if(z.pool)vkDestroyDescriptorPool(dev,z.pool,nullptr);z={};}
    void destroy(Pipe& p){if(p.pipe)vkDestroyPipeline(dev,p.pipe,nullptr);if(p.layout)vkDestroyPipelineLayout(dev,p.layout,nullptr);if(p.dsl)vkDestroyDescriptorSetLayout(dev,p.dsl,nullptr);if(p.mod)vkDestroyShaderModule(dev,p.mod,nullptr);p={};}
    uint64_t dispatch(const Pipe& p,const BindSet& s,const Cell& c,bool candidate,bool timed){
        VkCommandBufferAllocateInfo ai{VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO};ai.commandPool=cp;ai.level=VK_COMMAND_BUFFER_LEVEL_PRIMARY;ai.commandBufferCount=1;
        VkCommandBuffer cb=VK_NULL_HANDLE;if(vkAllocateCommandBuffers(dev,&ai,&cb)!=VK_SUCCESS)throw std::runtime_error("alloc command buffer failed");
        VkQueryPool query=VK_NULL_HANDLE;if(timed){VkQueryPoolCreateInfo qi{VK_STRUCTURE_TYPE_QUERY_POOL_CREATE_INFO};qi.queryType=VK_QUERY_TYPE_TIMESTAMP;qi.queryCount=2;if(vkCreateQueryPool(dev,&qi,nullptr,&query)!=VK_SUCCESS)throw std::runtime_error("query pool failed");}
        VkCommandBufferBeginInfo bi{VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};bi.flags=VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;vkBeginCommandBuffer(cb,&bi);
        if(timed){vkCmdResetQueryPool(cb,query,0,2);vkCmdWriteTimestamp(cb,VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,query,0);}
        vkCmdBindPipeline(cb,VK_PIPELINE_BIND_POINT_COMPUTE,p.pipe);vkCmdBindDescriptorSets(cb,VK_PIPELINE_BIND_POINT_COMPUTE,p.layout,0,1,&s.set,0,nullptr);
        uint32_t pc[7]={c.n,c.rows,1u,c.row_bytes,c.add_bias,0u,0u};vkCmdPushConstants(cb,p.layout,VK_SHADER_STAGE_COMPUTE_BIT,0,sizeof(pc),pc);
        uint32_t gx=candidate?(c.rows+3u)/4u:(c.rows+63u)/64u;vkCmdDispatch(cb,gx,1,1);
        VkMemoryBarrier mb{VK_STRUCTURE_TYPE_MEMORY_BARRIER};mb.srcAccessMask=VK_ACCESS_SHADER_WRITE_BIT;mb.dstAccessMask=VK_ACCESS_HOST_READ_BIT;
        vkCmdPipelineBarrier(cb,VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,VK_PIPELINE_STAGE_HOST_BIT,0,1,&mb,0,nullptr,0,nullptr);
        if(timed)vkCmdWriteTimestamp(cb,VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,query,1);
        if(vkEndCommandBuffer(cb)!=VK_SUCCESS)throw std::runtime_error("end command buffer failed");
        VkFenceCreateInfo fi{VK_STRUCTURE_TYPE_FENCE_CREATE_INFO};VkFence fence=VK_NULL_HANDLE;vkCreateFence(dev,&fi,nullptr,&fence);
        VkSubmitInfo si{VK_STRUCTURE_TYPE_SUBMIT_INFO};si.commandBufferCount=1;si.pCommandBuffers=&cb;
        if(vkQueueSubmit(q,1,&si,fence)!=VK_SUCCESS)throw std::runtime_error("queue submit failed");
        if(vkWaitForFences(dev,1,&fence,VK_TRUE,UINT64_MAX)!=VK_SUCCESS)throw std::runtime_error("fence wait failed");
        uint64_t ns=0;if(timed){uint64_t t[2]={};if(vkGetQueryPoolResults(dev,query,0,2,sizeof(t),t,sizeof(uint64_t),VK_QUERY_RESULT_64_BIT|VK_QUERY_RESULT_WAIT_BIT)!=VK_SUCCESS)throw std::runtime_error("timestamp result failed");ns=uint64_t(double(t[1]-t[0])*double(props.limits.timestampPeriod));}
        vkDestroyFence(dev,fence,nullptr);if(query)vkDestroyQueryPool(dev,query,nullptr);vkFreeCommandBuffers(dev,cp,1,&cb);return ns;
    }
    ~VkCtx(){if(dev)vkDeviceWaitIdle(dev);if(cp)vkDestroyCommandPool(dev,cp,nullptr);if(dev)vkDestroyDevice(dev,nullptr);if(inst)vkDestroyInstance(inst,nullptr);}
};

struct BankGpu{Buffer w,x,b,y0,y1;BindSet s0,s1;};
struct CellGpu{std::vector<BankGpu> banks;};

static void emit_vec(std::ostream& o,const std::vector<uint64_t>& v){o<<"[";for(size_t i=0;i<v.size();++i){if(i)o<<",";o<<v[i];}o<<"]";}
static void emit_metric(std::ostream& o,const Metric&m){o<<"{\"finite\":"<<(m.finite?"true":"false")<<",\"max_abs\":"<<std::setprecision(12)<<m.max_abs<<",\"rmse\":"<<m.rmse<<"}";}

static void verify_metrics(const Metric&m,const char* label){
    if(!m.finite||m.max_abs>0.02||m.rmse>0.005)throw std::runtime_error(std::string("correctness gate failed: ")+label);
}

int main(int argc,char**argv){
    std::string mode,process,baseline_spv,candidate_spv,out,authorization,quant="q4";
    try{
        for(int i=1;i<argc;++i){std::string a=argv[i];auto need=[&](const char*n){if(i+1>=argc)throw std::runtime_error(std::string("missing ")+n);return std::string(argv[++i]);};
            if(a=="--mode")mode=need("--mode");else if(a=="--process")process=need("--process");else if(a=="--quant")quant=need("--quant");else if(a=="--baseline-spv")baseline_spv=need("--baseline-spv");
            else if(a=="--candidate-spv")candidate_spv=need("--candidate-spv");else if(a=="--out")out=need("--out");else if(a=="--authorization")authorization=need("--authorization");
            else throw std::runtime_error("unknown arg "+a);}
        if(mode!="preflight"&&mode!="measure")throw std::runtime_error("mode must be preflight or measure");
        if(quant!="q4"&&quant!="q6")throw std::runtime_error("quant must be q4 or q6");
        if(baseline_spv.empty()||candidate_spv.empty()||out.empty())throw std::runtime_error("shader/out args required");
        if(mode=="measure"&&(process!="A"&&process!="B"))throw std::runtime_error("measure requires process A or B");
        if(mode=="measure"&&authorization.empty())throw std::runtime_error("measure requires authorization path");
        if(mode=="measure"){std::ifstream af(authorization);std::ostringstream ss;ss<<af.rdbuf();const std::string required=(quant=="q6")?"SA1_Q6_EXECUTION_AUTHORIZED":"SA1_Q4_EXECUTION_AUTHORIZED";if(!af||ss.str().find(required)==std::string::npos)throw std::runtime_error("execution authorization invalid");}
        std::vector<Cell> active_cells;
        if(quant=="q6")active_cells.assign(kQ6Cells.begin(),kQ6Cells.end());else active_cells.assign(kQ4Cells.begin(),kQ4Cells.end());

        VkCtx vk;vk.init();Pipe base=vk.pipeline(baseline_spv,false),cand=vk.pipeline(candidate_spv,true);
        std::ofstream o(out,std::ios::binary);if(!o)throw std::runtime_error("cannot open output");
        o<<std::setprecision(15);
        o<<"{\n\"schema\":\""<<((quant=="q6")?"arcllm.sa1.k2.q6.component.v0.1":"arcllm.sa1.k1.component.v0.1")<<"\",\n\"mode\":\""<<mode<<"\",\n";
        o<<"\"quant\":\""<<((quant=="q6")?"Q6_K":"Q4_K")<<"\",\n";
        o<<"\"device\":\""<<esc(vk.props.deviceName)<<"\",\"timestamp_period_ns\":"<<vk.props.limits.timestampPeriod<<",\"subgroup_size_default\":"<<vk.subgroup.subgroupSize<<",\n";
        o<<"\"required_subgroup_size\":32,\"candidate_local_size\":128,\"model_loaded\":false,\n";

        if(mode=="preflight"){
            o<<"\"performance_measurement\":false,\"timestamp_queries\":0,\"measured_pairs\":0,\"performance_gate_evaluated\":false,\n\"cells\":[";
            for(size_t ci=0;ci<active_cells.size();++ci){
                if(ci)o<<",";
                const Cell& c=active_cells[ci];o<<"{\"id\":\""<<c.id<<"\",\"banks\":[";
                for(uint32_t bank : {0u,3u}){
                    if(bank==3u)o<<",";
                    Fixture f=make_fixture(c,bank);auto ref=cpu_ref(c,f);
                    Buffer w=vk.buffer(f.w.size(),f.w.data()),x=vk.buffer(f.x.size()*4,f.x.data()),b=vk.buffer(f.bias.size()*4,f.bias.data()),y0=vk.buffer(size_t(c.rows)*4),y1=vk.buffer(size_t(c.rows)*4);
                    BindSet s0=vk.bind(base,w,x,b,y0),s1=vk.bind(cand,w,x,b,y1);
                    vk.dispatch(base,s0,c,false,false);vk.dispatch(cand,s1,c,true,false);
                    std::vector<float> a(c.rows),d(c.rows);std::memcpy(a.data(),y0.map,a.size()*4);std::memcpy(d.data(),y1.map,d.size()*4);
                    Metric m0=metric(a,ref),m1=metric(d,ref),m2=metric(d,a);verify_metrics(m0,"baseline_cpu");verify_metrics(m1,"candidate_cpu");verify_metrics(m2,"candidate_baseline");
                    o<<"{\"bank\":"<<bank<<",\"baseline_cpu\":";emit_metric(o,m0);o<<",\"candidate_cpu\":";emit_metric(o,m1);o<<",\"candidate_baseline\":";emit_metric(o,m2);
                    o<<",\"baseline_hash\":\""<<hex64(fnv1a64(a.data(),a.size()*4))<<"\",\"candidate_hash\":\""<<hex64(fnv1a64(d.data(),d.size()*4))<<"\"}";
                    vk.destroy(s1);vk.destroy(s0);vk.destroy(y1);vk.destroy(y0);vk.destroy(b);vk.destroy(x);vk.destroy(w);
                }
                o<<"]}";
            }
            o<<"],\n\"status\":\"PASS_CORRECTNESS_ZERO_MEASUREMENT\"\n}\n";
        } else {
            std::vector<Cell> cells(active_cells.begin(),active_cells.end());if(process=="B")std::reverse(cells.begin(),cells.end());
            const uint32_t measured_pairs=uint32_t(cells.size())*30u;
            const uint32_t timed_dispatches=measured_pairs*2u;
            const uint32_t timestamp_values=timed_dispatches*2u;
            o<<"\"performance_measurement\":true,\"timed_dispatches_expected\":"<<timed_dispatches<<",\"timestamp_values_expected\":"<<timestamp_values<<",\"measured_pairs\":"<<measured_pairs<<",\"process\":\""<<process<<"\",\n\"cells\":[";
            for(size_t ci=0;ci<cells.size();++ci){
                if(ci)o<<",";const Cell& c=cells[ci];
                std::vector<BankGpu> banks(4);
                for(uint32_t bi=0;bi<4;++bi){auto&g=banks[bi];Fixture f=make_fixture(c,bi);g.w=vk.buffer(f.w.size(),f.w.data());g.x=vk.buffer(f.x.size()*4,f.x.data());g.b=vk.buffer(f.bias.size()*4,f.bias.data());g.y0=vk.buffer(size_t(c.rows)*4);g.y1=vk.buffer(size_t(c.rows)*4);g.s0=vk.bind(base,g.w,g.x,g.b,g.y0);g.s1=vk.bind(cand,g.w,g.x,g.b,g.y1);Fixture empty;f=std::move(empty);}
                for(int wi=0;wi<10;++wi){uint32_t bi=uint32_t(wi)&3u;vk.dispatch(base,banks[bi].s0,c,false,false);vk.dispatch(cand,banks[bi].s1,c,true,false);}
                std::vector<uint64_t> bn,cn;
                for(int pi=0;pi<30;++pi){uint32_t bi=uint32_t(pi)&3u;bool base_first=((pi&1)==0);if(process=="B")base_first=!base_first;
                    uint64_t tb=0,tc=0;if(base_first){tb=vk.dispatch(base,banks[bi].s0,c,false,true);tc=vk.dispatch(cand,banks[bi].s1,c,true,true);}else{tc=vk.dispatch(cand,banks[bi].s1,c,true,true);tb=vk.dispatch(base,banks[bi].s0,c,false,true);}bn.push_back(tb);cn.push_back(tc);}
                o<<"{\"id\":\""<<c.id<<"\",\"baseline_ns\":";emit_vec(o,bn);o<<",\"candidate_ns\":";emit_vec(o,cn);o<<"}";
                for(auto&g:banks){vk.destroy(g.s1);vk.destroy(g.s0);vk.destroy(g.y1);vk.destroy(g.y0);vk.destroy(g.b);vk.destroy(g.x);vk.destroy(g.w);}
            }
            o<<"],\n\"status\":\"MEASUREMENT_COMPLETE_NOT_ADJUDICATED\"\n}\n";
        }
        vk.destroy(cand);vk.destroy(base);return 0;
    }catch(const std::exception&e){std::cerr<<"SA1 component error: "<<e.what()<<"\n";if(!out.empty()){std::ofstream o(out,std::ios::binary);if(o)o<<"{\"schema\":\"arcllm.sa1.k1.component.v0.1\",\"status\":\"ERROR\",\"error\":\""<<esc(e.what())<<"\"}\n";}return 2;}
}
