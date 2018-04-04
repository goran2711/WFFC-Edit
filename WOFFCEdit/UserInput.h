#pragma once
#include <Keyboard.h>
#include <Mouse.h>

struct UserInput
{
    DirectX::Keyboard::State keyboard;
    DirectX::Mouse::ButtonStateTracker mouse;
};