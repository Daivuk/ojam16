#include <onut/SpriteBatch.h>
#include <onut/Renderer.h>
#include <onut/Timing.h>

#include "part.h"
#include "particle.h"

PartDef partDefs[PART_COUNT];
Parts parts;
Part* pMainPart = nullptr;
std::vector<std::vector<Part*>> stages;

#define GRAVITY 3.0f

OTextureRef pEngineCoverTexture;
OTextureRef pFireTexture;
OTextureRef pSmokeTexture;
float shakeAmount = 0;
float globalStability = 0;

#define DEF_ATTACH_POINT(__part__, __x__, __y__) partDefs[__part__].attachPoints.push_back((Vector2(__x__, __y__) - Vector2(partDefs[__part__].pTexture->getSizef() / 2)) / 64)

void initPartDefs()
{
    pEngineCoverTexture = OGetTexture("engineCover.png");
    pFireTexture = OGetTexture("PARTICLE_FIRE.png");
    pSmokeTexture = OGetTexture("PARTICLE_SMOKE.png");

    partDefs[PART_TOP_CONE].pTexture = OGetTexture("PART_TOP_CONE.png");
    partDefs[PART_TOP_CONE].hsize = partDefs[PART_TOP_CONE].pTexture->getSizef() / 128.0f;
    DEF_ATTACH_POINT(PART_TOP_CONE, 32, 62);
    partDefs[PART_TOP_CONE].weight = 5;
    partDefs[PART_TOP_CONE].name = "Payload";
    partDefs[PART_TOP_CONE].price = 0;
    partDefs[PART_TOP_CONE].isStaged = true;

    partDefs[PART_SOLID_ROCKET].pTexture = OGetTexture("PART_SOLID_ROCKET.png");
    partDefs[PART_SOLID_ROCKET].hsize = partDefs[PART_SOLID_ROCKET].pTexture->getSizef() / 128.0f;
    DEF_ATTACH_POINT(PART_SOLID_ROCKET, 32, 4);
    DEF_ATTACH_POINT(PART_SOLID_ROCKET, 32, 96);
    DEF_ATTACH_POINT(PART_SOLID_ROCKET, 2, 32);
    DEF_ATTACH_POINT(PART_SOLID_ROCKET, 62, 32);
    partDefs[PART_SOLID_ROCKET].weight = 5;
    partDefs[PART_SOLID_ROCKET].name = "Solid Fuel Rocket";
    partDefs[PART_SOLID_ROCKET].price = 200;
    partDefs[PART_SOLID_ROCKET].solidFuel = 10;
    partDefs[PART_SOLID_ROCKET].isStaged = true;
    partDefs[PART_SOLID_ROCKET].trust = 100;

    partDefs[PART_DECOUPLER].pTexture = OGetTexture("PART_DECOUPLER.png");
    partDefs[PART_DECOUPLER].hsize = partDefs[PART_DECOUPLER].pTexture->getSizef() / 128.0f;
    DEF_ATTACH_POINT(PART_DECOUPLER, 32, 3);
    DEF_ATTACH_POINT(PART_DECOUPLER, 32, 13);
    partDefs[PART_DECOUPLER].weight = .25f;
    partDefs[PART_DECOUPLER].name = "Decoupler";
    partDefs[PART_DECOUPLER].price = 75;
    partDefs[PART_DECOUPLER].isStaged = true;

    partDefs[PART_CONE].pTexture = OGetTexture("PART_CONE.png");
    partDefs[PART_CONE].hsize = partDefs[PART_CONE].pTexture->getSizef() / 128.0f;
    DEF_ATTACH_POINT(PART_CONE, 32, 31);
    partDefs[PART_CONE].weight = .5f;
    partDefs[PART_CONE].name = "Aerodynamic Cone";
    partDefs[PART_CONE].price = 50;
    partDefs[PART_CONE].stability = 1.25f;

    partDefs[FIN_SMALL_LEFT].pTexture = OGetTexture("FIN_SMALL_LEFT.png");
    partDefs[FIN_SMALL_LEFT].hsize = partDefs[FIN_SMALL_LEFT].pTexture->getSizef() / 128.0f;
    DEF_ATTACH_POINT(FIN_SMALL_LEFT, 31, 16);
    partDefs[FIN_SMALL_LEFT].weight = .25f;
    partDefs[FIN_SMALL_LEFT].name = "Small Fin";
    partDefs[FIN_SMALL_LEFT].price = 50;
    partDefs[FIN_SMALL_LEFT].stability = 1;

    partDefs[FIN_SMALL_RIGHT].pTexture = OGetTexture("FIN_SMALL_RIGHT.png");
    partDefs[FIN_SMALL_RIGHT].hsize = partDefs[FIN_SMALL_RIGHT].pTexture->getSizef() / 128.0f;
    DEF_ATTACH_POINT(FIN_SMALL_RIGHT, 1, 16);
    partDefs[FIN_SMALL_RIGHT].weight = .25f;
    partDefs[FIN_SMALL_RIGHT].name = "Small Fin";
    partDefs[FIN_SMALL_RIGHT].price = 50;
    partDefs[FIN_SMALL_RIGHT].stability = 1;
}

