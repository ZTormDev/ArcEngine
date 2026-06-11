#pragma once

// Placeholder for the future Google Filament backend.
// Keep Filament isolated here so Game never includes Filament headers directly.

namespace arc
{
    class FilamentContext
    {
    public:
        bool Init();
        void Shutdown();
    };
}
