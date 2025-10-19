#pragma once

#include "SDLRenderer.h"

namespace Engine {

// Default renderer is SDL-based
// For backward compatibility, Renderer is an alias for SDLRenderer
using Renderer = SDLRenderer;

} // namespace Engine
