#import <Foundation/Foundation.h>
#include "k2gehooker/jailedhook.h"
#include "k2gehooker/utils.h"

// original implementation 
__attribute__((naked)) void* orig_func(void*, void*) {
    KHOrigBody("sub sp, sp, #0xb0", 0); // specify the first instruction of the target function and slot number
}

// replacement implementation
void* func(void* arg0, void* arg1) {
    NSLog(@"[k2gehooker] func called!");
    return orig_func(arg0, arg1);
}

int func2(void* arg0, long arg1, void* arg2) {
    NSLog(@"[k2gehooker] func2 called!");
    return 1;
}

%ctor {
    // slot number can be 0~5, you can hook up to 6 functions simultaneously
    
    // set up hooks
    KHJailedHook(
        (void*)get_real_offset("LINE", 0x103111111),  // binary name and target function address
        (void*)&func, // replacement function
        0 // slot number
    );

    KHJailedHook(
        (void*)get_real_offset("LINE", 0x103222222), 
        (void*)&func2, 
        1
    );
    
    // If you got crashes due to threading issues, try using safe hook version instead
    // KHJailedSafeHook( // simply replace KHJailedHook with KHJailedSafeHook
    //     (void*)get_real_offset("LINE", 0x103222222), 
    //     (void*)&func2, 
    //     1
    // );

    KHJailedUnhook(1); // unhook the second hook, now you can hook another function using slot 1 if needed
}