void deleteParts(Parts& parts)
{
    for (auto& part : parts)
    {
        deleteParts(part->children);
        delete part;
    }
    parts.clear();
}

struct OnTopSprite
{
    OTextureRef pTexture;
    Matrix transform;
};

extern Part* pHoverPart;

using OnTopSprites = std::vector<OnTopSprite>;
OnTopSprites onTopSprites;
OnTopSprite hoverSprite;

void drawOnTops()
{
    for (auto& onTopSprite : onTopSprites)
    {
        oSpriteBatch->drawSprite(onTopSprite.pTexture,
                                 onTopSprite.transform,
                                 Color::White);
    }
    onTopSprites.clear();
    if (pHoverPart)
    {
        oSpriteBatch->drawSprite(hoverSprite.pTexture,
                                 hoverSprite.transform,
                                 Color(1.5f, .75f, 1.5f, 1));
    }
}

Part* getTopParent(Part* pPart)
{
    if (!pPart->pParent) return pPart;
    return getTopParent(pPart->pParent);
}

void drawParts(const Matrix& parentTransform, Parts& parts, Part* pParent)
{
    for (auto pPart : parts)
    {
        auto& partDef = partDefs[pPart->type];
        Matrix transform = Matrix::CreateRotationZ(pPart->angle) * Matrix::CreateTranslation(pPart->position) * parentTransform;
        if (pPart == pHoverPart)
        {
            hoverSprite = {partDef.pTexture, Matrix::CreateScale(1.0f / 64.0f) * transform};
        }
        if (pPart->type == PART_DECOUPLER)
        {
            if (pParent)
            {
                if (pParent->type == PART_SOLID_ROCKET)
                {
                    onTopSprites.push_back({pEngineCoverTexture, Matrix::CreateScale(1.0f / 64.0f) * Matrix::CreateTranslation(0, -.35f, 0) * transform});
                }
            }
            onTopSprites.push_back({partDef.pTexture, Matrix::CreateScale(1.0f / 64.0f) * transform});
        }
        else
        {
            oSpriteBatch->drawSprite(partDef.pTexture,
                                     Matrix::CreateScale(1.0f / 64.0f) * transform,
                                     Color::White);
        }
        drawParts(transform, pPart->children, pPart);
    }
}

void drawAnchors(const Matrix& parentTransform, Parts& parts)
{
    for (auto pPart : parts)
    {
        auto& partDef = partDefs[pPart->type];
        Matrix transform = Matrix::CreateRotationZ(pPart->angle) * Matrix::CreateTranslation(pPart->position) * parentTransform;
        oSpriteBatch->begin(transform);
        for (auto& attachPoint : partDef.attachPoints)
        {
            oSpriteBatch->drawCross(attachPoint, .2f, Color(0, 1, 1), .05f);
        }
        drawParts(transform, pPart->children);
        oSpriteBatch->end();
    }
}

Rect vehiculeRect(Part* pPart, const Vector2& parentPos)
{
    Rect ret;
    auto& partDef = partDefs[pPart->type];
    ret.x = parentPos.x + pPart->position.x - partDef.hsize.x;
    ret.y = parentPos.y + pPart->position.y - partDef.hsize.y;
    ret.z = partDef.hsize.x * 2;
    ret.w = partDef.hsize.y * 2;
    for (auto pChild : pPart->children)
    {
        auto childRect = vehiculeRect(pChild, parentPos + pPart->position);
        Rect newRect(
            std::min(childRect.x, ret.x),
            std::min(childRect.y, ret.y),
            std::max(childRect.x + childRect.z, ret.x + ret.z) - std::min(childRect.x, ret.x),
            std::max(childRect.y + childRect.w, ret.y + ret.w) - std::min(childRect.y, ret.y));
        ret = newRect;
    }
    return ret;
}

Part* mouseHoverPart(Part* pPart, const Vector2& mousePos, const Vector2& parentPos)
{
    auto& partDef = partDefs[pPart->type];
    Rect ret;
    ret.x = parentPos.x + pPart->position.x - partDef.hsize.x;
    ret.y = parentPos.y + pPart->position.y - partDef.hsize.y;
    ret.z = partDef.hsize.x * 2;
    ret.w = partDef.hsize.y * 2;
    ret = ret.Grow(.1f);
    for (auto pChild : pPart->children)
    {
        auto pRet = mouseHoverPart(pChild, mousePos, parentPos + pPart->position);
        if (pRet)
        {
            return pRet;
        }
    }
    if (ret.Contains(mousePos)) return pPart;
    return nullptr;
}

struct Force
{
    Vector2 force;
    Vector2 position;
};
float totalMass = 0;
Vector2 centerOfMass;
using Forces = std::vector<Force>;
Forces forces;

