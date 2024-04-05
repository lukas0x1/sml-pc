#include <windows.h>
#include "include/global_variables.h"

namespace GlobalVariables {
    uintptr_t gamePtr = 0x23B2FB0;
    uintptr_t candleBarnPtr = 0x23EC4D8;
    // add new variables to below line

    // add new variables to above line
    uintptr_t end = -1;

    void InitGlobalVariables(uintptr_t skyBase) {
        uintptr_t* p = &gamePtr;
        while(p != &end) {
            uintptr_t tmp = 0;
            while((tmp = *(uintptr_t *)(skyBase + *p)) == NULL) Sleep(100);
            *p = tmp;
            p ++;
        }
    }
}