#include <stdlib.h>
#include <pthread.h>
#include <mach/mach.h>
#include <dispatch/dispatch.h>

#include "jailedhook.h"
#include "fishhook.h"
#include "mach_excServer.h"

static int g_hookCount = 0;
struct hook_entry g_hooks[MAX_SLOTS];
static struct arm_debug_state64 g_debugState = {};
static mach_port_t g_server = MACH_PORT_NULL;

// ----- exception handler ------ 
kern_return_t catch_mach_exception_raise(mach_port_t exception_port, mach_port_t thread, mach_port_t task, exception_type_t exception, mach_exception_data_t code, mach_msg_type_number_t codeCnt) {
    abort();
}

kern_return_t catch_mach_exception_raise_state_identity(mach_port_t exception_port, mach_port_t thread, mach_port_t task, exception_type_t exception, mach_exception_data_t code, mach_msg_type_number_t codeCnt, int *flavor, thread_state_t old_state, mach_msg_type_number_t old_stateCnt, thread_state_t new_state, mach_msg_type_number_t *new_stateCnt) {
    abort();
}

kern_return_t catch_mach_exception_raise_state(mach_port_t exception_port, exception_type_t exception, const mach_exception_data_t code, mach_msg_type_number_t codeCnt, int *flavor, const thread_state_t old_state, mach_msg_type_number_t old_stateCnt, thread_state_t new_state, mach_msg_type_number_t *new_stateCnt) {
    arm_thread_state64_t *old = (arm_thread_state64_t *)old_state;
    arm_thread_state64_t *new = (arm_thread_state64_t *)new_state;

    uint64_t pc = arm_thread_state64_get_pc(*old);

    for (int i = 0; i < MAX_SLOTS; ++i) {
        if (g_hooks[i].target && (uint64_t)g_hooks[i].target == pc) {
            *new = *old;
            *new_stateCnt = old_stateCnt;
            arm_thread_state64_set_pc_fptr(*new, g_hooks[i].replacement);
            return KERN_SUCCESS;
        }
    }

    return KERN_FAILURE;
}

static void *exception_handler_thread(void *arg) {
    mach_msg_server(mach_exc_server, sizeof(union __RequestUnion__catch_mach_exc_subsystem), g_server, MACH_MSG_OPTION_NONE);
    abort();
}

static kern_return_t (*orig_task_set_exception_ports)(task_t, exception_mask_t, mach_port_t, exception_behavior_t, thread_state_flavor_t) = NULL;
static kern_return_t hooked_task_set_exception_ports(task_t task, exception_mask_t exception_mask, mach_port_t new_port, exception_behavior_t behavior, thread_state_flavor_t new_flavor) {
    if (exception_mask == EXC_MASK_BREAKPOINT) return KERN_SUCCESS;
    exception_mask &= ~EXC_MASK_BREAKPOINT;
    return orig_task_set_exception_ports(task, exception_mask, new_port, behavior, new_flavor);
}

static void KHLaunchExceptionHandler(void) {
    if (g_server != MACH_PORT_NULL) return;

    kern_return_t kr = mach_port_allocate(mach_task_self(), MACH_PORT_RIGHT_RECEIVE, &g_server);
    if (kr != KERN_SUCCESS) return;
    mach_port_insert_right(mach_task_self(), g_server, g_server, MACH_MSG_TYPE_MAKE_SEND);

    task_set_exception_ports(mach_task_self(), EXC_MASK_BREAKPOINT, g_server, EXCEPTION_STATE | MACH_EXCEPTION_CODES, ARM_THREAD_STATE64);

    pthread_t thr;
    pthread_create(&thr, NULL, exception_handler_thread, NULL);

    struct rebinding rebindings[] = { { "task_set_exception_ports", hooked_task_set_exception_ports, (void*)&orig_task_set_exception_ports } };
    rebind_symbols(rebindings, 1);
}


// ------ hooker -----
void KHJailedHook(void* target, void* replacement, int slot) {
    if (slot < 0 || slot >= MAX_SLOTS) return;

    KHLaunchExceptionHandler();

    void* t = (void*)((uint64_t)target & 0x0000007fffffffff);
    void* r = (void*)((uint64_t)replacement & 0x0000007fffffffff);

    g_hooks[slot].target = t;
    g_hooks[slot].replacement = r;

    g_hookCount = 0;
    for (int i = 0; i < MAX_SLOTS; ++i) if (g_hooks[i].target) g_hookCount++;

    for (int i = 0; i < MAX_SLOTS; ++i) {
        if (g_hooks[i].target) {
            g_debugState.bvr[i] = (uint64_t)g_hooks[i].target;
            g_debugState.bcr[i] = 0x1e5; 
        } else {
            g_debugState.bvr[i] = 0;
            g_debugState.bcr[i] = 0;
        }
    }

    kern_return_t kret = task_set_state(mach_task_self(), ARM_DEBUG_STATE64, (thread_state_t)&g_debugState, ARM_DEBUG_STATE64_COUNT_);
    if (kret != KERN_SUCCESS) return;

    thread_act_array_t act_list;
    mach_msg_type_number_t listCnt;
    if (task_threads(mach_task_self(), &act_list, &listCnt) != KERN_SUCCESS) return;
    for (mach_msg_type_number_t i = 0; i < listCnt; ++i) {
        thread_t th = act_list[i];
        thread_set_state(th, ARM_DEBUG_STATE64, (thread_state_t)&g_debugState, ARM_DEBUG_STATE64_COUNT_);
        mach_port_deallocate(mach_task_self(), th);
    }
    vm_deallocate(mach_task_self(), (mach_vm_address_t)act_list, listCnt * sizeof(thread_t));
}

void KHJailedSafeHook(void* target, void* replacement, int slot) {
    dispatch_async(dispatch_get_main_queue(), ^{
        KHJailedHook(target, replacement, slot);
    });
}

void KHJailedUnhook(int slot) {
    if (slot < 0 || slot >= MAX_SLOTS) return;
    g_hooks[slot].target = NULL;
    g_hooks[slot].replacement = NULL;

    g_hookCount = 0;
    for (int i = 0; i < MAX_SLOTS; ++i) if (g_hooks[i].target) g_hookCount++;

    for (int i = 0; i < MAX_SLOTS; ++i) {
        if (g_hooks[i].target) {
            g_debugState.bvr[i] = (uint64_t)g_hooks[i].target;
            g_debugState.bcr[i] = 0x1e5;
        } else {
            g_debugState.bvr[i] = 0;
            g_debugState.bcr[i] = 0;
        }
    }

    task_set_state(mach_task_self(), ARM_DEBUG_STATE64, (thread_state_t)&g_debugState, ARM_DEBUG_STATE64_COUNT_);

    thread_act_array_t act_list;
    mach_msg_type_number_t listCnt;
    if (task_threads(mach_task_self(), &act_list, &listCnt) != KERN_SUCCESS) return;
    for (mach_msg_type_number_t i = 0; i < listCnt; ++i) {
        thread_t th = act_list[i];
        thread_set_state(th, ARM_DEBUG_STATE64, (thread_state_t)&g_debugState, ARM_DEBUG_STATE64_COUNT_);
        mach_port_deallocate(mach_task_self(), th);
    }
    vm_deallocate(mach_task_self(), (mach_vm_address_t)act_list, listCnt * sizeof(thread_t));
}
