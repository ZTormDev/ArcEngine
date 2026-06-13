#pragma once

#ifdef TRACY_ENABLE
#include <tracy/Tracy.hpp>
#define ARC_PROFILE_ZONE() ZoneScoped
#define ARC_PROFILE_FRAME() FrameMark
#else
#define ARC_PROFILE_ZONE()
#define ARC_PROFILE_FRAME()
#endif
