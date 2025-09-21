#include "dtb.h"
#include "kprintf.h"
#include "lib.h"

static uint64_t _dtb_base_address = 0;
static const char* _dt_strings_ptr = 0;

uint32_t bswap32(uint32_t val) {
    return ((val << 24) & 0xFF000000U) |
           ((val << 8) & 0x00FF0000U) |
           ((val >> 8) & 0x0000FF00U) |
           ((val >> 24) & 0x000000FFU);
}

uint64_t bswap64(uint64_t val) {
    return ((val << 56) & 0xFF00000000000000ULL) |
           ((val << 40) & 0x00FF000000000000ULL) |
           ((val << 24) & 0x0000FF0000000000ULL) |
           ((val << 8) & 0x000000FF00000000ULL) |
           ((val >> 8) & 0x00000000FF000000ULL) |
           ((val >> 24) & 0x0000000000FF0000ULL) |
           ((val >> 40) & 0x000000000000FF00ULL) |
           ((val >> 56) & 0x00000000000000FFULL);
}

void dtb_init(uint64_t dtb_addr) {
    _dtb_base_address = dtb_addr;
    fdt_header_t* header = (fdt_header_t*)_dtb_base_address;

    if (bswap32(header->magic) != FDT_MAGIC) {
        _dtb_base_address = 0;
        return;
    }

    _dt_strings_ptr = (const char*)(_dtb_base_address + bswap32(header->off_dt_strings));
}

static const void* dtb_find_node(const char* path) {
    if (!_dtb_base_address) return 0;

    fdt_header_t* header = (fdt_header_t*)_dtb_base_address;
    const char* struct_ptr = (const char*)(_dtb_base_address + bswap32(header->off_dt_struct));

    const char* current_path_segment = path;
    const char* next_slash = strchr(path, '/');

    int depth = 0;
    int path_match_depth = 0;

    while (bswap32(*(uint32_t*)struct_ptr) != FDT_END) {
        uint32_t tag = bswap32(*(uint32_t*)struct_ptr);
        struct_ptr += 4;

        if (tag == FDT_BEGIN_NODE) {
            const char* node_name = struct_ptr;
            struct_ptr += strlen(node_name) + 1;
            struct_ptr = (const char*)ALIGN_UP((uint64_t)struct_ptr, 4);

            if (depth == path_match_depth) {
                const char* name_end = strchr(node_name, '@');
                uint64_t name_len = (name_end) ? (uint64_t)(name_end - node_name) : strlen(node_name);

                if (next_slash) {
                    if (strncmp(node_name, current_path_segment, next_slash - current_path_segment) == 0 &&
                        (next_slash - current_path_segment) == name_len) {
                        path_match_depth++;
                        current_path_segment = next_slash + 1;
                        next_slash = strchr(current_path_segment, '/');
                    }
                } else {
                    if (strcmp(node_name, current_path_segment) == 0 ||
                        (name_end && strncmp(node_name, current_path_segment, name_len) == 0 && strlen(current_path_segment) == name_len)) {
                        return struct_ptr - 4;
                    }
                }
            }
            depth++;
        } else if (tag == FDT_END_NODE) {
            depth--;
            if (depth < path_match_depth) {
                path_match_depth = depth;
                if (current_path_segment != path) {
                    char* last_slash = (char*)current_path_segment - 2;
                    while (last_slash > path && *last_slash != '/') last_slash--;
                    current_path_segment = (last_slash == path) ? path : last_slash + 1;
                    next_slash = strchr(current_path_segment, '/');
                }
            }
        } else if (tag == FDT_PROP) {
            uint32_t len_prop = bswap32(*(uint32_t*)struct_ptr);
            struct_ptr += 4;
            struct_ptr += 4;
            struct_ptr += len_prop;
            struct_ptr = (const char*)ALIGN_UP((uint64_t)struct_ptr, 4);
        } else if (tag == FDT_NOP) {
        }
    }
    return 0;
}

