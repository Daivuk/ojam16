#include <onut/Curve.h>
#include <onut/Timing.h>
#include <onut/SpriteBatch.h>
#include <onut/Random.h>
#include <onut/Texture.h>
#include "particle.h"

Particles particles;

void spawnParticles(const Particle& templateParticle, int count, float spread, float angleRandom, float velRandom, float angleVelRandom, const Vector2& in_dir)
{
    auto dir = in_dir;
    dir.Normalize();
    for (int i = 0; i < count; ++i)
    {
        auto velSize = dir.Length();
        if (velSize > 0.0f)
        {
            float startAngle = std::atan2f(dir.y, dir.x);
            startAngle += DirectX::XMConvertToRadians(ORandFloat(-spread, spread));
            auto velDir = templateParticle.vel;
            velDir.Normalize();
            auto dot = velDir.Dot(dir);
            dot = 1 - std::fabsf(dot);
            dir.x = std::cosf(startAngle) * velSize * dot;
            dir.y = std::sinf(startAngle) * velSize * dot;
        }
        particles.push_back({
            templateParticle.position,
            templateParticle.vel + dir,
            0,
            templateParticle.duration,
            templateParticle.colorFrom, templateParticle.colorTo,
            templateParticle.sizeFrom, templateParticle.sizeTo,
            templateParticle.angle + ORandFloat(-angleRandom, angleRandom),
            templateParticle.angleVel + ORandFloat(-angleVelRandom, angleVelRandom),
            templateParticle.pTexture
        });
    }
}

void updateParticles()
{
    auto dt = ODT;
    for (auto it = particles.begin(); it != particles.end();)
    {
        auto& particle = *it;
        particle.life += (1.0f / particle.duration) * dt;
        particle.angle += particle.angleVel * dt;
        particle.position += particle.vel * dt;
        if (particle.life >= 1.0f)
        {
            it = particles.erase(it);
            continue;
        }
        ++it;
    }
}

void drawParticles()
{
    for (auto& particle : particles)
    {
        auto color = OLerp(particle.colorFrom, particle.colorTo, particle.life);
        auto size = OLerp(particle.sizeFrom, particle.sizeTo, particle.life);
        size /= particle.pTexture->getSizef().x;
        oSpriteBatch->drawSprite(particle.pTexture, particle.position, color, particle.angle, size);
    }
}
