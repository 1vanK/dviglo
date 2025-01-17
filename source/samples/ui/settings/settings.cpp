// Copyright (c) 2008-2023 the Urho3D project
// Copyright (c) 2022-2023 the Dviglo project
// License: MIT

#include <dviglo/core/core_events.h>
#include <dviglo/engine/engine.h>
#include <dviglo/graphics/graphics.h>
#include <dviglo/graphics/graphics_events.h>
#include <dviglo/graphics/material.h>
#include <dviglo/graphics/model.h>
#include <dviglo/graphics/octree.h>
#include <dviglo/graphics/static_model.h>
#include <dviglo/graphics/zone.h>
#include <dviglo/graphics_api/texture_2d.h>
#include <dviglo/input/input.h>
#include <dviglo/resource/resource_cache.h>
#include <dviglo/ui/button.h>
#include <dviglo/ui/check_box.h>
#include <dviglo/ui/dropdown_list.h>
#include <dviglo/ui/line_edit.h>
#include <dviglo/ui/text.h>
#include <dviglo/ui/tooltip.h>
#include <dviglo/ui/ui.h>
#include <dviglo/ui/ui_events.h>
#include <dviglo/ui/window.h>

#include "settings.h"

#include <dviglo/common/debug_new.h>

DV_DEFINE_APPLICATION_MAIN(WindowSettingsDemo)

WindowSettingsDemo::WindowSettingsDemo()
    : uiRoot_(GetSubsystem<UI>()->GetRoot())
{
}

void WindowSettingsDemo::Start()
{
    // Execute base class startup
    Sample::Start();

    // Enable OS cursor
    GetSubsystem<Input>()->SetMouseVisible(true);

    // Load XML file containing default UI style sheet
    auto* cache = GetSubsystem<ResourceCache>();
    auto* style = cache->GetResource<XMLFile>("UI/DefaultStyle.xml");

    // Set the loaded style as default style
    uiRoot_->SetDefaultStyle(style);

    // Create window with settings.
    InitSettings();
    SynchronizeSettings();
    SubscribeToEvent(E_SCREENMODE,
        [this](StringHash /*eventType*/, const VariantMap& /*eventData*/)
    {
        SynchronizeSettings();
    });

    // Set the mouse mode to use in the sample
    Sample::InitMouseMode(MM_FREE);

    // Create scene
    CreateScene();

    // Setup viewport
    auto* renderer = GetSubsystem<Renderer>();
    SharedPtr<Viewport> viewport(new Viewport(scene_, cameraNode_->GetComponent<Camera>()));
    renderer->SetViewport(0, viewport);
}

void WindowSettingsDemo::CreateScene()
{
    auto* cache = GetSubsystem<ResourceCache>();

    scene_ = new Scene();
    scene_->CreateComponent<Octree>();

    auto* zone = scene_->CreateComponent<Zone>();
    zone->SetAmbientColor(Color::WHITE);

    // Create 3D object
    Node* objectNode = scene_->CreateChild("Object");
    objectNode->SetRotation(Quaternion(45.0f, 45.0f, 45.0f));
    auto* objectModel = objectNode->CreateComponent<StaticModel>();
    objectModel->SetModel(cache->GetResource<Model>("Models/Box.mdl"));
    objectModel->SetMaterial(cache->GetResource<Material>("Materials/Stone.xml"));

    // Create camera
    cameraNode_ = scene_->CreateChild("Camera");
    cameraNode_->CreateComponent<Camera>();
    cameraNode_->SetPosition(Vector3(0.0f, 0.0f, -4.0f));

    // Rotate object
    SubscribeToEvent(scene_, E_SCENEUPDATE,
        [objectNode](StringHash /*eventType*/, VariantMap& eventData)
    {
        const float timeStep = eventData[SceneUpdate::P_TIMESTEP].GetFloat();
        objectNode->Rotate(Quaternion(0.0f, 20.0f * timeStep, 0.0f), TransformSpace::World);
    });
}

