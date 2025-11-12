#ifdef __cplusplus
extern "C" {
#endif

#define KHOrigBody(prologue_instr, slot)                                   \
__asm__ volatile (                                                         \
    ".extern _g_hooks\n\t"                                                 \
    "adrp x16, _g_hooks@PAGE\n\t"                                          \
    "add  x16, x16, _g_hooks@PAGEOFF\n\t"                                  \
    "ldr x16, [x16, %c[offset]]\n\t"                                       \
    "add  x16, x16, #4\n\t"                                                \
    prologue_instr "\n\t"                                                  \
    "br   x16\n\t"                                                         \
    :                                                                      \
    : [offset] "i" (HOOKENTRY_SIZE * (slot))                               \
); 

#define MAX_SLOTS 6
#define HOOKENTRY_SIZE (sizeof(struct hook_entry))

struct hook_entry{
    void* target;
    void* replacement;
};


#define ARM_DEBUG_STATE64 15
#define ARM_DEBUG_STATE64_COUNT_ ((mach_msg_type_number_t) (sizeof (struct arm_debug_state64)/sizeof(uint32_t)))

struct arm_debug_state64 {
    __uint64_t        bvr[16];
    __uint64_t        bcr[16];
    __uint64_t        wvr[16];
    __uint64_t        wcr[16];
    __uint64_t      mdscr_el1;
};


void KHJailedHook(void* target, void* replacement, int slot);
void KHJailedSafeHook(void* target, void* replacement, int slot);
void KHJailedUnhook(int slot);

#ifdef __cplusplus
}
#endif