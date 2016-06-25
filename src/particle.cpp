#include <onut/Curve.h>
#include <onut/Timing.h>
#include <onut/SpriteBatch.h>
#include <onut/Random.h>
#include <onut/Texture.h>
#include "particle.h"

Particles particles;

void spawnParticles(const Particle& templateParticle, int count, float spread, float angleRandom, float velRandom, float angleVelRandom)
{
    for (int i = 0; i < count; ++i)
    {
        auto dir = templateParticle.vel;
        auto velSize = dir.Length();
        if (velSize > 0.0f)
        {
            dir /= velSize;
            float startAngle = std::atan2f(dir.y, dir.x);
            startAngle += DirectX::XMConvertToRadians(ORandFloat(-spread, spread));
            dir.x = std::cosf(startAngle) * velSize;
            dir.y = std::sinf(startAngle) * velSize;
        }
        particles.push_back({
            templateParticle.position,
            dir,
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
