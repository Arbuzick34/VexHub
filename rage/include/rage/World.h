#pragma once

#include "Math.h"

#include <vector>

namespace rage
{
enum class HitboxGroup : int
{
    Head = 0,
    Neck = 1,
    Chest = 2,
    Stomach = 3,
    Pelvis = 4,
    Legs = 5,
    Feet = 6,
    Unknown = 7,
};

struct HitboxView
{
    HitboxGroup group = HitboxGroup::Unknown;
    Vec3 center;
    Basis basis;   // unit axes in world space
    Vec3 half;     // half extents along the basis axes

    float Radius() const { return (half.x + half.y) * 0.5f; }
};

struct PlayerView
{
    int index = -1;
    bool alive = true;
    int health = 100;
    int armor = 0;
    int team = 0;
    Vec3 origin;          // feet position
    Vec3 eye;             // eye position
    float velocity = 0.0f;
    Angles viewAngles;    // last networked view angles, used by the resolver memory
    std::vector<HitboxView> hitboxes;
};

struct WeaponInfo
{
    float damage = 36.0f;          // unmodified body damage
    float rangeModifier = 0.98f;   // damage falloff per 500 units
    float armorRatio = 0.5f;       // damage scale against kevlar
    float spread = 0.05f;          // current inaccuracy, degrees
    int maxPenetrations = 3;
    bool scoped = true;
    bool hasScope = false;
};

struct TraceHit
{
    bool hit = false;
    bool world = true;              // true when the trace stopped on a brush
    int entity = -1;                // player index for player hits
    HitboxGroup group = HitboxGroup::Unknown;
    float fraction = 1.0f;
    Vec3 end;
    Vec3 normal;
};

// Everything the solver needs from the game, so the algorithm stays testable and
// the SDK only has to implement this thin adapter. Defaults keep the interface
// cheap to implement: a host that does not care about anti-aim can ignore them.
class World
{
public:
    virtual ~World() = default;

    virtual int LocalTeam() const = 0;
    virtual Vec3 EyePosition() const = 0;
    virtual Angles ViewAngles() const = 0;
    virtual WeaponInfo Weapon() const = 0;
    virtual std::vector<PlayerView> Players() const = 0;

    virtual int LocalIndex() const { return -1; }
    virtual Vec3 LocalVelocity() const { return {}; }
    virtual bool LocalOnGround() const { return true; }

    // First collision along the segment; `world` distinguishes brushes from players.
    virtual TraceHit TraceLine(const Vec3& from, const Vec3& to) const = 0;

    // Engine-style TraceToExit: given the hit that entered a wall, step through it and
    // return where the ray leaves solid geometry. Returns false when the wall is too thick.
    virtual bool TraceToExit(Vec3& exit, const Vec3& start, const Vec3& dir, const TraceHit& enter) const = 0;
};

} // namespace rage
