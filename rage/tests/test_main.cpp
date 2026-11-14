#include "rage/AntiAim.h"
#include "rage/Config.h"
#include "rage/Math.h"
#include "rage/RageBot.h"
#include "rage/World.h"

#include <cmath>
#include <cstdio>
#include <vector>

using namespace rage;

namespace
{
int g_checks = 0;
int g_failures = 0;

void Check(bool condition, const char* what, int line)
{
    ++g_checks;
    if (!condition)
    {
        ++g_failures;
        std::printf("  FAIL line %d: %s\n", line, what);
    }
}

void CheckNear(float actual, float expected, float epsilon, const char* what, int line)
{
    ++g_checks;
    if (std::fabs(actual - expected) > epsilon)
    {
        ++g_failures;
        std::printf("  FAIL line %d: %s (%.4f vs %.4f)\n", line, what, actual, expected);
    }
}

#define CHECK(cond) Check((cond), #cond, __LINE__)
#define CHECK_NEAR(a, b, eps) CheckNear((a), (b), (eps), #a, __LINE__)

Basis Identity()
{
    Basis basis;
    basis.forward = { 1.0f, 0.0f, 0.0f };
    basis.right = { 0.0f, 1.0f, 0.0f };
    basis.up = { 0.0f, 0.0f, 1.0f };
    return basis;
}

struct Box
{
    Vec3 center;
    Vec3 half;
    Basis basis = Identity();
};

PlayerView MakePlayer(int index, int team, const Vec3& feet)
{
    PlayerView player;
    player.index = index;
    player.team = team;
    player.alive = true;
    player.health = 100;
    player.armor = 0;
    player.origin = feet;
    player.eye = feet + Vec3(0.0f, 0.0f, 64.0f);
    player.viewAngles = { 0.0f, 180.0f, 0.0f };
    player.hitboxes = {
        { HitboxGroup::Head, feet + Vec3(0.0f, 0.0f, 70.0f), Identity(), { 3.0f, 3.0f, 3.0f } },
        { HitboxGroup::Chest, feet + Vec3(0.0f, 0.0f, 55.0f), Identity(), { 4.0f, 6.0f, 9.0f } },
        { HitboxGroup::Stomach, feet + Vec3(0.0f, 0.0f, 45.0f), Identity(), { 4.0f, 5.0f, 5.0f } },
    };
    return player;
}

class FakeWorld : public World
{
public:
    Vec3 eye{ 0.0f, 0.0f, 64.0f };
    Angles view{ 0.0f, 0.0f, 0.0f };
    Vec3 velocity;
    bool onGround = true;
    int localTeam = 2;
    int localIndex = 0;
    WeaponInfo weapon;
    std::vector<PlayerView> players;
    std::vector<Box> walls;

    int LocalIndex() const override { return localIndex; }
    Vec3 LocalVelocity() const override { return velocity; }
    bool LocalOnGround() const override { return onGround; }
    int LocalTeam() const override { return localTeam; }
    Vec3 EyePosition() const override { return eye; }
    Angles ViewAngles() const override { return view; }
    WeaponInfo Weapon() const override { return weapon; }
    std::vector<PlayerView> Players() const override { return players; }

    bool InsideWall(const Vec3& point) const
    {
        for (const Box& wall : walls)
        {
            const Vec3 local = wall.basis.ToLocal(point - wall.center);
            if (std::fabs(local.x) <= wall.half.x && std::fabs(local.y) <= wall.half.y &&
                std::fabs(local.z) <= wall.half.z)
                return true;
        }
        return false;
    }

    TraceHit TraceLine(const Vec3& from, const Vec3& to) const override
    {
        TraceHit result;
        result.end = to;

        Vec3 delta = to - from;
        const float length = Length(delta);
        if (length < 1e-5f) return result;

        const Vec3 dir = delta / length;
        float best = length;

        auto consider = [&](float distance, int entity, bool world, HitboxGroup group)
        {
            if (distance > best) return;
            best = distance;
            result.hit = true;
            result.world = world;
            result.entity = entity;
            result.group = group;
            result.end = from + dir * distance;
            result.fraction = distance / length;
        };

        for (const Box& wall : walls)
        {
            float distance = 0.0f;
            if (RayObb(from, dir, wall.center, wall.basis, wall.half, distance))
                consider(distance, -1, true, HitboxGroup::Unknown);
        }

        for (const PlayerView& player : players)
        {
            for (const HitboxView& hitbox : player.hitboxes)
            {
                float distance = 0.0f;
                if (RayObb(from, dir, hitbox.center, hitbox.basis, hitbox.half, distance))
                    consider(distance, player.index, false, hitbox.group);
            }
        }

        return result;
    }

