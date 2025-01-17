// Copyright (c) 2008-2023 the Urho3D project
// Copyright (c) 2022-2023 the Dviglo project
// License: MIT

#include "../../core/context.h"
#include "../../core/profiler.h"
#include "../../graphics/graphics.h"
#include "../../graphics/graphics_events.h"
#include "../../graphics/renderer.h"
#include "d3d11_graphics_impl.h"
#include "../texture_3d.h"
#include "../../io/file_system.h"
#include "../../io/log.h"
#include "../../resource/resource_cache.h"
#include "../../resource/xml_file.h"

#include "../../common/debug_new.h"

namespace dviglo
{

void Texture3D::OnDeviceLost_D3D11()
{
    // No-op on Direct3D11
}

void Texture3D::OnDeviceReset_D3D11()
{
    // No-op on Direct3D11
}

void Texture3D::Release_D3D11()
{
    if (graphics_ && object_.ptr_)
    {
        for (unsigned i = 0; i < MAX_TEXTURE_UNITS; ++i)
        {
            if (graphics_->GetTexture(i) == this)
                graphics_->SetTexture(i, nullptr);
        }
    }

    DV_SAFE_RELEASE(object_.ptr_);
    DV_SAFE_RELEASE(shaderResourceView_);
    DV_SAFE_RELEASE(sampler_);
}

bool Texture3D::SetData_D3D11(unsigned level, int x, int y, int z, int width, int height, int depth, const void* data)
{
    DV_PROFILE(SetTextureData);

    if (!object_.ptr_)
    {
        DV_LOGERROR("No texture created, can not set data");
        return false;
    }

    if (!data)
    {
        DV_LOGERROR("Null source for setting data");
        return false;
    }

    if (level >= levels_)
    {
        DV_LOGERROR("Illegal mip level for setting data");
        return false;
    }

    int levelWidth = GetLevelWidth(level);
    int levelHeight = GetLevelHeight(level);
    int levelDepth = GetLevelDepth(level);
    if (x < 0 || x + width > levelWidth || y < 0 || y + height > levelHeight || z < 0 || z + depth > levelDepth || width <= 0 ||
        height <= 0 || depth <= 0)
    {
        DV_LOGERROR("Illegal dimensions for setting data");
        return false;
    }

    // If compressed, align the update region on a block
    if (IsCompressed_D3D11())
    {
        x &= ~3;
        y &= ~3;
        width += 3;
        width &= 0xfffffffc;
        height += 3;
        height &= 0xfffffffc;
    }

    unsigned char* src = (unsigned char*)data;
    unsigned rowSize = GetRowDataSize_D3D11(width);
    unsigned rowStart = GetRowDataSize_D3D11(x);
    unsigned subResource = D3D11CalcSubresource(level, 0, levels_);

    if (usage_ == TEXTURE_DYNAMIC)
    {
        if (IsCompressed_D3D11())
        {
            height = (height + 3) >> 2;
            y >>= 2;
        }

        D3D11_MAPPED_SUBRESOURCE mappedData;
        mappedData.pData = nullptr;

        HRESULT hr = graphics_->GetImpl_D3D11()->GetDeviceContext()->Map((ID3D11Resource*)object_.ptr_, subResource, D3D11_MAP_WRITE_DISCARD, 0,
            &mappedData);
        if (FAILED(hr) || !mappedData.pData)
        {
            DV_LOGD3DERROR("Failed to map texture for update", hr);
            return false;
        }
        else
        {
            for (int page = 0; page < depth; ++page)
            {
                for (int row = 0; row < height; ++row)
                {
                    memcpy((unsigned char*)mappedData.pData + (page + z) * mappedData.DepthPitch + (row + y) * mappedData.RowPitch +
                           rowStart, src + row * rowSize, rowSize);
                }
            }

            graphics_->GetImpl_D3D11()->GetDeviceContext()->Unmap((ID3D11Resource*)object_.ptr_, subResource);
        }
    }
    else
    {
        if (IsCompressed_D3D11())
            levelHeight = (levelHeight + 3) >> 2;

        D3D11_BOX destBox;
        destBox.left = (UINT)x;
        destBox.right = (UINT)(x + width);
        destBox.top = (UINT)y;
        destBox.bottom = (UINT)(y + height);
        destBox.front = (UINT)z;
        destBox.back = (UINT)(z + depth);

        graphics_->GetImpl_D3D11()->GetDeviceContext()->UpdateSubresource((ID3D11Resource*)object_.ptr_, subResource, &destBox, data,
            rowSize, levelHeight * rowSize);
    }

    return true;
}

bool Texture3D::SetData_D3D11(Image* image, bool useAlpha)
{
    if (!image)
    {
        DV_LOGERROR("Null image, can not load texture");
        return false;
    }

    // Use a shared ptr for managing the temporary mip images created during this function
    SharedPtr<Image> mipImage;
    unsigned memoryUse = sizeof(Texture3D);
    MaterialQuality quality = QUALITY_HIGH;
    Renderer* renderer = GetSubsystem<Renderer>();
    if (renderer)
        quality = renderer->GetTextureQuality();

    if (!image->IsCompressed())
    {
        // Convert unsuitable formats to RGBA
        unsigned components = image->GetComponents();
        if ((components == 1 && !useAlpha) || components == 2 || components == 3)
        {
            mipImage = image->ConvertToRGBA(); image = mipImage;
            if (!image)
                return false;
            components = image->GetComponents();
        }

        unsigned char* levelData = image->GetData();
        int levelWidth = image->GetWidth();
        int levelHeight = image->GetHeight();
        int levelDepth = image->GetDepth();
        unsigned format = 0;

        // Discard unnecessary mip levels
        for (unsigned i = 0; i < mipsToSkip_[quality]; ++i)
        {
            mipImage = image->GetNextLevel(); image = mipImage;
            levelData = image->GetData();
            levelWidth = image->GetWidth();
            levelHeight = image->GetHeight();
            levelDepth = image->GetDepth();
        }

        switch (components)
        {
        case 1:
            format = Graphics::GetAlphaFormat();
            break;

        case 4:
            format = Graphics::GetRGBAFormat();
            break;

        default: break;
        }

        // If image was previously compressed, reset number of requested levels to avoid error if level count is too high for new size
        if (IsCompressed_D3D11() && requestedLevels_ > 1)
            requestedLevels_ = 0;
        SetSize(levelWidth, levelHeight, levelDepth, format);

        for (unsigned i = 0; i < levels_; ++i)
        {
            SetData_D3D11(i, 0, 0, 0, levelWidth, levelHeight, levelDepth, levelData);
            memoryUse += levelWidth * levelHeight * levelDepth * components;

            if (i < levels_ - 1)
            {
                mipImage = image->GetNextLevel(); image = mipImage;
                levelData = image->GetData();
                levelWidth = image->GetWidth();
                levelHeight = image->GetHeight();
                levelDepth = image->GetDepth();
            }
        }
    }
    else
    {
        int width = image->GetWidth();
        int height = image->GetHeight();
        int depth = image->GetDepth();
        unsigned levels = image->GetNumCompressedLevels();
        unsigned format = graphics_->GetFormat(image->GetCompressedFormat());
        bool needDecompress = false;

        if (!format)
        {
            format = Graphics::GetRGBAFormat();
            needDecompress = true;
        }

        unsigned mipsToSkip = mipsToSkip_[quality];
        if (mipsToSkip >= levels)
            mipsToSkip = levels - 1;
        while (mipsToSkip && (width / (1 << mipsToSkip) < 4 || height / (1 << mipsToSkip) < 4 || depth / (1 << mipsToSkip) < 4))
            --mipsToSkip;
        width /= (1 << mipsToSkip);
        height /= (1 << mipsToSkip);
        depth /= (1 << mipsToSkip);

        SetNumLevels(Max((levels - mipsToSkip), 1U));
        SetSize(width, height, depth, format);

        for (unsigned i = 0; i < levels_ && i < levels - mipsToSkip; ++i)
        {
            CompressedLevel level = image->GetCompressedLevel(i + mipsToSkip);
            if (!needDecompress)
            {
                SetData_D3D11(i, 0, 0, 0, level.width_, level.height_, level.depth_, level.data_);
                memoryUse += level.depth_ * level.rows_ * level.rowSize_;
            }
            else
            {
                unsigned char* rgbaData = new unsigned char[level.width_ * level.height_ * level.depth_ * 4];
                level.Decompress(rgbaData);
                SetData_D3D11(i, 0, 0, 0, level.width_, level.height_, level.depth_, rgbaData);
                memoryUse += level.width_ * level.height_ * level.depth_ * 4;
                delete[] rgbaData;
            }
        }
    }

    SetMemoryUse(memoryUse);
    return true;
}

bool Texture3D::GetData_D3D11(unsigned level, void* dest) const
{
    if (!object_.ptr_)
    {
        DV_LOGERROR("No texture created, can not get data");
        return false;
    }

    if (!dest)
    {
        DV_LOGERROR("Null destination for getting data");
        return false;
    }

    if (level >= levels_)
    {
        DV_LOGERROR("Illegal mip level for getting data");
        return false;
    }

    int levelWidth = GetLevelWidth(level);
    int levelHeight = GetLevelHeight(level);
    int levelDepth = GetLevelDepth(level);

    D3D11_TEXTURE3D_DESC textureDesc;
    memset(&textureDesc, 0, sizeof textureDesc);
    textureDesc.Width = (UINT)levelWidth;
    textureDesc.Height = (UINT)levelHeight;
    textureDesc.Depth = (UINT)levelDepth;
    textureDesc.MipLevels = 1;
    textureDesc.Format = (DXGI_FORMAT)format_;
    textureDesc.Usage = D3D11_USAGE_STAGING;
    textureDesc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;

    ID3D11Texture3D* stagingTexture = nullptr;
    HRESULT hr = graphics_->GetImpl_D3D11()->GetDevice()->CreateTexture3D(&textureDesc, nullptr, &stagingTexture);
    if (FAILED(hr))
    {
        DV_LOGD3DERROR("Failed to create staging texture for GetData", hr);
        DV_SAFE_RELEASE(stagingTexture);
        return false;
    }

    unsigned srcSubResource = D3D11CalcSubresource(level, 0, levels_);
    D3D11_BOX srcBox;
    srcBox.left = 0;
    srcBox.right = (UINT)levelWidth;
    srcBox.top = 0;
    srcBox.bottom = (UINT)levelHeight;
    srcBox.front = 0;
    srcBox.back = (UINT)levelDepth;
    graphics_->GetImpl_D3D11()->GetDeviceContext()->CopySubresourceRegion(stagingTexture, 0, 0, 0, 0, (ID3D11Resource*)object_.ptr_,
        srcSubResource, &srcBox);

    D3D11_MAPPED_SUBRESOURCE mappedData;
    mappedData.pData = nullptr;
    unsigned rowSize = GetRowDataSize_D3D11(levelWidth);
    unsigned numRows = (unsigned)(IsCompressed_D3D11() ? (levelHeight + 3) >> 2 : levelHeight);

    hr = graphics_->GetImpl_D3D11()->GetDeviceContext()->Map((ID3D11Resource*)stagingTexture, 0, D3D11_MAP_READ, 0, &mappedData);
    if (FAILED(hr) || !mappedData.pData)
    {
        DV_LOGD3DERROR("Failed to map staging texture for GetData", hr);
        DV_SAFE_RELEASE(stagingTexture);
        return false;
    }
    else
    {
        for (int page = 0; page < levelDepth; ++page)
        {
            for (unsigned row = 0; row < numRows; ++row)
            {
                memcpy((unsigned char*)dest + (page * numRows + row) * rowSize,
                    (unsigned char*)mappedData.pData + page * mappedData.DepthPitch + row * mappedData.RowPitch, rowSize);
            }
        }
        graphics_->GetImpl_D3D11()->GetDeviceContext()->Unmap((ID3D11Resource*)stagingTexture, 0);
        DV_SAFE_RELEASE(stagingTexture);
        return true;
    }
}

bool Texture3D::Create_D3D11()
{
    Release_D3D11();

    if (!graphics_ || !width_ || !height_ || !depth_)
        return false;

    levels_ = CheckMaxLevels(width_, height_, depth_, requestedLevels_);

    D3D11_TEXTURE3D_DESC textureDesc;
    memset(&textureDesc, 0, sizeof textureDesc);
    textureDesc.Width = (UINT)width_;
    textureDesc.Height = (UINT)height_;
    textureDesc.Depth = (UINT)depth_;
    textureDesc.MipLevels = usage_ != TEXTURE_DYNAMIC ? levels_ : 1;
    textureDesc.Format = (DXGI_FORMAT)(sRGB_ ? GetSRGBFormat_D3D11(format_) : format_);
    textureDesc.Usage = usage_ == TEXTURE_DYNAMIC ? D3D11_USAGE_DYNAMIC : D3D11_USAGE_DEFAULT;
    textureDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
    textureDesc.CPUAccessFlags = usage_ == TEXTURE_DYNAMIC ? D3D11_CPU_ACCESS_WRITE : 0;

    HRESULT hr = graphics_->GetImpl_D3D11()->GetDevice()->CreateTexture3D(&textureDesc, nullptr, (ID3D11Texture3D**)&object_.ptr_);
    if (FAILED(hr))
    {
        DV_LOGD3DERROR("Failed to create texture", hr);
        DV_SAFE_RELEASE(object_.ptr_);
        return false;
    }

    D3D11_SHADER_RESOURCE_VIEW_DESC resourceViewDesc;
    memset(&resourceViewDesc, 0, sizeof resourceViewDesc);
    resourceViewDesc.Format = (DXGI_FORMAT)GetSRVFormat_D3D11(textureDesc.Format);
    resourceViewDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE3D;
    resourceViewDesc.Texture3D.MipLevels = usage_ != TEXTURE_DYNAMIC ? (UINT)levels_ : 1;

    hr = graphics_->GetImpl_D3D11()->GetDevice()->CreateShaderResourceView((ID3D11Resource*)object_.ptr_, &resourceViewDesc,
        (ID3D11ShaderResourceView**)&shaderResourceView_);
    if (FAILED(hr))
    {
        DV_LOGD3DERROR("Failed to create shader resource view for texture", hr);
        DV_SAFE_RELEASE(shaderResourceView_);
        return false;
    }

    return true;
}

}
