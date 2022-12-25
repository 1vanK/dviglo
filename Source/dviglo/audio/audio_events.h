// Copyright (c) 2008-2022 the Urho3D project
// Copyright (c) 2022-2022 the Dviglo project
// License: MIT

#pragma once

#include "../core/object.h"

namespace Urho3D
{

/// Sound playback finished. Sent through the SoundSource's Node.
URHO3D_EVENT(E_SOUNDFINISHED, SoundFinished)
{
    URHO3D_PARAM(P_NODE, Node);                     // Node pointer
    URHO3D_PARAM(P_SOUNDSOURCE, SoundSource);       // SoundSource pointer
    URHO3D_PARAM(P_SOUND, Sound);                   // Sound pointer
}

}