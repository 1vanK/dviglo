// Copyright (c) 2008-2023 the Urho3D project
// Copyright (c) 2022-2023 the Dviglo project
// License: MIT

#include "appstate_benchmark04.h"
#include "app_state_manager.h"

#include <dviglo/core/timer.h>
#include <dviglo/graphics/graphics_events.h>
#include <dviglo/graphics_api/texture_2d.h>
#include <dviglo/input/input.h>
#include <dviglo/resource/resource_cache.h>

#include <dviglo/common/debug_new.h>

using namespace dviglo;

void AppState_Benchmark04::OnEnter()
{
    assert(!scene_);

    // Сцена и вьюпорт не нужны

    GetSubsystem<Input>()->SetMouseVisible(false);
    SubscribeToEvent(E_ENDALLVIEWSRENDER, DV_HANDLER(AppState_Benchmark04, HandleEndAllViewsRender));
    fpsCounter_.Clear();
    spriteBatch_ = new SpriteBatch();
}

void AppState_Benchmark04::OnLeave()
{
    UnsubscribeFromAllEvents();
    spriteBatch_ = nullptr;
}

void AppState_Benchmark04::HandleEndAllViewsRender(StringHash eventType, VariantMap& eventData)
{
    float timeStep = GetSubsystem<Time>()->GetTimeStep();

    fpsCounter_.Update(timeStep);
    UpdateCurrentFpsElement();

    if (GetSubsystem<Input>()->GetKeyDown(KEY_ESCAPE))
    {
        GetSubsystem<AppStateManager>()->SetRequiredAppStateId(APPSTATEID_MAINSCREEN);
        return;
    }

    if (fpsCounter_.GetTotalTime() >= 25.f)
    {
        GetSubsystem<AppStateManager>()->SetRequiredAppStateId(APPSTATEID_RESULTSCREEN);
        return;
    }

    angle_ += timeStep * 100.0f;
    angle_ = fmod(angle_, 360.0f);

    scale_ += timeStep;

    Graphics* graphics = GetSubsystem<Graphics>();
    ResourceCache* cache = GetSubsystem<ResourceCache>();
    Texture2D* ball = GetSubsystem<ResourceCache>()->GetResource<Texture2D>("Urho2D/Ball.png");
    Texture2D* head = cache->GetResource<Texture2D>("Textures/FishBoneLogo.png");

    GetSubsystem<Graphics>()->Clear(CLEAR_COLOR, Color::GREEN);

    for (int i = 0; i < 20000; i++)
    {
        spriteBatch_->DrawSprite(ball,
            Vector2(Random(0.0f, (float)(graphics->GetWidth() - ball->GetWidth())), Random(0.0f, (float)graphics->GetHeight() - ball->GetHeight())), nullptr, 0xFFFFFFFF);
    }

    spriteBatch_->DrawSprite(head, Vector2(200.0f, 200.0f), nullptr, 0xFFFFFFFF, 0.0f, Vector2::ZERO, Vector2::ONE, FlipModes::Both);

    float scale = cos(scale_) + 1.0f; // cos возвращает значения в диапазоне [-1, 1], значит scale будет в диапазоне [0, 2].
    Vector2 origin = Vector2(head->GetWidth() * 0.5f, head->GetHeight() * 0.5f);
    spriteBatch_->DrawSprite(head, Vector2(400.0f, 300.0f), nullptr, 0xFFFFFFFF, angle_, origin, Vector2(scale, scale));

    spriteBatch_->DrawString("Отзеркаленный текст", cache->GetResource<Font>("Fonts/Anonymous Pro.ttf"), 40.0f,
        Vector2(250.0f, 200.0f), 0xFF0000FF, 0.0f, Vector2::ZERO, Vector2::ONE, FlipModes::Both);

    spriteBatch_->DrawString("Некий текст", cache->GetResource<Font>("Fonts/Anonymous Pro.ttf"), 40.0f,
        Vector2(400.0f, 300.0f), 0xFFFF0000, angle_, Vector2::ZERO, Vector2(scale, scale));

    spriteBatch_->Flush();
}
