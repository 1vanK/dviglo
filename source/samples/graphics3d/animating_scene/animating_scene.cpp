// Copyright (c) 2008-2023 the Urho3D project
// Copyright (c) 2022-2023 the Dviglo project
// License: MIT

#include <dviglo/core/core_events.h>
#include <dviglo/engine/engine.h>
#include <dviglo/graphics/camera.h>
#include <dviglo/graphics/graphics.h>
#include <dviglo/graphics/material.h>
#include <dviglo/graphics/model.h>
#include <dviglo/graphics/octree.h>
#include <dviglo/graphics/renderer.h>
#include <dviglo/graphics/static_model.h>
#include <dviglo/graphics/zone.h>
#include <dviglo/input/input.h>
#include <dviglo/resource/resource_cache.h>
#include <dviglo/scene/scene.h>
#include <dviglo/ui/font.h>
#include <dviglo/ui/text.h>
#include <dviglo/ui/ui.h>

#include "animating_scene.h"
#include "rotator.h"

#include <dviglo/common/debug_new.h>

DV_DEFINE_APPLICATION_MAIN(AnimatingScene)

AnimatingScene::AnimatingScene()
{
    // Register an object factory for our custom Rotator component so that we can create them to scene nodes
    DV_CONTEXT.RegisterFactory<Rotator>();
}

void AnimatingScene::Start()
{
    // Execute base class startup
    Sample::Start();

    // Create the scene content
    CreateScene();

    // Create the UI content
    CreateInstructions();

    // Setup the viewport for displaying the scene
    SetupViewport();

    // Hook up to the frame update events
    SubscribeToEvents();

    // Set the mouse mode to use in the sample
    Sample::InitMouseMode(MM_RELATIVE);
}

void AnimatingScene::CreateScene()
{
    auto* cache = GetSubsystem<ResourceCache>();

    scene_ = new Scene();

    // Create the Octree component to the scene so that drawable objects can be rendered. Use default volume
    // (-1000, -1000, -1000) to (1000, 1000, 1000)
    scene_->CreateComponent<Octree>();

    // Create a Zone component into a child scene node. The Zone controls ambient lighting and fog settings. Like the Octree,
    // it also defines its volume with a bounding box, but can be rotated (so it does not need to be aligned to the world X, Y
    // and Z axes.) Drawable objects "pick up" the zone they belong to and use it when rendering; several zones can exist
    Node* zoneNode = scene_->CreateChild("Zone");
    auto* zone = zoneNode->CreateComponent<Zone>();
    // Set same volume as the Octree, set a close bluish fog and some ambient light
    zone->SetBoundingBox(BoundingBox(-1000.0f, 1000.0f));
    zone->SetAmbientColor(Color(0.05f, 0.1f, 0.15f));
    zone->SetFogColor(Color(0.1f, 0.2f, 0.3f));
    zone->SetFogStart(10.0f);
    zone->SetFogEnd(100.0f);

    // Create randomly positioned and oriented box StaticModels in the scene
    const unsigned NUM_OBJECTS = 2000;
    for (unsigned i = 0; i < NUM_OBJECTS; ++i)
    {
        Node* boxNode = scene_->CreateChild("Box");
        boxNode->SetPosition(Vector3(Random(200.0f) - 100.0f, Random(200.0f) - 100.0f, Random(200.0f) - 100.0f));
        // Orient using random pitch, yaw and roll Euler angles
        boxNode->SetRotation(Quaternion(Random(360.0f), Random(360.0f), Random(360.0f)));
        auto* boxObject = boxNode->CreateComponent<StaticModel>();
        boxObject->SetModel(cache->GetResource<Model>("Models/Box.mdl"));
        boxObject->SetMaterial(cache->GetResource<Material>("Materials/Stone.xml"));

        // Add our custom Rotator component which will rotate the scene node each frame, when the scene sends its update event.
        // The Rotator component derives from the base class LogicComponent, which has convenience functionality to subscribe
        // to the various update events, and forward them to virtual functions that can be implemented by subclasses. This way
        // writing logic/update components in C++ becomes similar to scripting.
        // Now we simply set same rotation speed for all objects
        auto* rotator = boxNode->CreateComponent<Rotator>();
        rotator->SetRotationSpeed(Vector3(10.0f, 20.0f, 30.0f));
    }

    // Create the camera. Let the starting position be at the world origin. As the fog limits maximum visible distance, we can
    // bring the far clip plane closer for more effective culling of distant objects
    cameraNode_ = scene_->CreateChild("Camera");
    auto* camera = cameraNode_->CreateComponent<Camera>();
    camera->SetFarClip(100.0f);

    // Create a point light to the camera scene node
    auto* light = cameraNode_->CreateComponent<Light>();
    light->SetLightType(LIGHT_POINT);
    light->SetRange(30.0f);
}