void WindowSettingsDemo::InitSettings()
{
    auto* graphics = GetSubsystem<Graphics>();

    // Create the Window and add it to the UI's root node
    window_ = uiRoot_->CreateChild<Window>("Window");

    // Set Window size and layout settings
    window_->SetPosition(128, 128);
    window_->SetMinWidth(300);
    window_->SetLayout(LM_VERTICAL, 6, IntRect(6, 6, 6, 6));
    window_->SetMovable(true);
    window_->SetStyleAuto();

    // Create the Window title Text
    auto* windowTitle = window_->CreateChild<Text>("WindowTitle");
    windowTitle->SetText("Window Settings");
    windowTitle->SetStyleAuto();

    // Create monitor selector
    monitorControl_ = window_->CreateChild<DropDownList>("Monitor");
    monitorControl_->SetMinHeight(24);
    monitorControl_->SetStyleAuto();
    for (int i = 0; i < graphics->GetMonitorCount(); ++i)
    {
        auto text = MakeShared<Text>();
        text->SetText(ToString("Monitor %d", i));
        text->SetMinWidth(CeilToInt(text->GetRowWidth(0) + 10));
        monitorControl_->AddItem(text);
        text->SetStyleAuto();
    }

    // Create resolution selector
    resolutionControl_ = window_->CreateChild<DropDownList>("Resolution");
    resolutionControl_->SetMinHeight(24);
    resolutionControl_->SetStyleAuto();

    auto resolutionPlaceholder = MakeShared<Text>();
    resolutionPlaceholder->SetText("[Cannot fill list of resolutions]");
    resolutionPlaceholder->SetMinWidth(CeilToInt(resolutionPlaceholder->GetRowWidth(0) + 10));
    resolutionControl_->AddItem(resolutionPlaceholder);
    resolutionPlaceholder->SetStyleAuto();

    // Create fullscreen controller
    auto* fullscreenFrame = window_->CreateChild<UIElement>("Fullscreen Frame");
    fullscreenFrame->SetMinHeight(24);
    fullscreenFrame->SetLayout(LM_HORIZONTAL, 6);

    fullscreenControl_ = fullscreenFrame->CreateChild<CheckBox>("Fullscreen Control");
    fullscreenControl_->SetStyleAuto();

    auto* fullscreenText = fullscreenFrame->CreateChild<Text>("Fullscreen Label");
    fullscreenText->SetText("Fullscreen");
    fullscreenText->SetMinWidth(CeilToInt(fullscreenText->GetRowWidth(0) + 10));
    fullscreenText->SetStyleAuto();

    // Create borderless controller
    auto* borderlessFrame = window_->CreateChild<UIElement>("Borderless Frame");
    borderlessFrame->SetMinHeight(24);
    borderlessFrame->SetLayout(LM_HORIZONTAL, 6);

    borderlessControl_ = borderlessFrame->CreateChild<CheckBox>("Borderless Control");
    borderlessControl_->SetStyleAuto();

    auto* borderlessText = borderlessFrame->CreateChild<Text>("Borderless Label");
    borderlessText->SetText("Borderless");
    borderlessText->SetMinWidth(CeilToInt(borderlessText->GetRowWidth(0) + 10));
    borderlessText->SetStyleAuto();

    // Create resizable controller
    auto* resizableFrame = window_->CreateChild<UIElement>("Resizable Frame");
    resizableFrame->SetMinHeight(24);
    resizableFrame->SetLayout(LM_HORIZONTAL, 6);

    resizableControl_ = resizableFrame->CreateChild<CheckBox>("Resizable Control");
    resizableControl_->SetStyleAuto();

    auto* resizableText = resizableFrame->CreateChild<Text>("Resizable Label");
    resizableText->SetText("Resizable");
    resizableText->SetMinWidth(CeilToInt(resizableText->GetRowWidth(0) + 10));
    resizableText->SetStyleAuto();

    // Create resizable controller
    auto* vsyncFrame = window_->CreateChild<UIElement>("V-Sync Frame");
    vsyncFrame->SetMinHeight(24);
    vsyncFrame->SetLayout(LM_HORIZONTAL, 6);

    vsyncControl_ = vsyncFrame->CreateChild<CheckBox>("V-Sync Control");
    vsyncControl_->SetStyleAuto();

    auto* vsyncText = vsyncFrame->CreateChild<Text>("V-Sync Label");
    vsyncText->SetText("V-Sync");
    vsyncText->SetMinWidth(CeilToInt(vsyncText->GetRowWidth(0) + 10));
    vsyncText->SetStyleAuto();

    // Create multi-sample controller from 1 (= 2^0) to 16 (= 2^4)
    multiSampleControl_ = window_->CreateChild<DropDownList>("Multi-Sample Control");
    multiSampleControl_->SetMinHeight(24);
    multiSampleControl_->SetStyleAuto();
    for (int i = 0; i <= 4; ++i)
    {
        auto text = MakeShared<Text>();
        text->SetText(i == 0 ? "No MSAA" : ToString("MSAA x%d", 1 << i));
        text->SetMinWidth(CeilToInt(text->GetRowWidth(0) + 10));
        multiSampleControl_->AddItem(text);
        text->SetStyleAuto();
    }

    // Create "Apply" button
    auto* applyButton = window_->CreateChild<Button>("Apply");
    applyButton->SetLayout(LM_HORIZONTAL, 6, IntRect(6, 6, 6, 6));
    applyButton->SetStyleAuto();

    auto* applyButtonText = applyButton->CreateChild<Text>("Apply Text");
    applyButtonText->SetAlignment(HA_CENTER, VA_CENTER);
    applyButtonText->SetText("Apply");
    applyButtonText->SetStyleAuto();

    applyButton->SetFixedWidth(CeilToInt(applyButtonText->GetRowWidth(0) + 20));
    applyButton->SetFixedHeight(30);

    // Apply settings when "Apply" button is clicked
    SubscribeToEvent(applyButton, E_RELEASED,
        [this, graphics](StringHash /*eventType*/, const VariantMap& /*eventData*/)
    {
        const unsigned monitor = monitorControl_->GetSelection();
        if (monitor == M_MAX_UNSIGNED)
            return;

        const auto& resolutions = graphics->GetResolutions(monitor);
        const i32 selectedResolution = resolutionControl_->GetSelection();
        if (selectedResolution >= resolutions.Size())
            return;

        const bool fullscreen = fullscreenControl_->IsChecked();
        const bool borderless = borderlessControl_->IsChecked();
        const bool resizable = resizableControl_->IsChecked();
        const bool vsync = vsyncControl_->IsChecked();

        const unsigned multiSampleSelection = multiSampleControl_->GetSelection();
        const int multiSample = multiSampleSelection == M_MAX_UNSIGNED ? 1 : static_cast<int>(1 << multiSampleSelection);

        // TODO: Expose these options too?
        const bool highDPI = graphics->GetHighDPI();
        const bool tripleBuffer = graphics->GetTripleBuffer();

        const int width = resolutions[selectedResolution].x_;
        const int height = resolutions[selectedResolution].y_;
        const int refreshRate = resolutions[selectedResolution].z_;
        graphics->SetMode(width, height, fullscreen, borderless, resizable, highDPI, vsync, tripleBuffer, multiSample, monitor, refreshRate);
    });
}

