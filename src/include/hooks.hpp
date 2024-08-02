#ifndef HOOKS_HPP
#define HOOKS_HPP

#include <Windows.h>

namespace Hooks {
    void Init();
    void Free();

    inline bool bShuttingDown;
}

namespace H = Hooks;
#endif // HOOKS_HPP