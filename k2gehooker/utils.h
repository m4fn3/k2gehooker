#import <mach-o/dyld.h>

uintptr_t get_real_offset(const char *substr, uint64_t offset) {
    uint32_t count = _dyld_image_count();
    for (uint32_t i = 0; i < count; i++) {
        const char *name = _dyld_get_image_name(i);
        if (name == NULL) continue;
        if (strstr(name, substr) != NULL) {
            return _dyld_get_image_vmaddr_slide(i) + offset;
        }
    }
    return 0;
}