// Copyright (c) 2008-2023 the Urho3D project
// Copyright (c) 2022-2023 the Dviglo project
// License: MIT

#include <dviglo/core/core_events.h>
#include <dviglo/core/process_utils.h>
#include <dviglo/input/input.h>
#include <dviglo/ui/font.h>
#include <dviglo/ui/text.h>
#include <dviglo/ui/ui.h>

#include "clicker.h"

#include <dviglo/common/debug_new.h>

// Expands to this example's entry-point
DV_DEFINE_APPLICATION_MAIN(Clicker)

Clicker::Clicker()
{
}

void Clicker::Start()
{
    Sample::Start();
    CreateUI();
    Sample::InitMouseMode(MM_FREE);
    SubscribeToEvents();
}

void Clicker::CreateUI()
{
    ResourceCache* cache = GetSubsystem<ResourceCache>();
    XMLFile* style = cache->GetResource<XMLFile>("UI/DefaultStyle.xml");
    UIElement* uiRoot = GetSubsystem<UI>()->GetRoot();
    uiRoot->SetDefaultStyle(style);

    // Text in the center of the screen will initially contain hint, and then score
    Text* scoreText = uiRoot->CreateChild<Text>("Score");
    scoreText->SetText("Hold LMB to play.\nClick RMB to upgrade power.");
    scoreText->SetFont(cache->GetResource<Font>("Fonts/Anonymous Pro.ttf"), 30);
    scoreText->SetColor(Color::GREEN);
    scoreText->SetHorizontalAlignment(HA_CENTER);
    scoreText->SetVerticalAlignment(VA_CENTER);

    Text* powerText = uiRoot->CreateChild<Text>("Power");
    powerText->SetText("Power: " + power_.ToString());
    powerText->SetFont(cache->GetResource<Font>("Fonts/Anonymous Pro.ttf"), 30);
    powerText->SetColor(Color::WHITE);
    powerText->SetPosition({10, 10});
}

void Clicker::SubscribeToEvents()
{
    SubscribeToEvent(E_UPDATE, DV_HANDLER(Clicker, HandleUpdate));
    SubscribeToEvent(E_MOUSEBUTTONDOWN, DV_HANDLER(Clicker, HandleMouseButtonDown));
}

static String ShortNumberRepresentation(const BigInt& value)
{
    String str = value.ToString();

    u32 len = str.Length();

    if (len > 45)
        return str.Substring(0, len - 45) + " quattuordecillion";

    if (len > 42)
        return str.Substring(0, len - 42) + " tredecillion";

    if (len > 39)
        return str.Substring(0, len - 39) + " duodecillion";

    if (len > 36)
        return str.Substring(0, len - 36) + " undecillion";

    if (len > 33)
        return str.Substring(0, len - 33) + " decillion";

    if (len > 30)
        return str.Substring(0, len - 30) + " nonillion";

    if (len > 27)
        return str.Substring(0, len - 27) + " octillion";

    if (len > 24)
        return str.Substring(0, len - 24) + " septillion";

    if (len > 21)
        return str.Substring(0, len - 21) + " sextillion";

    if (len > 18)
        return str.Substring(0, len - 18) + " quintillion";

    if (len > 15)
        return str.Substring(0, len - 15) + " quadrillion";

    if (len > 12)
        return str.Substring(0, len - 12) + " trillion";

    if (len > 9)
        return str.Substring(0, len - 9) + " billion";

    if (len > 6)
        return str.Substring(0, len - 6) + " million";

    if (len > 3)
        return str.Substring(0, len - 3) + " thousand";

    return str;
}

void Clicker::HandleUpdate(StringHash eventType, VariantMap& eventData)
{
    using namespace Update;

    float timeStep = eventData[P_TIMESTEP].GetFloat();

    if (clickDelay_ > 0.f)
        clickDelay_ -= timeStep;

    Input* input = GetSubsystem<Input>();

    if (input->GetMouseButtonDown(MOUSEB_LEFT) && clickDelay_ <= 0.f)
    {
        score_ += power_;

        UIElement* uiRoot = GetSubsystem<UI>()->GetRoot();
        Text* scoreText = static_cast<Text*>(uiRoot->GetChild("Score", false));
        scoreText->SetText(ShortNumberRepresentation(score_));

        clickDelay_ = 0.2f;
    }
}

void Clicker::HandleMouseButtonDown(StringHash eventType, VariantMap& eventData)
{
    using namespace MouseButtonDown;

    MouseButton button = (MouseButton)eventData[P_BUTTON].GetU32();
    
    if (button == MOUSEB_RIGHT)
    {
        power_ *= 2;

        UIElement* uiRoot = GetSubsystem<UI>()->GetRoot();
        Text* powerText = static_cast<Text*>(uiRoot->GetChild("Power", false));
        powerText->SetText("Power: " + ShortNumberRepresentation(power_));
    }
}
