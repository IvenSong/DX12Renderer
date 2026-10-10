#include "mathLib.h"

namespace Math {

double radians(double angleDegrees) noexcept {
    return angleDegrees * pi / 180.0;
}

double degrees(double angleRadians) noexcept {
    return angleRadians * 180.0 / pi;
}

float lerp(float a, float b, float c, float wa, float wb, float wc) noexcept {
    return a * wa + b * wb + c * wc;
}

} // namespace Math
