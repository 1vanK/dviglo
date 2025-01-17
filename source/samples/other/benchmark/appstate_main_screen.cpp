// Copyright (c) 2008-2023 the Urho3D project
// Copyright (c) 2022-2023 the Dviglo project
// License: MIT

#include "appstate_main_screen.h"
#include "app_state_manager.h"

#include <dviglo/input/input.h>
#include <dviglo/scene/scene_events.h>
#include <dviglo/ui/button.h>
#include <dviglo/ui/text.h>
#include <dviglo/ui/ui.h>
#include <dviglo/ui/ui_events.h>
#include <dviglo/ui/window.h>

#include <dviglo/common/debug_new.h>

using namespace dviglo;

static const String MAIN_SCREEN_WINDOW_STR = "Main Screen Window";
static const String BENCHMARK_01_STR = "Benchmark 01";
static const String BENCHMARK_02_STR = "Benchmark 02";
static const String BENCHMARK_03_STR = "Benchmark 03";
static const String BENCHMARK_04_STR = "Benchmark 04";

void AppState_MainScreen::HandleButtonPressed(StringHash eventType, VariantMap& eventData)
{
    Button* pressedButton = static_cast<Button*>(eventData["Element"].GetPtr());
    AppStateManager* appStateManager = GetSubsystem<AppStateManager>();
    
    if (pressedButton->GetName() == BENCHMARK_01_STR)
        appStateManager->SetRequiredAppStateId(APPSTATEID_BENCHMARK01);
    else if (pressedButton->GetName() == BENCHMARK_02_STR)
        appStateManager->SetRequiredAppStateId(APPSTATEID_BENCHMARK02);
    else if (pressedButton->GetName() == BENCHMARK_03_STR)
        appStateManager->SetRequiredAppStateId(APPSTATEID_BENCHMARK03);
    else if (pressedButton->GetName() == BENCHMARK_04_STR)
        appStateManager->SetRequiredAppStateId(APPSTATEID_BENCHMARK04);
}

void AppState_MainScreen::CreateButton(const String& name, const String& text, Window& parent)
{
    Button* button = parent.CreateChild<Button>(name);
    button->SetStyleAuto();
    button->SetFixedHeight(24);

    Text* buttonText = button->CreateChild<Text>();
    buttonText->SetStyleAuto();
    buttonText->SetText(text);
    buttonText->SetAlignment(HA_CENTER, VA_CENTER);

    SubscribeToEvent(button, E_RELEASED, DV_HANDLER(AppState_MainScreen, HandleButtonPressed));
}

void AppState_MainScreen::CreateGui()
{
    UIElement* root = GetSubsystem<UI>()->GetRoot();

    Window* window = root->CreateChild<Window>(MAIN_SCREEN_WINDOW_STR);
    window->SetStyleAuto();
    window->SetMinWidth(384);
    window->SetLayout(LM_VERTICAL, 6, IntRect(6, 6, 6, 6));
    window->SetPosition(10, 34);

    Text* windowTitle = window->CreateChild<Text>();
    windowTitle->SetStyleAuto();
    windowTitle->SetText("Benchmark list");

    AppStateManager* appStateManager = GetSubsystem<AppStateManager>();

    CreateButton(BENCHMARK_01_STR, appStateManager->GetName(APPSTATEID_BENCHMARK01), *window);
    CreateButton(BENCHMARK_02_STR, appStateManager->GetName(APPSTATEID_BENCHMARK02), *window);
    CreateButton(BENCHMARK_03_STR, appStateManager->GetName(APPSTATEID_BENCHMARK03), *window);
    CreateButton(BENCHMARK_04_STR, appStateManager->GetName(APPSTATEID_BENCHMARK04), *window);
}

void AppState_MainScreen::DestroyGui()
{
    UIElement* root = GetSubsystem<UI>()->GetRoot();
    Window* window = root->GetChildStaticCast<Window>(MAIN_SCREEN_WINDOW_STR);
    window->Remove();
}

void AppState_MainScreen::OnEnter()
{
    assert(!scene_);
    LoadSceneXml("99_Benchmark/Scenes/MainScreen.xml");

    CreateGui();
    SetupViewport();
    GetSubsystem<Input>()->SetMouseVisible(true);
    SubscribeToEvent(scene_, E_SCENEUPDATE, DV_HANDLER(AppState_MainScreen, HandleSceneUpdate));
    fpsCounter_.Clear();
}

void AppState_MainScreen::OnLeave()
{
    DestroyViewport();
    DestroyGui();
    scene_ = nullptr;
}

void AppState_MainScreen::HandleSceneUpdate(StringHash eventType, VariantMap& eventData)
{
    float timeStep = eventData[SceneUpdate::P_TIMESTEP].GetFloat();

    fpsCounter_.Update(timeStep);
    UpdateCurrentFpsElement();
}
