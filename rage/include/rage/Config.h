#pragma once

#include "World.h"

#include <vector>

namespace rage
{
// How competing shot plans are ranked.
enum class AimPriority : int
{
    Damage = 0,
    Hitchance = 1,
    Distance = 2,
};

struct RageConfig
{
    bool enabled = true;
    bool autoFire = true;
    bool autoWall = true;      // allow shooting through penetrable geometry
    bool autoScope = true;     // scope before firing a sniper
    bool autoStop = true;      // signal the host to stop movement while shooting

    float fov = 180.0f;        // degrees, 180 = no restriction
    float maxRange = 8192.0f;  // world units

    // Damage gate. When the target is at or below `lowHealthThreshold` the
    // solver accepts `minDamageLow` instead of `minDamage`.
    float minDamage = 30.0f;
    float minDamageLow = 10.0f;
    int lowHealthThreshold = 35;

    float hitchance = 55.0f;   // percent
    int maxMisses = 4;         // resolver brute-force resets after this many misses

    bool multipoint = true;
    float multipointScale = 0.55f;  // fraction of the hitbox half extents
    bool multipointHeadOnly = false;

    bool penetration = true;
    int maxPenetrations = 3;
    float penetrationScale = 0.5f;  // damage multiplier per penetrated wall

    bool resolver = true;
    bool bruteForce = true;

    bool preferHead = true;
    AimPriority priority = AimPriority::Damage;

    // Priority order for aim points; `preferHead` lifts Head to the front.
    std::vector<HitboxGroup> hitboxOrder = {
        HitboxGroup::Head,
        HitboxGroup::Neck,
        HitboxGroup::Chest,
        HitboxGroup::Stomach,
        HitboxGroup::Pelvis,
        HitboxGroup::Legs,
        HitboxGroup::Feet,
    };
};

} // namespace rage
