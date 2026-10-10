#pragma once

#include <cassert>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <stdexcept>
#include <type_traits>

namespace Math {

inline constexpr double pi = 3.14159265358979323846;

double radians(double degrees) noexcept;
double degrees(double radians) noexcept;

template <typename T> struct vector2 {
    T x{}, y{};
    constexpr vector2() = default;
    constexpr vector2(T x_, T y_) : x(x_), y(y_) {}

    constexpr T& operator[](std::size_t i) { assert(i < 2); return i == 0 ? x : y; }
    constexpr const T& operator[](std::size_t i) const { assert(i < 2); return i == 0 ? x : y; }
    constexpr vector2 operator+() const { return *this; }
    constexpr vector2 operator-() const { return {-x, -y}; }
    constexpr vector2 operator+(const vector2& v) const { return {x + v.x, y + v.y}; }
    constexpr vector2 operator-(const vector2& v) const { return {x - v.x, y - v.y}; }
    constexpr vector2 operator*(T s) const { return {x * s, y * s}; }
    constexpr vector2 operator*(const vector2& v) const { return {x * v.x, y * v.y}; }
    constexpr vector2 operator/(T s) const { return {x / s, y / s}; }
    constexpr vector2& operator+=(const vector2& v) { x += v.x; y += v.y; return *this; }
    constexpr vector2& operator-=(const vector2& v) { x -= v.x; y -= v.y; return *this; }
    constexpr vector2& operator*=(T s) { x *= s; y *= s; return *this; }
    constexpr vector2& operator*=(const vector2& v) { x *= v.x; y *= v.y; return *this; }
    constexpr vector2& operator/=(T s) { x /= s; y /= s; return *this; }
};

template <typename T> struct vector3 {
    T x{}, y{}, z{};
    constexpr vector3() = default;
    constexpr vector3(T x_, T y_, T z_) : x(x_), y(y_), z(z_) {}

    constexpr T& operator[](std::size_t i) { assert(i < 3); return i == 0 ? x : (i == 1 ? y : z); }
    constexpr const T& operator[](std::size_t i) const { assert(i < 3); return i == 0 ? x : (i == 1 ? y : z); }
    constexpr vector3 operator+() const { return *this; }
    constexpr vector3 operator-() const { return {-x, -y, -z}; }
    constexpr vector3 operator+(const vector3& v) const { return {x + v.x, y + v.y, z + v.z}; }
    constexpr vector3 operator-(const vector3& v) const { return {x - v.x, y - v.y, z - v.z}; }
    constexpr vector3 operator*(T s) const { return {x * s, y * s, z * s}; }
    constexpr vector3 operator*(const vector3& v) const { return {x * v.x, y * v.y, z * v.z}; }
    constexpr vector3 operator/(T s) const { return {x / s, y / s, z / s}; }
    constexpr vector3& operator+=(const vector3& v) { x += v.x; y += v.y; z += v.z; return *this; }
    constexpr vector3& operator-=(const vector3& v) { x -= v.x; y -= v.y; z -= v.z; return *this; }
    constexpr vector3& operator*=(T s) { x *= s; y *= s; z *= s; return *this; }
    constexpr vector3& operator*=(const vector3& v) { x *= v.x; y *= v.y; z *= v.z; return *this; }
    constexpr vector3& operator/=(T s) { x /= s; y /= s; z /= s; return *this; }
};

template <typename T> struct vector4 {
    T x{}, y{}, z{}, w{};
    constexpr vector4() = default;
    constexpr vector4(T x_, T y_, T z_, T w_) : x(x_), y(y_), z(z_), w(w_) {}
    constexpr explicit operator vector2<T>() const { return {x, y}; }
    constexpr explicit operator vector3<T>() const { return {x, y, z}; }

    constexpr T& operator[](std::size_t i) { assert(i < 4); return i == 0 ? x : (i == 1 ? y : (i == 2 ? z : w)); }
    constexpr const T& operator[](std::size_t i) const { assert(i < 4); return i == 0 ? x : (i == 1 ? y : (i == 2 ? z : w)); }
    constexpr vector4 operator+() const { return *this; }
    constexpr vector4 operator-() const { return {-x, -y, -z, -w}; }
    constexpr vector4 operator+(const vector4& v) const { return {x + v.x, y + v.y, z + v.z, w + v.w}; }
    constexpr vector4 operator-(const vector4& v) const { return {x - v.x, y - v.y, z - v.z, w - v.w}; }
    constexpr vector4 operator*(T s) const { return {x * s, y * s, z * s, w * s}; }
    constexpr vector4 operator*(const vector4& v) const { return {x * v.x, y * v.y, z * v.z, w * v.w}; }
    constexpr vector4 operator/(T s) const { return {x / s, y / s, z / s, w / s}; }
    constexpr vector4& operator+=(const vector4& v) { x += v.x; y += v.y; z += v.z; w += v.w; return *this; }
    constexpr vector4& operator-=(const vector4& v) { x -= v.x; y -= v.y; z -= v.z; w -= v.w; return *this; }
    constexpr vector4& operator*=(T s) { x *= s; y *= s; z *= s; w *= s; return *this; }
    constexpr vector4& operator*=(const vector4& v) { x *= v.x; y *= v.y; z *= v.z; w *= v.w; return *this; }
    constexpr vector4& operator/=(T s) { x /= s; y /= s; z /= s; w /= s; return *this; }
};

template <typename T> constexpr vector2<T> operator*(T s, const vector2<T>& v) { return v * s; }
template <typename T> constexpr vector3<T> operator*(T s, const vector3<T>& v) { return v * s; }
template <typename T> constexpr vector4<T> operator*(T s, const vector4<T>& v) { return v * s; }

template <typename T> constexpr T dot(const vector2<T>& a, const vector2<T>& b) { return a.x * b.x + a.y * b.y; }
template <typename T> constexpr T dot(const vector3<T>& a, const vector3<T>& b) { return a.x * b.x + a.y * b.y + a.z * b.z; }
template <typename T> constexpr T dot(const vector4<T>& a, const vector4<T>& b) { return a.x * b.x + a.y * b.y + a.z * b.z + a.w * b.w; }
template <typename T> constexpr T cross(const vector2<T>& a, const vector2<T>& b) { return a.x * b.y - a.y * b.x; }
template <typename T> constexpr vector3<T> cross(const vector3<T>& a, const vector3<T>& b) {
    return {a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x};
}

template <typename V> constexpr auto lengthSQ(const V& v) { return dot(v, v); }
template <typename V> auto length(const V& v) { return std::sqrt(lengthSQ(v)); }
template <typename V> V normalize(const V& v) { return v / static_cast<decltype(v.x)>(length(v)); }

template <typename T> constexpr T lerp(const T& a, const T& b, float t) {
    return a + (b - a) * t;
}
float lerp(float a, float b, float c, float wa, float wb, float wc) noexcept;

template <typename E, typename Point>
constexpr vector2<E> connect_pixel(const Point& a, const Point& b) {
    return {static_cast<E>(b.x - a.x), static_cast<E>(b.y - a.y)};
}

template <typename T> struct Matrix33 {
    T m[9]{T(1), T(0), T(0), T(0), T(1), T(0), T(0), T(0), T(1)};
    constexpr Matrix33() = default;
    constexpr explicit Matrix33(T diagonal) : m{diagonal, T(0), T(0), T(0), diagonal, T(0), T(0), T(0), diagonal} {}

    constexpr T get(std::size_t row, std::size_t col) const { assert(row < 3 && col < 3); return m[col * 3 + row]; }
    constexpr void set(std::size_t row, std::size_t col, T value) { assert(row < 3 && col < 3); m[col * 3 + row] = value; }
    constexpr void set(T m00, T m01, T m02, T m10, T m11, T m12, T m20, T m21, T m22) {
        m[0] = m00; m[3] = m01; m[6] = m02;
        m[1] = m10; m[4] = m11; m[7] = m12;
        m[2] = m20; m[5] = m21; m[8] = m22;
    }
    constexpr Matrix33& identity() { *this = Matrix33{}; return *this; }
    constexpr vector3<T> getColum(std::size_t col) const { assert(col < 3); return {m[col * 3], m[col * 3 + 1], m[col * 3 + 2]}; }
    constexpr void setColum(const vector3<T>& v, std::size_t col) { assert(col < 3); m[col * 3] = v.x; m[col * 3 + 1] = v.y; m[col * 3 + 2] = v.z; }
    constexpr vector3<T> operator*(const vector3<T>& v) const {
        return {get(0, 0) * v.x + get(0, 1) * v.y + get(0, 2) * v.z,
                get(1, 0) * v.x + get(1, 1) * v.y + get(1, 2) * v.z,
                get(2, 0) * v.x + get(2, 1) * v.y + get(2, 2) * v.z};
    }
    constexpr Matrix33 operator*(T s) const { Matrix33 out(T(0)); for (std::size_t i = 0; i < 9; ++i) out.m[i] = m[i] * s; return out; }
};

template <typename T> struct Matrix44 {
    T m[16]{T(1), T(0), T(0), T(0), T(0), T(1), T(0), T(0), T(0), T(0), T(1), T(0), T(0), T(0), T(0), T(1)};
    constexpr Matrix44() = default;
    constexpr explicit Matrix44(T diagonal) : m{diagonal, T(0), T(0), T(0), T(0), diagonal, T(0), T(0), T(0), T(0), diagonal, T(0), T(0), T(0), T(0), diagonal} {}
    constexpr Matrix44(T sx, T sy, T sz) : m{sx, T(0), T(0), T(0), T(0), sy, T(0), T(0), T(0), T(0), sz, T(0), T(0), T(0), T(0), T(1)} {}

    constexpr T get(std::size_t row, std::size_t col) const { assert(row < 4 && col < 4); return m[col * 4 + row]; }
    constexpr void set(std::size_t row, std::size_t col, T value) { assert(row < 4 && col < 4); m[col * 4 + row] = value; }
    constexpr void set(T m00, T m01, T m02, T m03, T m10, T m11, T m12, T m13,
                       T m20, T m21, T m22, T m23, T m30, T m31, T m32, T m33) {
        m[0] = m00; m[4] = m01; m[8] = m02; m[12] = m03;
        m[1] = m10; m[5] = m11; m[9] = m12; m[13] = m13;
        m[2] = m20; m[6] = m21; m[10] = m22; m[14] = m23;
        m[3] = m30; m[7] = m31; m[11] = m32; m[15] = m33;
    }
    constexpr Matrix44& identity() { *this = Matrix44{}; return *this; }
    constexpr vector4<T> getColum(std::size_t col) const { assert(col < 4); return {m[col * 4], m[col * 4 + 1], m[col * 4 + 2], m[col * 4 + 3]}; }
    constexpr void setColum(const vector4<T>& v, std::size_t col) { assert(col < 4); m[col * 4] = v.x; m[col * 4 + 1] = v.y; m[col * 4 + 2] = v.z; m[col * 4 + 3] = v.w; }
    constexpr vector4<T> operator*(const vector4<T>& v) const {
        return {get(0, 0) * v.x + get(0, 1) * v.y + get(0, 2) * v.z + get(0, 3) * v.w,
                get(1, 0) * v.x + get(1, 1) * v.y + get(1, 2) * v.z + get(1, 3) * v.w,
                get(2, 0) * v.x + get(2, 1) * v.y + get(2, 2) * v.z + get(2, 3) * v.w,
                get(3, 0) * v.x + get(3, 1) * v.y + get(3, 2) * v.z + get(3, 3) * v.w};
    }
    constexpr Matrix44 operator*(T s) const { Matrix44 out(T(0)); for (std::size_t i = 0; i < 16; ++i) out.m[i] = m[i] * s; return out; }
};

template <typename T> constexpr Matrix33<T> transpose(const Matrix33<T>& a) {
    Matrix33<T> out(T(0));
    for (std::size_t row = 0; row < 3; ++row)
        for (std::size_t col = 0; col < 3; ++col) out.set(row, col, a.get(col, row));
    return out;
}
template <typename T> constexpr Matrix44<T> transpose(const Matrix44<T>& a) {
    Matrix44<T> out(T(0));
    for (std::size_t row = 0; row < 4; ++row)
        for (std::size_t col = 0; col < 4; ++col) out.set(row, col, a.get(col, row));
    return out;
}
template <typename T> constexpr Matrix33<T> operator*(const Matrix33<T>& a, const Matrix33<T>& b) {
    Matrix33<T> out(T(0));
    for (std::size_t row = 0; row < 3; ++row)
        for (std::size_t col = 0; col < 3; ++col)
            for (std::size_t k = 0; k < 3; ++k) out.m[col * 3 + row] += a.get(row, k) * b.get(k, col);
    return out;
}
template <typename T> constexpr Matrix44<T> operator*(const Matrix44<T>& a, const Matrix44<T>& b) {
    Matrix44<T> out(T(0));
    for (std::size_t row = 0; row < 4; ++row)
        for (std::size_t col = 0; col < 4; ++col)
            for (std::size_t k = 0; k < 4; ++k) out.m[col * 4 + row] += a.get(row, k) * b.get(k, col);
    return out;
}

template <typename T> Matrix44<T> inverse(const Matrix44<T>& source) {
    static_assert(std::is_floating_point_v<T>, "Matrix inverse requires floating-point elements");
    T augmented[4][8]{};
    for (std::size_t row = 0; row < 4; ++row)
        for (std::size_t col = 0; col < 4; ++col) {
            augmented[row][col] = source.get(row, col);
            augmented[row][col + 4] = row == col ? T(1) : T(0);
        }
    for (std::size_t col = 0; col < 4; ++col) {
        std::size_t pivot = col;
        for (std::size_t row = col + 1; row < 4; ++row)
            if (std::abs(augmented[row][col]) > std::abs(augmented[pivot][col])) pivot = row;
        if (augmented[pivot][col] == T(0)) throw std::domain_error("singular Matrix44");
        if (pivot != col)
            for (std::size_t k = 0; k < 8; ++k) {
                const T tmp = augmented[col][k];
                augmented[col][k] = augmented[pivot][k];
                augmented[pivot][k] = tmp;
            }
        const T divisor = augmented[col][col];
        for (std::size_t k = 0; k < 8; ++k) augmented[col][k] /= divisor;
        for (std::size_t row = 0; row < 4; ++row) {
            if (row == col) continue;
            const T factor = augmented[row][col];
            for (std::size_t k = 0; k < 8; ++k) augmented[row][k] -= factor * augmented[col][k];
        }
    }
    Matrix44<T> out(T(0));
    for (std::size_t row = 0; row < 4; ++row)
        for (std::size_t col = 0; col < 4; ++col) out.set(row, col, augmented[row][col + 4]);
    return out;
}

template <typename T> constexpr Matrix44<T> scale(const Matrix44<T>& source, T x, T y, T z) {
    Matrix44<T> out = source;
    const T values[3]{x, y, z};
    for (std::size_t col = 0; col < 3; ++col)
        for (std::size_t row = 0; row < 4; ++row) out.m[col * 4 + row] *= values[col];
    return out;
}
template <typename T> constexpr Matrix44<T> translate(const Matrix44<T>& source, T x, T y, T z) {
    Matrix44<T> out = source;
    out.setColum(source.getColum(3) + source.getColum(0) * x + source.getColum(1) * y + source.getColum(2) * z, 3);
    return out;
}
template <typename T> constexpr Matrix44<T> translate(const Matrix44<T>& source, const vector3<T>& v) {
    return translate(source, v.x, v.y, v.z);
}
template <typename T> Matrix44<T> rotate(const Matrix44<T>& source, T theta, const vector3<T>& axis) {
    const vector3<T> n = normalize(axis);
    const T c = std::cos(theta), s = std::sin(theta), d = T(1) - c;
    Matrix44<T> r;
    r.set(0, 0, c + n.x * n.x * d); r.set(0, 1, n.x * n.y * d - n.z * s); r.set(0, 2, n.x * n.z * d + n.y * s);
    r.set(1, 0, n.y * n.x * d + n.z * s); r.set(1, 1, c + n.y * n.y * d); r.set(1, 2, n.y * n.z * d - n.x * s);
    r.set(2, 0, n.z * n.x * d - n.y * s); r.set(2, 1, n.z * n.y * d + n.x * s); r.set(2, 2, c + n.z * n.z * d);
    return source * r;
}
template <typename T> Matrix44<T> orthographic(T left, T right, T bottom, T top, T nearPlane, T farPlane) {
    Matrix44<T> out;
    out.set(0, 0, T(2) / (right - left)); out.set(0, 3, -(right + left) / (right - left));
    out.set(1, 1, T(2) / (top - bottom)); out.set(1, 3, -(top + bottom) / (top - bottom));
    out.set(2, 2, T(-2) / (farPlane - nearPlane)); out.set(2, 3, -(farPlane + nearPlane) / (farPlane - nearPlane));
    return out;
}
template <typename T> Matrix44<T> perspective(T fovy, T aspect, T nearPlane, T farPlane) {
    const T tanHalfFovy = static_cast<T>(std::tan(radians(static_cast<double>(fovy) / 2.0)));
    Matrix44<T> out(T(0));
    out.set(0, 0, T(1) / (aspect * tanHalfFovy));
    out.set(1, 1, T(1) / tanHalfFovy);
    out.set(2, 2, -(farPlane + nearPlane) / (farPlane - nearPlane));
    out.set(2, 3, T(-2) * farPlane * nearPlane / (farPlane - nearPlane));
    out.set(3, 2, T(-1));
    return out;
}
template <typename T> constexpr Matrix44<T> screenMatrix(std::uint32_t width, std::uint32_t height) {
    Matrix44<T> out;
    out.set(0, 0, static_cast<T>(width) / T(2)); out.set(0, 3, static_cast<T>(width) / T(2));
    out.set(1, 1, static_cast<T>(height) / T(2)); out.set(1, 3, static_cast<T>(height) / T(2));
    out.set(2, 2, T(0.5)); out.set(2, 3, T(0.5));
    return out;
}
template <typename T, typename V> Matrix44<T> lookAt(const vector3<V>& eye, const vector3<V>& center, const vector3<V>& up) {
    const vector3<V> forward = normalize(center - eye);
    const vector3<V> right = normalize(cross(forward, up));
    const vector3<V> trueUp = cross(right, forward);
    Matrix44<T> out;
    out.set(0, 0, static_cast<T>(right.x)); out.set(0, 1, static_cast<T>(right.y)); out.set(0, 2, static_cast<T>(right.z)); out.set(0, 3, static_cast<T>(-dot(right, eye)));
    out.set(1, 0, static_cast<T>(trueUp.x)); out.set(1, 1, static_cast<T>(trueUp.y)); out.set(1, 2, static_cast<T>(trueUp.z)); out.set(1, 3, static_cast<T>(-dot(trueUp, eye)));
    out.set(2, 0, static_cast<T>(-forward.x)); out.set(2, 1, static_cast<T>(-forward.y)); out.set(2, 2, static_cast<T>(-forward.z)); out.set(2, 3, static_cast<T>(dot(forward, eye)));
    return out;
}

} 

using vec2i = Math::vector2<int>;
using vec2f = Math::vector2<float>;
using vec3i = Math::vector3<int>;
using vec3f = Math::vector3<float>;
using vec4i = Math::vector4<int>;
using vec4f = Math::vector4<float>;
using mat3f = Math::Matrix33<float>;
using mat4f = Math::Matrix44<float>;
