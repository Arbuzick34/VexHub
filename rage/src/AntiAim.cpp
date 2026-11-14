#include "rage/AntiAim.h"

#include <cmath>
#include <cstdint>

namespace rage
{
namespace
{
// Small deterministic PRNG so a `Random` yaw is reproducible in tests and demos.
float RandomUnit(int seed)
{
    std::uint32_t x = static_cast<std::uint32_t>(seed) * 2654435761u + 0x9E3779B9u;
    x ^= x >> 16;
    x *= 0x7FEB352Du;
    x ^= x >> 15;
    x *= 0x846CA68Bu;
    x ^= x >> 16;
    return static_cast<float>(x & 0xFFFFFFu) / 16777215.0f;
}
} // namespace

void AntiAim::Reset()
{
    _spin = 0.0f;
    _jitterSign = 1.0f;
    _leanSign = 1.0f;
    _lastMoveYaw = 0.0f;
    _lbyTicks = 0;
}

AntiAimOutput AntiAim::Update(const AntiAimConfig& cfg, const AntiAimInput& in)
{
    AntiAimOutput out;
    out.angles = { in.pitch, in.yaw, 0.0f };
    out.bodyYaw = in.yaw;
    out.sendPacket = true;
    out.active = false;

    if (!cfg.enabled) return out;
    if (cfg.onGroundOnly && !in.onGround) return out;

    const float speed = std::sqrt(in.velocity.x * in.velocity.x + in.velocity.y * in.velocity.y);
    if (speed > 5.0f) _lastMoveYaw = std::atan2(in.velocity.y, in.velocity.x) * kRadToDeg;

    float baseYaw = in.yaw;
    switch (cfg.base)
    {
    case YawBase::View:
        baseYaw = in.yaw;
        break;
    case YawBase::Movement:
        baseYaw = speed > 5.0f ? _lastMoveYaw : in.yaw;
        break;
    case YawBase::LastMovement:
        baseYaw = _lastMoveYaw;
        break;
    }

    _jitterSign = -_jitterSign;
    _leanSign = (in.tick & 1) ? 1.0f : -1.0f;

    float yaw = baseYaw;
    switch (cfg.yaw)
    {
    case YawMode::Off:
        yaw = in.yaw;
        break;
    case YawMode::Jitter:
        yaw = baseYaw + _jitterSign * std::fabs(cfg.yawJitter);
        break;
    case YawMode::Spin:
        _spin = NormalizeYaw(_spin + cfg.spinSpeed);
        yaw = baseYaw + _spin;
        break;
    case YawMode::Sideways:
        yaw = baseYaw + 90.0f * _leanSign;
        break;
    case YawMode::Backwards:
        yaw = baseYaw + 180.0f;
        break;
    case YawMode::Static:
        yaw = baseYaw + cfg.staticYaw;
        break;
    case YawMode::Random:
        yaw = baseYaw + RandomUnit(in.tick) * 360.0f;
        break;
    }

    float pitch = in.pitch;
    switch (cfg.pitch)
    {
    case PitchMode::Off:
        pitch = in.pitch;
        break;
    case PitchMode::Down:
        pitch = std::fabs(cfg.pitchDown);
        break;
    case PitchMode::Up:
        pitch = -std::fabs(cfg.pitchDown);
        break;
    case PitchMode::Zero:
        pitch = 0.0f;
        break;
    case PitchMode::Jitter:
        pitch = (in.tick & 1 ? 1.0f : -1.0f) * std::fabs(cfg.pitchJitter);
        break;
    case PitchMode::FakeDown:
        pitch = (in.tick & 1) ? std::fabs(cfg.pitchDown) : 0.0f;
        break;
    }

    float desync = 0.0f;
    if (cfg.desync)
    {
        const float sign = cfg.jitterBody ? _leanSign : 1.0f;
        desync = std::fabs(cfg.desyncLimit) * sign;

        // Standing still: the body yaw updates in steps, flip the lean when the
        // server would refresh it so the real angle is never settled.
        if (cfg.breakLby && speed <= 5.0f && in.onGround)
        {
            if (++_lbyTicks >= 16)
            {
                _lbyTicks = 0;
                desync = -desync;
            }
        }
        else
        {
            _lbyTicks = 0;
        }
    }

    out.angles = Normalize(Angles{ pitch, yaw, cfg.roll });
    out.desync = desync;
    out.bodyYaw = NormalizeYaw(yaw + desync);
    out.roll = cfg.roll;
    out.sendPacket = !(cfg.hideShots && in.firing);
    out.active = true;
    return out;
}

} // namespace rage
