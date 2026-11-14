#pragma once

#include "Math.h"

namespace rage
{
enum class PitchMode : int
{
    Off = 0,
    Down,
    Up,
    Zero,
    Jitter,
    FakeDown,
};

enum class YawMode : int
{
    Off = 0,
    Jitter,
    Spin,
    Sideways,
    Backwards,
    Static,
    Random,
};

enum class YawBase : int
{
    View = 0,      // relative to current view angles
    Movement,      // relative to movement direction while moving
    LastMovement,  // relative to the last movement direction, even while standing
};

struct AntiAimConfig
{
    bool enabled = true;

    PitchMode pitch = PitchMode::Down;
    YawMode yaw = YawMode::Jitter;
    YawBase base = YawBase::View;

    float pitchDown = 89.0f;
    float pitchJitter = 45.0f;
    float yawJitter = 60.0f;
    float spinSpeed = 18.0f;    // degrees per tick
    float staticYaw = 180.0f;
    float roll = 0.0f;

    bool desync = true;
    float desyncLimit = 58.0f;  // degrees, matches the CS body-yaw limit
    bool jitterBody = true;     // alternate the lean side every tick
    bool breakLby = true;       // flip the lean periodically while standing

    bool hideShots = true;      // signal packet choke on the firing tick
    bool onGroundOnly = true;   // do not run the anti-aim in the air
};

struct AntiAimInput
{
    float yaw = 0.0f;
    float pitch = 0.0f;
    Vec3 velocity;
    bool onGround = true;
    bool firing = false;
    int tick = 0;
};

struct AntiAimOutput
{
    Angles angles;
    float bodyYaw = 0.0f;   // server-side body direction predicted from `angles`
    float desync = 0.0f;    // signed offset between view yaw and body yaw
    float roll = 0.0f;
    bool sendPacket = true;
    bool active = false;
};

// Deterministic anti-aim generator. The host calls Update once per CreateMove,
// writes `angles` into CUserCmd and uses `bodyYaw`/`desync` for its own model
// prediction. No game state is touched, so the module is unit-testable.
class AntiAim
{
public:
    AntiAimOutput Update(const AntiAimConfig& cfg, const AntiAimInput& in);
    void Reset();

private:
    float _spin = 0.0f;
    float _jitterSign = 1.0f;
    float _leanSign = 1.0f;
    float _lastMoveYaw = 0.0f;
    int _lbyTicks = 0;
};

} // namespace rage
