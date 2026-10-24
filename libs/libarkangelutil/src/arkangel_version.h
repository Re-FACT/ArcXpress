#ifndef ARKANGEL_VERSION_H
#define ARKANGEL_VERSION_H
#include <cstddef>

namespace arcxpress {
extern const char* VERSION;
extern const char* VERSION_SHORT;

extern const size_t VERSION_MAJOR;
extern const size_t VERSION_MINOR;
extern const size_t VERSION_PATCH;
extern const char* VERSION_PRERELEASE;

extern const char* VCS_REVISION;
extern const char* COMPILER;
extern const char* BUILD_TIMESTAMP;
extern const char* BUILD_INFO;
}  // namespace arcxpress

#endif
