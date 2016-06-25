#pragma once
#include <onut/Maths.h>
#include <vector>

#include <onut/ForwardDeclaration.h>
OForwardDeclare(Texture);

struct Particle
{
    Vector2 position;
    Vector2 vel;
    float life;
    float duration;
    Color colorFrom, colorTo;
    float sizeFrom, sizeTo;
    float angle;
    float angleVel;
    OTextureRef pTexture;
};

void spawnParticles(const Particle& templateParticle, int count, float spread, float angleRandom, float velRandom, float angleVelRandom);
void updateParticles();
void drawParticles();

using Particles = std::vector<Particle>;

extern Particles particles;
