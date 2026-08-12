#include "Interfaces.hpp"

#include <windows.h>

namespace Interfaces {
using CreateInterfaceFn = void* (*)(const char* pName, int* pReturnCode);

void* Get(const char* moduleName, const char* interfaceName) {
    HMODULE hModule = GetModuleHandleA(moduleName);
    if (!hModule) {
        return nullptr;
    }

    auto createInterface =
        reinterpret_cast<CreateInterfaceFn>(GetProcAddress(hModule, "CreateInterface"));
    if (!createInterface) {
        return nullptr;
    }

    return createInterface(interfaceName, nullptr);
}
} // namespace Interfaces
