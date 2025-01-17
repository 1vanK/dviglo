// Copyright (c) 2008-2023 the Urho3D project
// Copyright (c) 2022-2023 the Dviglo project
// License: MIT

#include "../../graphics/graphics.h"
#include "../constant_buffer.h"
#include "../graphics_impl.h"
#include "../../io/log.h"

#include "../../common/debug_new.h"

namespace dviglo
{

void ConstantBuffer::Release_OGL()
{
    if (object_.name_)
    {
        if (!graphics_)
            return;

#ifndef GL_ES_VERSION_2_0
        graphics_->SetUBO_OGL(0);
        glDeleteBuffers(1, &object_.name_);
#endif
        object_.name_ = 0;
    }

    shadowData_.Reset();
    size_ = 0;
}

void ConstantBuffer::OnDeviceReset_OGL()
{
    if (size_)
        SetSize_OGL(size_); // Recreate
}

bool ConstantBuffer::SetSize_OGL(unsigned size)
{
    if (!size)
    {
        DV_LOGERROR("Can not create zero-sized constant buffer");
        return false;
    }

    // Round up to next 16 bytes
    size += 15;
    size &= 0xfffffff0;

    size_ = size;
    dirty_ = false;
    shadowData_ = new unsigned char[size_];
    memset(shadowData_.Get(), 0, size_);

    if (graphics_)
    {
#ifndef GL_ES_VERSION_2_0
        if (!object_.name_)
            glGenBuffers(1, &object_.name_);
        graphics_->SetUBO_OGL(object_.name_);
        glBufferData(GL_UNIFORM_BUFFER, size_, shadowData_.Get(), GL_DYNAMIC_DRAW);
#endif
    }

    return true;
}

void ConstantBuffer::Apply_OGL()
{
    if (dirty_ && object_.name_)
    {
#ifndef GL_ES_VERSION_2_0
        graphics_->SetUBO_OGL(object_.name_);
        glBufferData(GL_UNIFORM_BUFFER, size_, shadowData_.Get(), GL_DYNAMIC_DRAW);
#endif
        dirty_ = false;
    }
}

}