Matrix getWorldTransform(Part* pPart)
{
    Matrix transform;
    if (pPart->pParent)
    {
        transform = Matrix::CreateRotationZ(pPart->angle) * Matrix::CreateTranslation(pPart->position) * getWorldTransform(pPart->pParent);
    }
    else
    {
        transform = Matrix::CreateRotationZ(pPart->angle) * Matrix::CreateTranslation(pPart->position);
    }
    return std::move(transform);
}

float getTotalMass(Part* pPart)
{
    float ret = 0;
    auto& partDef = partDefs[pPart->type];
    ret += partDef.weight;
    for (auto pChild : pPart->children) ret += getTotalMass(pChild);
    return ret;
}

float getTotalStability(Part* pPart)
{
    float ret = 0;
    auto& partDef = partDefs[pPart->type];
    ret += partDef.stability;
    for (auto pChild : pPart->children) ret += getTotalStability(pChild);
    return ret;
}

void updatePart(Part* pPart)
{
    auto& partDef = partDefs[pPart->type];
    if (!pPart->pParent)
    {
        forces.clear();
        totalMass = 0;
        centerOfMass = Vector2::Zero;
        shakeAmount = 0;
        globalStability = 0;
    }
    else
    {
        centerOfMass += pPart->position * partDef.weight;
    }

    globalStability += partDef.stability;
    totalMass += partDef.weight;

    if (pPart->pParent)
    {
        // Copy parent's physic
        pPart->vel = pPart->pParent->vel;
        pPart->angleVelocity = pPart->pParent->angleVelocity;
    }

    if (pPart->isActive)
    {
        switch (pPart->type)
        {
            case PART_SOLID_ROCKET:
            {
                if (pPart->solidFuel > 0)
                {
                    shakeAmount += 1;
                    pPart->solidFuel -= ODT;
                    auto transform = getWorldTransform(pPart);
                    auto worldPos = transform.Translation();
                    auto forward = transform.Up();
                    forward *= -1;
                    forward.Normalize();
                    worldPos -= forward * .75f;
                    forces.push_back({forward * partDef.trust, worldPos});
                    if (pPart->solidFuel <= 0.0f)
                    {
                        pPart->solidFuel = 0;
                    }
                }
                break;
            }
        }
    }

    for (auto pChild : pPart->children)
    {
        updatePart(pChild);
    }

    // Finalize update and physic of the main body
    if (!pPart->pParent)
    {
        auto dirToPlanet = -pPart->position;
        dirToPlanet.Normalize();

        // Apply different forces
        // f = ma
        // a = f / m
        centerOfMass /= totalMass;
        auto transformMe = getWorldTransform(pPart);
        auto worldCenterOfMass = Vector2::Transform(centerOfMass, transformMe);
        auto right = Vector2(transformMe.Right());
        right.Normalize();
        for (int i = 0; i < (int)forces.size(); ++i)
        {
            auto& force = forces[i];
            Vector2 dirToCenterOfMass = force.position - worldCenterOfMass;
            dirToCenterOfMass.Normalize();
            auto forceDir = force.force;
            forceDir.Normalize();
            float directEffect = std::fabsf(dirToCenterOfMass.y);
            float angularEffect = dirToCenterOfMass.Dot(right);
            pPart->vel += Vector2(force.force / totalMass) * ODT;
            pPart->angleVelocity -= (angularEffect / totalMass * 100) * ODT;
        }

        pPart->angle += pPart->angleVelocity * ODT;
        pPart->vel += dirToPlanet * GRAVITY * ODT;
        pPart->position += pPart->vel * ODT;
        if (pPart->angleVelocity > 0)
        {
            pPart->angleVelocity -= globalStability / totalMass * 4 * ODT;
            if (pPart->angleVelocity < 0)
            {
                pPart->angleVelocity = 0;
            }
        }
        else if (pPart->angleVelocity < 0)
        {
            pPart->angleVelocity += globalStability / totalMass * 4 * ODT;
            if (pPart->angleVelocity > 0)
            {
                pPart->angleVelocity = 0;
            }
        }
    }

    if (pPart->isActive)
    {
        switch (pPart->type)
        {
            case PART_SOLID_ROCKET:
            {
                if (pPart->solidFuel > 0)
                {
                    auto transform = getWorldTransform(pPart);
                    auto worldPos = transform.Translation();
                    auto forward = transform.Up();
                    forward *= -1;
                    forward.Normalize();
                    worldPos -= forward * .75f;
                    spawnParticles({
                        worldPos,
                        pPart->vel - Vector2(forward * 10.0f),
                        0,
                        .25f,
                        Color(1, 1, 0, 1), Color(0, 0, 0, 0),
                        .5f, 2.0f,
                        2.0f,
                        45.0f,
                        pFireTexture
                    }, 2, 10.0f, 360.0f, 0, 0, -forward);
                    spawnParticles({
                        worldPos,
                        Vector2::Zero,
                        0,
                        1,
                        Color(1, 1, 1, 1), Color(0, 0, 0, 0),
                        .5f, 10.0f,
                        2.0f,
                        5.0f,
                        pSmokeTexture
                    }, 1, 0, 360.0f, 0, 0, -forward);
                }
                break;
            }
        }
    }
}