    bool TraceToExit(Vec3& exit, const Vec3& start, const Vec3& dir, const TraceHit&) const override
    {
        Vec3 point = start;
        for (int step = 0; step < 512; ++step)
        {
            point += dir * 1.0f;
            if (!InsideWall(point))
            {
                exit = point;
                return true;
            }
        }
        return false;
    }
};

FakeWorld BaseWorld()
{
    FakeWorld world;
    world.weapon.damage = 36.0f;
    world.weapon.rangeModifier = 0.98f;
    world.weapon.armorRatio = 0.5f;
    world.weapon.spread = 0.05f;
    world.weapon.maxPenetrations = 3;
    world.players.push_back(MakePlayer(1, 3, { 1500.0f, 0.0f, 0.0f }));
    return world;
}

AntiAimConfig NoAntiAim()
{
    AntiAimConfig cfg;
    cfg.enabled = false;
    return cfg;
}

// ── tests ────────────────────────────────────────────────────────────────────

void TestMath()
{
    std::printf("math\n");

    const Vec3 forward = ForwardVector({ 0.0f, 90.0f, 0.0f });
    CHECK_NEAR(forward.x, 0.0f, 1e-4f);
    CHECK_NEAR(forward.y, 1.0f, 1e-4f);
    CHECK_NEAR(forward.z, 0.0f, 1e-4f);

    const Angles angles = VectorAngles({ 0.0f, 1.0f, 0.0f });
    CHECK_NEAR(angles.yaw, 90.0f, 1e-4f);
    CHECK_NEAR(angles.pitch, 0.0f, 1e-4f);

    CHECK_NEAR(NormalizeYaw(540.0f), -180.0f, 1e-4f);
    CHECK_NEAR(NormalizeYaw(-540.0f), -180.0f, 1e-4f);

    float distance = 0.0f;
    CHECK(RayObb({ 0.0f, 0.0f, 0.0f }, { 1.0f, 0.0f, 0.0f }, { 10.0f, 0.0f, 0.0f }, Identity(), { 2.0f, 2.0f, 2.0f }, distance));
    CHECK_NEAR(distance, 8.0f, 1e-3f);
    CHECK(!RayObb({ 0.0f, 0.0f, 0.0f }, { 0.0f, 1.0f, 0.0f }, { 10.0f, 0.0f, 0.0f }, Identity(), { 2.0f, 2.0f, 2.0f }, distance));
}

void TestDamageModel()
{
    std::printf("damage model\n");

    CHECK(HitboxMultiplier(HitboxGroup::Head) > HitboxMultiplier(HitboxGroup::Chest));
    CHECK(HitboxMultiplier(HitboxGroup::Stomach) > HitboxMultiplier(HitboxGroup::Legs));
    CHECK_NEAR(DamageFalloff(100.0f, 0.98f, 0.0f), 100.0f, 1e-3f);
    CHECK(DamageFalloff(100.0f, 0.98f, 1000.0f) < 100.0f);
    CHECK_NEAR(ApplyArmor(100.0f, 0, 0.5f), 100.0f, 1e-3f);
    CHECK(ApplyArmor(100.0f, 100, 0.5f) < 100.0f);
    CHECK_NEAR(ApplyArmor(10.0f, 100, 0.5f), 5.0f, 1e-3f);

    CHECK_NEAR(Hitchance(0.0f, 3.0f, 1000.0f), 100.0f, 1e-3f);
    CHECK(Hitchance(0.05f, 3.0f, 500.0f) > Hitchance(0.05f, 3.0f, 3000.0f));
    CHECK(Hitchance(0.05f, 5.0f, 1000.0f) > Hitchance(0.05f, 2.0f, 1000.0f));
    CHECK(Hitchance(5.0f, 3.0f, 1000.0f) < 1.0f);
}

void TestAimPoints()
{
    std::printf("aim points\n");

    const PlayerView player = MakePlayer(1, 3, { 1500.0f, 0.0f, 0.0f });
    RageConfig cfg;

    const auto points = RageBot::BuildAimPoints(player, cfg);
    CHECK(!points.empty());
    CHECK(points.front().group == HitboxGroup::Head);
    CHECK(points.front().center);
    CHECK(points.size() == 27); // three hitboxes, center + eight multipoints each

    int headCenters = 0;
    for (const AimPoint& point : points)
    {
        if (point.group == HitboxGroup::Head && point.center) headCenters++;
        CHECK(point.radius > 0.0f);
    }
    CHECK(headCenters == 1);

    cfg.multipoint = false;
    const auto onlyCenters = RageBot::BuildAimPoints(player, cfg);
    CHECK(onlyCenters.size() == 3);
}

void TestDirectHit()
{
    std::printf("direct hit\n");

    FakeWorld world = BaseWorld();
    RageConfig cfg;
    RageBot bot;

    const RageCommand cmd = bot.Update(world, cfg, NoAntiAim(), 1);

    CHECK(cmd.plan.valid);
    CHECK(cmd.plan.target == 1);
    CHECK(cmd.plan.group == HitboxGroup::Head);
    CHECK(cmd.plan.penetrations == 0);
    CHECK(!cmd.plan.throughWall);
    CHECK(cmd.fire);
    CHECK_NEAR(cmd.plan.angles.yaw, 0.0f, 0.5f);
    CHECK(cmd.plan.angles.pitch < 0.0f);
    CHECK(cmd.plan.damage > 50.0f);
    CHECK(cmd.plan.hitchance > cfg.hitchance);
}

void TestTeamAndDead()
{
    std::printf("team / dead filter\n");

    FakeWorld world = BaseWorld();
    world.players.push_back(MakePlayer(2, 2, { 600.0f, 0.0f, 0.0f }));  // teammate, closer
    PlayerView dead = MakePlayer(3, 3, { 700.0f, 0.0f, 0.0f });
    dead.alive = false;
    world.players.push_back(dead);

    RageBot bot;
    const RageCommand cmd = bot.Update(world, RageConfig{}, NoAntiAim(), 1);

    CHECK(cmd.plan.valid);
    CHECK(cmd.plan.target == 1);
}

void TestWallRequiresAutoWall()
{
    std::printf("wall + autoWall gate\n");

    FakeWorld world = BaseWorld();
    world.walls.push_back({ { 800.0f, 0.0f, 40.0f }, { 15.0f, 200.0f, 100.0f } });

    RageConfig cfg;
    cfg.autoWall = false;
    RageBot bot;

    const RageCommand blocked = bot.Update(world, cfg, NoAntiAim(), 1);
    CHECK(!blocked.plan.valid);
    CHECK(!blocked.fire);

    cfg.autoWall = true;
    const RageCommand through = bot.Update(world, cfg, NoAntiAim(), 2);

    CHECK(through.plan.valid);
    CHECK(through.plan.throughWall);
    CHECK(through.plan.penetrations == 1);
    CHECK(through.plan.damage < 100.0f);
    CHECK(through.fire);
}

void TestThickWall()
{
    std::printf("thick wall\n");

    FakeWorld world = BaseWorld();
    world.walls.push_back({ { 800.0f, 0.0f, 40.0f }, { 600.0f, 200.0f, 100.0f } });

    RageConfig cfg;
    RageBot bot;
    const RageCommand cmd = bot.Update(world, cfg, NoAntiAim(), 1);

    CHECK(!cmd.plan.valid);
    CHECK(!cmd.fire);
}

void TestHitchanceGate()
{
    std::printf("hitchance gate\n");

    FakeWorld world = BaseWorld();
    world.weapon.spread = 6.0f;

    RageConfig cfg;
    cfg.hitchance = 55.0f;
    RageBot bot;

    const RageCommand cmd = bot.Update(world, cfg, NoAntiAim(), 1);
    CHECK(!cmd.plan.valid);
    CHECK(!cmd.fire);

    world.weapon.spread = 0.0f;
    const RageCommand perfect = bot.Update(world, cfg, NoAntiAim(), 2);
    CHECK(perfect.plan.valid);
    CHECK_NEAR(perfect.plan.hitchance, 100.0f, 0.01f);
}

void TestFov()
{
    std::printf("fov gate\n");

    FakeWorld world = BaseWorld();
    world.view.yaw = 90.0f; // target sits at yaw 0

    RageConfig cfg;
    cfg.fov = 30.0f;
    RageBot bot;

    CHECK(!bot.Update(world, cfg, NoAntiAim(), 1).plan.valid);

    world.view.yaw = 0.0f;
    CHECK(bot.Update(world, cfg, NoAntiAim(), 2).plan.valid);
}

void TestResolver()
{
    std::printf("resolver\n");

    RageConfig cfg;
    Resolver resolver;
    PlayerView player = MakePlayer(1, 3, { 0.0f, 0.0f, 0.0f });

    resolver.Observe(1, player);
    CHECK_NEAR(resolver.Offset(1, cfg, 0), 0.0f, 1e-5f);

    resolver.OnMiss(1);
    const float first = resolver.Offset(1, cfg, 0);
    CHECK_NEAR(first, 30.0f, 1e-5f);

    resolver.OnMiss(1);
    const float second = resolver.Offset(1, cfg, 0);
    CHECK(second != first);

    resolver.OnHit(1);
    CHECK_NEAR(resolver.Offset(1, cfg, 0), 0.0f, 1e-5f);

    player.hitboxes[0].center = player.origin + Vec3(10.0f, 0.0f, 70.0f);
    Resolver::RotatePlayer(player, 90.0f);
    CHECK_NEAR(player.hitboxes[0].center.x, 0.0f, 0.01f);
    CHECK_NEAR(player.hitboxes[0].center.y, 10.0f, 0.01f);
    CHECK_NEAR(player.hitboxes[0].center.z, 70.0f, 0.01f);
    CHECK_NEAR(player.hitboxes[0].basis.right.x, -1.0f, 0.01f);
}

void TestAntiAim()
{
    std::printf("anti-aim\n");

    AntiAimConfig cfg;
    cfg.pitch = PitchMode::Down;
    cfg.yaw = YawMode::Jitter;
    cfg.base = YawBase::View;
    cfg.desync = true;
    cfg.jitterBody = true;
    cfg.breakLby = false;
    cfg.onGroundOnly = false;

    AntiAim aa;
    AntiAimInput input;
    input.yaw = 0.0f;
    input.pitch = 0.0f;
    input.onGround = true;
    input.tick = 1;

    const AntiAimOutput first = aa.Update(cfg, input);
    CHECK(first.active);
    CHECK_NEAR(first.angles.pitch, 89.0f, 1e-3f);
    CHECK_NEAR(std::fabs(first.angles.yaw), 60.0f, 1e-3f);
    CHECK_NEAR(std::fabs(first.desync), 58.0f, 1e-3f);

    input.tick = 2;
    const AntiAimOutput second = aa.Update(cfg, input);
    CHECK(first.angles.yaw * second.angles.yaw < 0.0f);
    CHECK_NEAR(first.desync, -second.desync, 1e-3f);

    AntiAimConfig spinCfg;
    spinCfg.pitch = PitchMode::Zero;
    spinCfg.yaw = YawMode::Spin;
    spinCfg.spinSpeed = 10.0f;
    spinCfg.desync = false;
    spinCfg.onGroundOnly = false;

    AntiAim spinner;
    input.tick = 1;
    CHECK_NEAR(spinner.Update(spinCfg, input).angles.yaw, 10.0f, 1e-3f);
    input.tick = 2;
    CHECK_NEAR(spinner.Update(spinCfg, input).angles.yaw, 20.0f, 1e-3f);

    AntiAimConfig upCfg = spinCfg;
    upCfg.yaw = YawMode::Off;
    upCfg.pitch = PitchMode::Up;
    AntiAim upper;
    CHECK_NEAR(upper.Update(upCfg, input).angles.pitch, -89.0f, 1e-3f);

    AntiAimConfig offCfg;
    offCfg.enabled = false;
    input.yaw = 42.0f;
    const AntiAimOutput off = aa.Update(offCfg, input);
    CHECK(!off.active);
    CHECK_NEAR(off.angles.yaw, 42.0f, 1e-4f);

    AntiAimConfig hideCfg = spinCfg;
    hideCfg.pitch = PitchMode::Off;
    hideCfg.yaw = YawMode::Off;
    hideCfg.hideShots = true;
    input.firing = true;
    CHECK(!aa.Update(hideCfg, input).sendPacket);
    input.firing = false;
    CHECK(aa.Update(hideCfg, input).sendPacket);
}

} // namespace

int main()
{
    std::printf("rage solver tests\n\n");

    TestMath();
    TestDamageModel();
    TestAimPoints();
    TestDirectHit();
    TestTeamAndDead();
    TestWallRequiresAutoWall();
    TestThickWall();
    TestHitchanceGate();
    TestFov();
    TestResolver();
    TestAntiAim();

    std::printf("\n%d checks, %d failures\n", g_checks, g_failures);
    return g_failures == 0 ? 0 : 1;
}