void AnimatingScene::CreateInstructions()
{
    auto* cache = GetSubsystem<ResourceCache>();
    auto* ui = GetSubsystem<UI>();

    // Construct new Text object, set string to display and font to use
    auto* instructionText = ui->GetRoot()->CreateChild<Text>();
    instructionText->SetText("Use WASD keys and mouse/touch to move");
    instructionText->SetFont(cache->GetResource<Font>("Fonts/Anonymous Pro.ttf"), 15);

    // Position the text relative to the screen center
    instructionText->SetHorizontalAlignment(HA_CENTER);
    instructionText->SetVerticalAlignment(VA_CENTER);
    instructionText->SetPosition(0, ui->GetRoot()->GetHeight() / 4);
}

void AnimatingScene::SetupViewport()
{
    auto* renderer = GetSubsystem<Renderer>();

    // Set up a viewport to the Renderer subsystem so that the 3D scene can be seen
    SharedPtr<Viewport> viewport(new Viewport(scene_, cameraNode_->GetComponent<Camera>()));
    renderer->SetViewport(0, viewport);
}

void AnimatingScene::SubscribeToEvents()
{
    // Subscribe HandleUpdate() function for processing update events
    SubscribeToEvent(E_UPDATE, DV_HANDLER(AnimatingScene, HandleUpdate));
}

void AnimatingScene::MoveCamera(float timeStep)
{
    // Do not move if the UI has a focused element (the console)
    if (GetSubsystem<UI>()->GetFocusElement())
        return;

    auto* input = GetSubsystem<Input>();

    // Movement speed as world units per second
    const float MOVE_SPEED = 20.0f;
    // Mouse sensitivity as degrees per pixel
    const float MOUSE_SENSITIVITY = 0.1f;

    // Use this frame's mouse motion to adjust camera node yaw and pitch. Clamp the pitch between -90 and 90 degrees
    IntVector2 mouseMove = input->GetMouseMove();
    yaw_ += MOUSE_SENSITIVITY * mouseMove.x_;
    pitch_ += MOUSE_SENSITIVITY * mouseMove.y_;
    pitch_ = Clamp(pitch_, -90.0f, 90.0f);

    // Construct new orientation for the camera scene node from yaw and pitch. Roll is fixed to zero
    cameraNode_->SetRotation(Quaternion(pitch_, yaw_, 0.0f));

    // Read WASD keys and move the camera scene node to the corresponding direction if they are pressed
    if (input->GetKeyDown(KEY_W))
        cameraNode_->Translate(Vector3::FORWARD * MOVE_SPEED * timeStep);
    if (input->GetKeyDown(KEY_S))
        cameraNode_->Translate(Vector3::BACK * MOVE_SPEED * timeStep);
    if (input->GetKeyDown(KEY_A))
        cameraNode_->Translate(Vector3::LEFT * MOVE_SPEED * timeStep);
    if (input->GetKeyDown(KEY_D))
        cameraNode_->Translate(Vector3::RIGHT * MOVE_SPEED * timeStep);
}

void AnimatingScene::HandleUpdate(StringHash eventType, VariantMap& eventData)
{
    using namespace Update;

    // Take the frame time step, which is stored as a float
    float timeStep = eventData[P_TIMESTEP].GetFloat();

    // Move the camera, scale movement with time step
    MoveCamera(timeStep);
}
