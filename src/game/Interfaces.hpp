#pragma once

namespace Interfaces {
void* Get(const char* moduleName, const char* interfaceName);

template<typename T>
T* Get(const char* moduleName, const char* interfaceName) {
    return static_cast<T*>(Get(moduleName, interfaceName));
}
} // namespace Interfaces
