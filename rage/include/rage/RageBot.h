#pragma once

#include "AntiAim.h"
#include "Config.h"
#include "Math.h"
#include "World.h"

#include <unordered_map>
#include <vector>

namespace rage
{
struct AimPoint
{
    Vec3 point;
    HitboxGroup group = HitboxGroup::Unknown;
    bool center = true;
    float radius = 0.0f;  // usable target radius around the point, world units
};

struct ShotPlan
{
    bool valid = false;
    int target = -1;
    Vec3 point;
    HitboxGroup group = HitboxGroup::Unknown;
    Angles angles;
    float damage = 0.0f;
    float hitchance = 0.0f;
    float distance = 0.0f;
    int penetrations = 0;
    bool throughWall = false;
    bool safePoint = true;   // true for the hitbox center, false for a multipoint edge
};

struct RageCommand
{
    int tick = 0;

    bool writeAngles = false;
    Angles angles;

    bool fire = false;
    bool scope = false;
    bool stop = false;      // auto-stop movement for this tick
    bool choke = false;     // hide-shots hint: suppress the outgoing packet

    ShotPlan plan;
    AntiAimOutput antiAim;
};

// Per-player resolver memory: tracks the last real yaw, hit/miss counters and
// the brute-force step to apply next.
struct ResolverState
{
    float lastYaw = 0.0f;
    float lastDelta = 0.0f;
    float offset = 0.0f;
    int misses = 0;
    int hits = 0;
    int shot = 0;
};

class Resolver
{
public:
    void Reset();
    void Observe(int player, const PlayerView& view);
    void OnHit(int player);
    void OnMiss(int player);

    float Offset(int player, const RageConfig& cfg, int tick) const;
    const ResolverState* State(int player) const;

    // Rotates the hitboxes of the player around its origin, which is how a
    // desynced body is re-projected for the trace.
    static void RotatePlayer(PlayerView& player, float deltaYaw);

private:
    std::unordered_map<int, ResolverState> _states;
};

// Damage / accuracy model, public so the host and the tests share it.
inline float HitboxMultiplier(HitboxGroup group)
{
    switch (group)
    {
    case HitboxGroup::Head: return 4.0f;
    case HitboxGroup::Neck: return 1.5f;
    case HitboxGroup::Chest: return 1.0f;
    case HitboxGroup::Stomach: return 1.25f;
    case HitboxGroup::Pelvis: return 1.0f;
    case HitboxGroup::Legs: return 0.75f;
    case HitboxGroup::Feet: return 0.75f;
    default: return 1.0f;
    }
}

inline float DamageFalloff(float damage, float rangeModifier, float distance)
{
    const float steps = distance / 500.0f;
    return damage * std::pow(rangeModifier, steps);
}

inline float ApplyArmor(float damage, int armor, float armorRatio)
{
    if (armor <= 0) return damage;

    float health = damage * armorRatio;
    const float absorbed = (damage - health) * 0.5f;
    if (absorbed > static_cast<float>(armor)) health = damage - static_cast<float>(armor) * 2.0f;
    return std::max(0.0f, health);
}

// Rayleigh approximation: a gaussian angular error with sigma `spreadDegrees`
// landing inside a cone of `radius` at `distance`. Returns percent.
inline float Hitchance(float spreadDegrees, float radius, float distance)
{
    if (distance <= 0.0f || radius <= 0.0f) return 0.0f;
    if (spreadDegrees <= 0.0001f) return 100.0f;

    const float cone = std::atan2(radius, distance) * kRadToDeg;
    const float exponent = -(cone * cone) / (2.0f * spreadDegrees * spreadDegrees);
    return std::clamp((1.0f - std::exp(exponent)) * 100.0f, 0.0f, 100.0f);
}

class RageBot
{
public:
    RageCommand Update(const World& world, const RageConfig& cfg, const AntiAimConfig& aaConfig, int tick);
    void Reset();

    Resolver& GetResolver() { return _resolver; }
    const Resolver& GetResolver() const { return _resolver; }

    // Exposed pieces used by the tests and by external tooling.
    static std::vector<AimPoint> BuildAimPoints(const PlayerView& player, const RageConfig& cfg);
    static ShotPlan SolvePoint(const World& world, const WeaponInfo& weapon, const PlayerView& player,
                               const AimPoint& aim, const Vec3& eye, const RageConfig& cfg);

private:
    Resolver _resolver;
    AntiAim _antiAim;
};

} // namespace rage
