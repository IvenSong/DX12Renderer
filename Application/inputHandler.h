#pragma once

#include <cstdint>
#include "inputKey.h"

class InputHandler {
public:
    virtual ~InputHandler() = default;

    virtual void onKeyDown(const uint32_t& key) = 0;
    virtual void onKeyUp(const uint32_t& key) = 0;
    virtual void onRMouseDown(const int& x, const int& y) = 0;
    virtual void onRMouseUp(const int& x, const int& y) = 0;
    virtual void onMouseMove(const int& x, const int& y) = 0;
};
