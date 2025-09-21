#ifndef DTB_H
#define DTB_H

typedef unsigned long long uint64_t;
typedef unsigned int uint32_t;
typedef unsigned short uint16_t;
typedef unsigned char uint8_t;

typedef struct {
    uint32_t magic;
    uint32_t totalsize;
    uint32_t off_dt_struct;
    uint32_t off_dt_strings;
    uint32_t off_mem_rsvmap;
    uint32_t version;
    uint32_t last_comp_version;
    uint32_t boot_cpuid_phys;
    uint32_t size_dt_strings;
    uint32_t size_dt_struct;
} fdt_header_t;

typedef struct {
    uint64_t address;
    uint64_t size;
} fdt_reserve_entry_t;

#define FDT_MAGIC       0xd00dfeed
#define FDT_BEGIN_NODE  0x1
#define FDT_END_NODE    0x2
#define FDT_PROP        0x3
#define FDT_NOP         0x4
#define FDT_END         0x9

void dtb_init(uint64_t dtb_addr);
int dtb_get_framebuffer_info(uint64_t* fb_base, uint32_t* fb_width, uint32_t* fb_height, uint32_t* fb_pitch);
int dtb_get_memory_info(uint64_t* mem_start, uint64_t* mem_size);
const void* dtb_get_property(const char* node_path, const char* prop_name, uint32_t* len);
uint32_t bswap32(uint32_t val);
uint64_t bswap64(uint64_t val);

#endif