void WindowSettingsDemo::SynchronizeSettings()
{
    auto* graphics = GetSubsystem<Graphics>();

    // Synchronize monitor
    const unsigned currentMonitor = graphics->GetMonitor();
    monitorControl_->SetSelection(currentMonitor);

    // Synchronize resolution list
    resolutionControl_->RemoveAllItems();
    const auto& resolutions = graphics->GetResolutions(currentMonitor);
    for (const IntVector3& resolution : resolutions)
    {
        auto resolutionEntry = MakeShared<Text>();
        resolutionEntry->SetText(ToString("%dx%d, %d Hz", resolution.x_, resolution.y_, resolution.z_));
        resolutionEntry->SetMinWidth(CeilToInt(resolutionEntry->GetRowWidth(0) + 10));
        resolutionControl_->AddItem(resolutionEntry);
        resolutionEntry->SetStyleAuto();
    }

    // Synchronize selected resolution
    const i32 currentResolution = graphics->FindBestResolutionIndex(currentMonitor,
        graphics->GetWidth(), graphics->GetHeight(), graphics->GetRefreshRate());
    resolutionControl_->SetSelection(currentResolution);

    // Synchronize fullscreen and borderless flags
    fullscreenControl_->SetChecked(graphics->GetFullscreen());
    borderlessControl_->SetChecked(graphics->GetBorderless());
    resizableControl_->SetChecked(graphics->GetResizable());
    vsyncControl_->SetChecked(graphics->GetVSync());

    // Synchronize MSAA
    for (unsigned i = 0; i <= 4; ++i)
    {
        if (graphics->GetMultiSample() == static_cast<int>(1 << i))
            multiSampleControl_->SetSelection(i);
    }
}
