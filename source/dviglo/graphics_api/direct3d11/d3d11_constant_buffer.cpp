// Copyright (c) 2008-2023 the Urho3D project
// Copyright (c) 2022-2023 the Dviglo project
// License: MIT

#include "../../graphics/graphics.h"
#include "../constant_buffer.h"
#include "d3d11_graphics_impl.h"
#include "../../io/log.h"

#include "../../common/debug_new.h"

namespace dviglo
{

void ConstantBuffer::OnDeviceReset_D3D11()
{
    // No-op on Direct3D11
}

void ConstantBuffer::Release_D3D11()
{
    DV_SAFE_RELEASE(object_.ptr_);

    shadowData_.Reset();
    size_ = 0;
}

bool ConstantBuffer::SetSize_D3D11(unsigned size)
{
    Release_D3D11();

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
        D3D11_BUFFER_DESC bufferDesc;
        memset(&bufferDesc, 0, sizeof bufferDesc);

        bufferDesc.ByteWidth = size_;
        bufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
        bufferDesc.CPUAccessFlags = 0;
        bufferDesc.Usage = D3D11_USAGE_DEFAULT;

        HRESULT hr = graphics_->GetImpl_D3D11()->GetDevice()->CreateBuffer(&bufferDesc, 0, (ID3D11Buffer**)&object_.ptr_);
        if (FAILED(hr))
        {
            DV_SAFE_RELEASE(object_.ptr_);
            DV_LOGD3DERROR("Failed to create constant buffer", hr);
            return false;
        }
    }

    return true;
}

void ConstantBuffer::Apply_D3D11()
{
    if (dirty_ && object_.ptr_)
    {
        graphics_->GetImpl_D3D11()->GetDeviceContext()->UpdateSubresource((ID3D11Buffer*)object_.ptr_, 0, 0, shadowData_.Get(), 0, 0);
        dirty_ = false;
    }
}

}
