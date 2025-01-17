// Copyright (c) 2008-2023 the Urho3D project
// Copyright (c) 2022-2023 the Dviglo project
// License: MIT

#include "../graphics/graphics.h"
#include "shader.h"
#include "shader_variation.h"

#include "../common/debug_new.h"

namespace dviglo
{

ShaderParameter::ShaderParameter(const String& name, unsigned glType, int location) :   // NOLINT(hicpp-member-init)
    name_{name},
    glType_{glType},
    location_{location}
{
}

ShaderParameter::ShaderParameter(ShaderType type, const String& name, unsigned offset, unsigned size, unsigned buffer) :    // NOLINT(hicpp-member-init)
    type_{type},
    name_{name},
    offset_{offset},
    size_{size},
    buffer_{buffer}
{
}

ShaderParameter::ShaderParameter(ShaderType type, const String& name, unsigned reg, unsigned regCount) :    // NOLINT(hicpp-member-init)
    type_{type},
    name_{name},
    register_{reg},
    regCount_{regCount}
{
}

ShaderVariation::ShaderVariation(Shader* owner, ShaderType type) :
    GPUObject(owner->GetSubsystem<Graphics>()),
    owner_(owner),
    type_(type)
{
}

ShaderVariation::~ShaderVariation()
{
    Release();
}

void ShaderVariation::SetName(const String& name)
{
    name_ = name;
}

Shader* ShaderVariation::GetOwner() const
{
    return owner_;
}

void ShaderVariation::OnDeviceLost()
{
    GAPI gapi = Graphics::GetGAPI();

#ifdef DV_OPENGL
    if (gapi == GAPI_OPENGL)
        return OnDeviceLost_OGL();
#endif

#ifdef DV_D3D11
    if (gapi == GAPI_D3D11)
        return OnDeviceLost_D3D11();
#endif
}

void ShaderVariation::Release()
{
    GAPI gapi = Graphics::GetGAPI();

#ifdef DV_OPENGL
    if (gapi == GAPI_OPENGL)
        return Release_OGL();
#endif

#ifdef DV_D3D11
    if (gapi == GAPI_D3D11)
        return Release_D3D11();
#endif
}

bool ShaderVariation::Create()
{
    GAPI gapi = Graphics::GetGAPI();

#ifdef DV_OPENGL
    if (gapi == GAPI_OPENGL)
        return Create_OGL();
#endif

#ifdef DV_D3D11
    if (gapi == GAPI_D3D11)
        return Create_D3D11();
#endif

    return {}; // Prevent warning
}

void ShaderVariation::SetDefines(const String& defines)
{
    GAPI gapi = Graphics::GetGAPI();

#ifdef DV_OPENGL
    if (gapi == GAPI_OPENGL)
        return SetDefines_OGL(defines);
#endif

#ifdef DV_D3D11
    if (gapi == GAPI_D3D11)
        return SetDefines_D3D11(defines);
#endif
}

}