const void* dtb_get_property(const char* node_path, const char* prop_name, uint32_t* len) {
    if (!_dtb_base_address || !_dt_strings_ptr) return 0;

    fdt_header_t* header = (fdt_header_t*)_dtb_base_address;
    const char* struct_ptr = (const char*)(_dtb_base_address + bswap32(header->off_dt_struct));

    const char* current_node_path = node_path;
    const char* next_slash = strchr(node_path, '/');
    int node_found = 0;
    int depth = 0;
    int target_depth = 0;

    for (int i = 0; node_path[i] != '\0'; i++) {
        if (node_path[i] == '/') target_depth++;
    }
    if (node_path[0] == '/') target_depth--;

    while (bswap32(*(uint32_t*)struct_ptr) != FDT_END) {
        uint32_t tag = bswap32(*(uint32_t*)struct_ptr);
        struct_ptr += 4;

        if (tag == FDT_BEGIN_NODE) {
            const char* node_name = struct_ptr;
            struct_ptr += strlen(node_name) + 1;
            struct_ptr = (const char*)ALIGN_UP((uint64_t)struct_ptr, 4);
            depth++;

            if (!node_found && depth == target_depth + 1) {
                const char* name_end = strchr(node_name, '@');
                uint64_t name_len = (name_end) ? (uint64_t)(name_end - node_name) : strlen(node_name);

                if (next_slash) {
                    if (strncmp(node_name, current_node_path, next_slash - current_node_path) == 0 &&
                        (next_slash - current_node_path) == name_len) {
                        current_node_path = next_slash + 1;
                        next_slash = strchr(current_node_path, '/');
                        if (*current_node_path == '\0') {
                            node_found = 1;
                        }
                    }
                } else {
                    if (strcmp(node_name, current_node_path) == 0 ||
                        (name_end && strncmp(node_name, current_node_path, name_len) == 0 && strlen(current_node_path) == name_len)) {
                        node_found = 1;
                    }
                }
            }
        } else if (tag == FDT_END_NODE) {
            if (node_found) {
                return 0;
            }
            depth--;
        } else if (tag == FDT_PROP) {
            uint32_t len_prop = bswap32(*(uint32_t*)struct_ptr);
            struct_ptr += 4;
            uint32_t nameoff_prop = bswap32(*(uint32_t*)struct_ptr);
            struct_ptr += 4;

            const char* prop_string = _dt_strings_ptr + nameoff_prop;
            const void* prop_value = struct_ptr;

            struct_ptr += len_prop;
            struct_ptr = (const char*)ALIGN_UP((uint64_t)struct_ptr, 4);

            if (node_found && strcmp(prop_string, prop_name) == 0) {
                if (len) *len = len_prop;
                return prop_value;
            }
        } else if (tag == FDT_NOP) {
        }
    }
    return 0;
}

int dtb_get_framebuffer_info(uint64_t* fb_base, uint32_t* fb_width, uint32_t* fb_height, uint32_t* fb_pitch) {
    if (!_dtb_base_address) return -1;

    const char* fb_node_path = "/chosen";
    const char* fb_prop_name = "framebuffer";

    uint32_t len;
    const uint32_t* prop_val = (const uint32_t*)dtb_get_property(fb_node_path, fb_prop_name, &len);

    if (prop_val && len >= (4 * sizeof(uint32_t))) {
        *fb_base = bswap32(prop_val[0]);
        *fb_width = bswap32(prop_val[1]);
        *fb_height = bswap32(prop_val[2]);
        *fb_pitch = bswap32(prop_val[3]);
        return 0;
    }

    return -1;
}

int dtb_get_memory_info(uint64_t* mem_start, uint64_t* mem_size) {
    if (!_dtb_base_address) return -1;

    const char* mem_node_path = "/memory";
    const char* mem_prop_name = "reg";

    uint32_t len;
    const uint64_t* prop_val = (const uint64_t*)dtb_get_property(mem_node_path, mem_prop_name, &len);

    if (prop_val && len >= (2 * sizeof(uint64_t))) {
        *mem_start = bswap64(prop_val[0]);
        *mem_size = bswap64(prop_val[1]);
        return 0;
    }
    return -1;
}


