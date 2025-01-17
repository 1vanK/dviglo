// Copyright (c) 2008-2023 the Urho3D project
// Copyright (c) 2022-2023 the Dviglo project
// License: MIT

#include "camera.h"
#include "graphics.h"
#include "renderer.h"
#include "render_path.h"
#include "view.h"
#include "../resource/resource_cache.h"
#include "../resource/xml_file.h"
#include "../scene/scene.h"

#include "../common/debug_new.h"

namespace dviglo
{

Viewport::Viewport() :
    rect_(IntRect::ZERO),
    drawDebug_(true)
{
    SetRenderPath((RenderPath*)nullptr);
}

Viewport::Viewport(Scene* scene, Camera* camera, RenderPath* renderPath) :
    scene_(scene),
    camera_(camera),
    rect_(IntRect::ZERO),
    drawDebug_(true)
{
    SetRenderPath(renderPath);
}

Viewport::Viewport(Scene* scene, Camera* camera, const IntRect& rect, RenderPath* renderPath) :   // NOLINT(modernize-pass-by-value)
    scene_(scene),
    camera_(camera),
    rect_(rect),
    drawDebug_(true)
{
    SetRenderPath(renderPath);
}

Viewport::~Viewport() = default;

void Viewport::SetScene(Scene* scene)
{
    scene_ = scene;
}

void Viewport::SetCamera(Camera* camera)
{
    camera_ = camera;
}

void Viewport::SetCullCamera(Camera* camera)
{
    cullCamera_ = camera;
}

void Viewport::SetRect(const IntRect& rect)
{
    rect_ = rect;
}

void Viewport::SetDrawDebug(bool enable)
{
    drawDebug_ = enable;
}

void Viewport::SetRenderPath(RenderPath* renderPath)
{
    if (renderPath)
        renderPath_ = renderPath;
    else
    {
        auto* renderer = GetSubsystem<Renderer>();
        if (renderer)
            renderPath_ = renderer->GetDefaultRenderPath();
    }
}

bool Viewport::SetRenderPath(XMLFile* file)
{
    SharedPtr<RenderPath> newRenderPath(new RenderPath());
    if (newRenderPath->Load(file))
    {
        renderPath_ = newRenderPath;
        return true;
    }
    return false;
}

Scene* Viewport::GetScene() const
{
    return scene_;
}

Camera* Viewport::GetCamera() const
{
    return camera_;
}

Camera* Viewport::GetCullCamera() const
{
    return cullCamera_;
}

View* Viewport::GetView() const
{
    return view_;
}

RenderPath* Viewport::GetRenderPath() const
{
    return renderPath_;
}

Ray Viewport::GetScreenRay(int x, int y) const
{
    if (!camera_)
        return Ray();

    float screenX;
    float screenY;

    if (rect_ == IntRect::ZERO)
    {
        auto* graphics = GetSubsystem<Graphics>();
        screenX = (float)x / (float)graphics->GetWidth();
        screenY = (float)y / (float)graphics->GetHeight();
    }
    else
    {
        screenX = float(x - rect_.left_) / (float)rect_.Width();
        screenY = float(y - rect_.top_) / (float)rect_.Height();
    }

    return camera_->GetScreenRay(screenX, screenY);
}

IntVector2 Viewport::WorldToScreenPoint(const Vector3& worldPos) const
{
    if (!camera_)
        return IntVector2::ZERO;

    Vector2 screenPoint = camera_->WorldToScreenPoint(worldPos);

    int x;
    int y;
    if (rect_ == IntRect::ZERO)
    {
        /// \todo This is incorrect if the viewport is used on a texture rendertarget instead of the backbuffer, as it may have different dimensions.
        auto* graphics = GetSubsystem<Graphics>();
        x = (int)(screenPoint.x_ * graphics->GetWidth());
        y = (int)(screenPoint.y_ * graphics->GetHeight());
    }
    else
    {
        x = (int)(rect_.left_ + screenPoint.x_ * rect_.Width());
        y = (int)(rect_.top_ + screenPoint.y_ * rect_.Height());
    }

    return IntVector2(x, y);
}

Vector3 Viewport::ScreenToWorldPoint(int x, int y, float depth) const
{
    if (!camera_)
        return Vector3::ZERO;

    float screenX;
    float screenY;

    if (rect_ == IntRect::ZERO)
    {
        /// \todo This is incorrect if the viewport is used on a texture rendertarget instead of the backbuffer, as it may have different dimensions.
        auto* graphics = GetSubsystem<Graphics>();
        screenX = (float)x / (float)graphics->GetWidth();
        screenY = (float)y / (float)graphics->GetHeight();
    }
    else
    {
        screenX = float(x - rect_.left_) / (float)rect_.Width();
        screenY = float(y - rect_.top_) / (float)rect_.Height();
    }

    return camera_->ScreenToWorldPoint(Vector3(screenX, screenY, depth));
}

void Viewport::AllocateView()
{
    view_ = new View();
}

}
