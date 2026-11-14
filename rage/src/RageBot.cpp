#include "rage/RageBot.h"

#include <algorithm>
#include <cmath>

namespace rage
{
namespace
{
constexpr float kNoScore = -1.0e30f;

float ScorePlan(const ShotPlan& plan, const RageConfig& cfg)
{
    float score = 0.0f;
    switch (cfg.priority)
    {
    case AimPriority::Damage:
        score = plan.damage * 4.0f + plan.hitchance;
        break;
    case AimPriority::Hitchance:
        score = plan.hitchance * 4.0f + plan.damage;
        break;
    case AimPriority::Distance:
        score = 4000.0f - plan.distance + plan.damage * 2.0f;
        break;
    }

    if (plan.group == HitboxGroup::Head) score += 20.0f;
    if (plan.throughWall) score -= 15.0f;
    if (!plan.safePoint) score -= 2.0f;
    return score;
}

float GroupRadius(const PlayerView& player, HitboxGroup group)
{
    for (const HitboxView& box : player.hitboxes)
    {
        if (box.group == group) return std::min({ box.half.x, box.half.y, box.half.z });
    }
    return 2.0f;
}
} // namespace

// ── resolver ─────────────────────────────────────────────────────────────────

void Resolver::Reset() { _states.clear(); }

void Resolver::Observe(int player, const PlayerView& view)
{
    if (player < 0) return;

    ResolverState& state = _states[player];
    const float delta = DeltaYaw(state.lastYaw, view.viewAngles.yaw);
    if (std::fabs(delta) > 1.0f && std::fabs(delta) < 179.0f) state.lastDelta = delta;
    state.lastYaw = view.viewAngles.yaw;
}

void Resolver::OnHit(int player)
{
    ResolverState& state = _states[player];
    state.hits++;
    state.misses = 0;
    state.shot = 0;
    state.offset = 0.0f;
}

void Resolver::OnMiss(int player)
{
    ResolverState& state = _states[player];
    state.misses++;
    state.shot++;
    if (state.shot > 11) state.shot = 0;
}

float Resolver::Offset(int player, const RageConfig& cfg, int tick) const
{
    (void)tick;

    if (!cfg.resolver) return 0.0f;

    const auto it = _states.find(player);
    if (it == _states.end()) return 0.0f;

    const ResolverState& state = it->second;
    if (!cfg.bruteForce || state.misses == 0) return 0.0f;

    static const float kBrute[] = {
        0.0f, 30.0f, -30.0f, 60.0f, -60.0f, 90.0f,
        -90.0f, 120.0f, -120.0f, 150.0f, -150.0f, 180.0f,
    };
    constexpr int kSteps = static_cast<int>(sizeof(kBrute) / sizeof(kBrute[0]));

    // Once the target has been missed repeatedly, sweep the whole range.
    int step = state.shot % kSteps;
    if (state.misses > cfg.maxMisses) step = (state.misses / cfg.maxMisses) % kSteps;
    return kBrute[step];
}

const ResolverState* Resolver::State(int player) const
{
    const auto it = _states.find(player);
    return it == _states.end() ? nullptr : &it->second;
}

void Resolver::RotatePlayer(PlayerView& player, float deltaYaw)
{
    if (std::fabs(deltaYaw) < 0.01f) return;

    for (HitboxView& box : player.hitboxes)
    {
        const Vec3 relative = box.center - player.origin;
        box.center = player.origin + RotateYaw(relative, deltaYaw);
        box.basis.forward = RotateYaw(box.basis.forward, deltaYaw);
        box.basis.right = RotateYaw(box.basis.right, deltaYaw);
        box.basis.up = RotateYaw(box.basis.up, deltaYaw);
    }

    player.viewAngles.yaw = NormalizeYaw(player.viewAngles.yaw + deltaYaw);
}

// ── aim points ───────────────────────────────────────────────────────────────

std::vector<AimPoint> RageBot::BuildAimPoints(const PlayerView& player, const RageConfig& cfg)
{
    std::vector<AimPoint> points;
    points.reserve(32);

    std::vector<HitboxGroup> order = cfg.hitboxOrder;
    if (cfg.preferHead)
    {
        const auto it = std::find(order.begin(), order.end(), HitboxGroup::Head);
        if (it != order.end() && it != order.begin())
            std::rotate(order.begin(), it, it + 1);
        else if (it == order.end())
            order.insert(order.begin(), HitboxGroup::Head);
    }

    for (const HitboxGroup group : order)
    {
        const HitboxView* box = nullptr;
        for (const HitboxView& candidate : player.hitboxes)
        {
            if (candidate.group == group)
            {
                box = &candidate;
                break;
            }
        }
        if (box == nullptr) continue;

        points.push_back({ box->center, group, true, std::max(0.5f, std::min({ box->half.x, box->half.y, box->half.z })) });

        const bool multipoint = cfg.multipoint && (!cfg.multipointHeadOnly || group == HitboxGroup::Head);
        if (!multipoint) continue;

        const float sx = box->half.y * cfg.multipointScale;  // along the right axis
        const float sy = box->half.z * cfg.multipointScale;  // along the up axis

        struct Offset { float x; float y; };
        static const Offset kOffsets[8] = {
            { 1.0f, 0.0f }, { -1.0f, 0.0f }, { 0.0f, 1.0f }, { 0.0f, -1.0f },
            { 1.0f, 1.0f }, { 1.0f, -1.0f }, { -1.0f, 1.0f }, { -1.0f, -1.0f },
        };

        for (const Offset& offset : kOffsets)
        {
            const float ox = offset.x * sx;
            const float oy = offset.y * sy;
            if (ox == 0.0f && oy == 0.0f) continue;

            const Vec3 point = box->center + box->basis.right * ox + box->basis.up * oy;
            const float rx = std::max(0.5f, box->half.y - std::fabs(ox));
            const float ry = std::max(0.5f, box->half.z - std::fabs(oy));
            const float radius = std::min(rx, ry);
            if (radius < 0.75f) continue;

            points.push_back({ point, group, false, radius });
        }
    }

    return points;
}

// ── shot solving ─────────────────────────────────────────────────────────────

ShotPlan RageBot::SolvePoint(const World& world, const WeaponInfo& weapon, const PlayerView& player,
                             const AimPoint& aim, const Vec3& eye, const RageConfig& cfg)
{
    ShotPlan plan;
    plan.target = player.index;
    plan.group = aim.group;
    plan.point = aim.point;
    plan.safePoint = aim.center;

    const Vec3 toPoint = aim.point - eye;
    const float distance = Length(toPoint);
    if (distance < 1.0f) return plan;

    const Vec3 dir = toPoint / distance;

    int penetrations = 0;
    bool throughWall = false;
    const int maxPenetrations =
        cfg.penetration ? std::min(cfg.maxPenetrations, weapon.maxPenetrations) : 0;

    TraceHit hit = world.TraceLine(eye, aim.point);

    while (true)
    {
        if (hit.hit && hit.entity == player.index)
        {
            // A graze can land in a neighbouring hitbox; the damage then follows
            // the box that was actually hit, not the one that was aimed at.
            const HitboxGroup group = hit.group == HitboxGroup::Unknown ? aim.group : hit.group;
            const float hitDistance = Length(hit.end - eye);

            float damage = weapon.damage * HitboxMultiplier(group);
            damage = DamageFalloff(damage, weapon.rangeModifier, hitDistance);
            damage *= std::pow(cfg.penetrationScale, static_cast<float>(penetrations));
            damage = ApplyArmor(damage, player.armor, weapon.armorRatio);

            plan.valid = true;
            plan.group = group;
            plan.point = aim.point;
            plan.distance = hitDistance;
            plan.penetrations = penetrations;
            plan.throughWall = throughWall;
            plan.angles = VectorAngles(aim.point - eye);
            plan.damage = std::min(damage, static_cast<float>(player.health));
            plan.hitchance = Hitchance(weapon.spread, aim.center ? GroupRadius(player, group) : aim.radius, hitDistance);
            return plan;
        }

        if (!hit.hit) return plan;
        if (hit.entity >= 0) return plan;              // another player blocks the line
        if (penetrations >= maxPenetrations) return plan;

        Vec3 exit;
        if (!world.TraceToExit(exit, hit.end, dir, hit)) return plan;

        throughWall = true;
        penetrations++;

        hit = world.TraceLine(exit, aim.point);
    }
}

// ── main update ──────────────────────────────────────────────────────────────

void RageBot::Reset()
{
    _resolver.Reset();
    _antiAim.Reset();
}

RageCommand RageBot::Update(const World& world, const RageConfig& cfg, const AntiAimConfig& aaConfig, int tick)
{
    RageCommand cmd;
    cmd.tick = tick;

    const WeaponInfo weapon = world.Weapon();
    const Vec3 eye = world.EyePosition();
    const Angles view = world.ViewAngles();
    const Vec3 forward = ForwardVector(view);
    const int localTeam = world.LocalTeam();
    const int localIndex = world.LocalIndex();

    std::vector<PlayerView> players = world.Players();

    if (cfg.resolver)
    {
        for (PlayerView& player : players)
        {
            _resolver.Observe(player.index, player);
            const float offset = _resolver.Offset(player.index, cfg, tick);
            if (offset != 0.0f) Resolver::RotatePlayer(player, offset);
        }
    }

    ShotPlan best;
    float bestScore = kNoScore;

    if (cfg.enabled)
    {
        for (const PlayerView& player : players)
        {
            if (!player.alive || player.health <= 0) continue;
            if (player.index == localIndex) continue;
            if (player.team == localTeam) continue;

            const float distance = Length(player.origin - eye);
            if (distance > cfg.maxRange) continue;

            for (const AimPoint& aim : BuildAimPoints(player, cfg))
            {
                const Vec3 toPoint = aim.point - eye;
                if (LengthSquared(toPoint) < 1.0f) continue;
                if (cfg.fov < 180.0f && AngleBetween(forward, toPoint) > cfg.fov) continue;

                const ShotPlan plan = SolvePoint(world, weapon, player, aim, eye, cfg);
                if (!plan.valid) continue;
                if (plan.throughWall && !cfg.autoWall) continue;
                if (plan.hitchance < cfg.hitchance) continue;

                float required = cfg.minDamage;
                if (player.health <= cfg.lowHealthThreshold)
                    required = std::min(required, cfg.minDamageLow);
                if (plan.damage < required) continue;

                const float score = ScorePlan(plan, cfg);
                if (score > bestScore)
                {
                    bestScore = score;
                    best = plan;
                }
            }
        }
    }

    if (best.valid)
    {
        cmd.plan = best;
        cmd.angles = best.angles;
        cmd.writeAngles = true;
        cmd.fire = cfg.autoFire;
        cmd.stop = cfg.autoStop;
        cmd.scope = cfg.autoScope && weapon.hasScope && !weapon.scoped;
        if (cmd.scope) cmd.fire = false;
    }

    if (aaConfig.enabled)
    {
        AntiAimInput input;
        input.yaw = view.yaw;
        input.pitch = view.pitch;
        input.velocity = world.LocalVelocity();
        input.onGround = world.LocalOnGround();
        input.firing = cmd.fire;
        input.tick = tick;

        cmd.antiAim = _antiAim.Update(aaConfig, input);

        if (cmd.antiAim.active)
        {
            cmd.writeAngles = true;
            cmd.choke = aaConfig.hideShots && cmd.fire;
            if (!(cmd.fire && aaConfig.hideShots)) cmd.angles = cmd.antiAim.angles;
        }
    }

    return cmd;
}

} // namespace rage
