#pragma once

#include <algorithm>
#include <cmath>

namespace rage
{
constexpr float kPi = 3.14159265358979323846f;
constexpr float kRadToDeg = 180.0f / kPi;
constexpr float kDegToRad = kPi / 180.0f;

struct Vec3
{
    float x = 0.0f;
    float y = 0.0f;
    float z = 0.0f;

    Vec3() = default;
    Vec3(float x_, float y_, float z_) : x(x_), y(y_), z(z_) {}

    Vec3 operator+(const Vec3& other) const { return { x + other.x, y + other.y, z + other.z }; }
    Vec3 operator-(const Vec3& other) const { return { x - other.x, y - other.y, z - other.z }; }
    Vec3 operator-() const { return { -x, -y, -z }; }
    Vec3 operator*(float scale) const { return { x * scale, y * scale, z * scale }; }
    Vec3 operator/(float scale) const { return { x / scale, y / scale, z / scale }; }
    Vec3& operator+=(const Vec3& other) { x += other.x; y += other.y; z += other.z; return *this; }
    Vec3& operator-=(const Vec3& other) { x -= other.x; y -= other.y; z -= other.z; return *this; }
    Vec3& operator*=(float scale) { x *= scale; y *= scale; z *= scale; return *this; }
};

inline Vec3 operator*(float scale, const Vec3& v) { return v * scale; }

inline float Dot(const Vec3& a, const Vec3& b) { return a.x * b.x + a.y * b.y + a.z * b.z; }

inline Vec3 Cross(const Vec3& a, const Vec3& b)
{
    return {
        a.y * b.z - a.z * b.y,
        a.z * b.x - a.x * b.z,
        a.x * b.y - a.y * b.x,
    };
}

inline float LengthSquared(const Vec3& v) { return Dot(v, v); }
inline float Length(const Vec3& v) { return std::sqrt(LengthSquared(v)); }

inline Vec3 Normalize(const Vec3& v)
{
    const float length = Length(v);
    return length > 1e-6f ? v / length : Vec3{};
}

inline Vec3 Lerp(const Vec3& a, const Vec3& b, float t) { return a + (b - a) * t; }

struct Angles
{
    float pitch = 0.0f;
    float yaw = 0.0f;
    float roll = 0.0f;
};

inline float NormalizeYaw(float yaw)
{
    if (!std::isfinite(yaw)) return 0.0f;
    yaw = std::fmod(yaw + 180.0f, 360.0f);
    if (yaw < 0.0f) yaw += 360.0f;
    return yaw - 180.0f;
}

inline float ClampPitch(float pitch) { return std::clamp(pitch, -89.0f, 89.0f); }

inline Angles Normalize(const Angles& in)
{
    return { ClampPitch(in.pitch), NormalizeYaw(in.yaw), NormalizeYaw(in.roll) };
}

// Signed shortest rotation from `from` to `to`, in degrees.
inline float DeltaYaw(float from, float to) { return NormalizeYaw(to - from); }

struct Basis
{
    Vec3 forward;
    Vec3 right;
    Vec3 up;

    Vec3 ToWorld(const Vec3& local) const { return forward * local.x + right * local.y + up * local.z; }
    Vec3 ToLocal(const Vec3& world) const { return { Dot(world, forward), Dot(world, right), Dot(world, up) }; }
};

inline Basis AnglesToBasis(const Angles& angles)
{
    const float sp = std::sin(angles.pitch * kDegToRad);
    const float cp = std::cos(angles.pitch * kDegToRad);
    const float sy = std::sin(angles.yaw * kDegToRad);
    const float cy = std::cos(angles.yaw * kDegToRad);

    Basis basis;
    basis.forward = { cp * cy, cp * sy, -sp };
    basis.right = { -sy, cy, 0.0f };
    basis.up = Cross(basis.right, basis.forward);
    return basis;
}

inline Vec3 ForwardVector(const Angles& angles) { return AnglesToBasis(angles).forward; }

inline Angles VectorAngles(const Vec3& forward)
{
    Angles out;
    const float planar = std::sqrt(forward.x * forward.x + forward.y * forward.y);

    if (planar < 1e-5f)
    {
        out.yaw = 0.0f;
        out.pitch = forward.z >= 0.0f ? -90.0f : 90.0f;
        return out;
    }

    out.yaw = std::atan2(forward.y, forward.x) * kRadToDeg;
    out.pitch = std::atan2(-forward.z, planar) * kRadToDeg;
    return out;
}

inline float AngleBetween(const Vec3& a, const Vec3& b)
{
    const Vec3 na = Normalize(a);
    const Vec3 nb = Normalize(b);
    return std::acos(std::clamp(Dot(na, nb), -1.0f, 1.0f)) * kRadToDeg;
}

inline Vec3 RotateYaw(const Vec3& v, float degrees)
{
    const float rad = degrees * kDegToRad;
    const float s = std::sin(rad);
    const float c = std::cos(rad);
    return { v.x * c - v.y * s, v.x * s + v.y * c, v.z };
}

// Ray vs oriented box (slab test in the box's local space).
// Returns true when the segment hits; `distance` is the entry distance along `dir` (normalized).
inline bool RayObb(const Vec3& origin, const Vec3& dir, const Vec3& center, const Basis& basis, const Vec3& half, float& distance)
{
    const Vec3 localOrigin = basis.ToLocal(origin - center);
    const Vec3 localDir = basis.ToLocal(dir);

    float tMin = -1e9f;
    float tMax = 1e9f;

    const float originAxis[3] = { localOrigin.x, localOrigin.y, localOrigin.z };
    const float dirAxis[3] = { localDir.x, localDir.y, localDir.z };
    const float halfAxis[3] = { half.x, half.y, half.z };

    for (int i = 0; i < 3; ++i)
    {
        if (std::fabs(dirAxis[i]) < 1e-6f)
        {
            if (originAxis[i] < -halfAxis[i] || originAxis[i] > halfAxis[i]) return false;
            continue;
        }

        const float inv = 1.0f / dirAxis[i];
        float t1 = (-halfAxis[i] - originAxis[i]) * inv;
        float t2 = (halfAxis[i] - originAxis[i]) * inv;
        if (t1 > t2) std::swap(t1, t2);

        tMin = std::max(tMin, t1);
        tMax = std::min(tMax, t2);
        if (tMin > tMax) return false;
    }

    // Segment accepted only when the intersection lies in front of the origin.
    if (tMax < 0.0f) return false;
    distance = tMin >= 0.0f ? tMin : 0.0f;
    return true;
}

} // namespace rage
