#include <onut/Curve.h>
#include <onut/SpriteBatch.h>
#include <onut/Renderer.h>
#include <onut/Timing.h>
#include <onut/Random.h>
#include <onut/Sound.h>

#include "part.h"
#include "particle.h"
#include "defines.h"

PartDef partDefs[PART_COUNT + 1];
Parts parts;
Part* pMainPart = nullptr;
std::vector<std::vector<Part*>> stages;

OTextureRef pEngineCoverTexture;
OTextureRef pEngineCoverWideTexture;
OTextureRef pFireTexture;
OTextureRef pBlueFireTexture;
OTextureRef pDebrisTexture;
OTextureRef pSmokeTexture;
float shakeAmount = 0;
float globalStability = 0;

#define DEF_ATTACH_POINT(__part__, __x__, __y__) partDefs[__part__].attachPoints.push_back((Vector2(__x__, __y__) - Vector2(partDefs[__part__].pTexture->getSizef() / 2)) / 64)

void initPartDefs()
{
    pEngineCoverTexture = OGetTexture("PART_ENGINE_COVER.png");
    pEngineCoverWideTexture = OGetTexture("PART_ENGINE_COVER_WIDE.png");
    pFireTexture = OGetTexture("PARTICLE_FIRE.png");
    pSmokeTexture = OGetTexture("PARTICLE_SMOKE.png");
    pBlueFireTexture = OGetTexture("PARTICLE_BLUE_FLAME.png");
    pDebrisTexture = OGetTexture("PART_DECOUPLER_HORIZONTAL_LEFT.png");

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

    partDefs[PART_DECOUPLER_WIDE].pTexture = OGetTexture("PART_DECOUPLER_WIDE.png");
    partDefs[PART_DECOUPLER_WIDE].hsize = partDefs[PART_DECOUPLER_WIDE].pTexture->getSizef() / 128.0f;
    DEF_ATTACH_POINT(PART_DECOUPLER_WIDE, 64, 3);
    DEF_ATTACH_POINT(PART_DECOUPLER_WIDE, 64, 13);
    partDefs[PART_DECOUPLER_WIDE].weight = .5f;
    partDefs[PART_DECOUPLER_WIDE].name = "Decoupler";
    partDefs[PART_DECOUPLER_WIDE].price = 150;
    partDefs[PART_DECOUPLER_WIDE].isStaged = true;

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

    partDefs[FIN_MEDIUM_LEFT].pTexture = OGetTexture("FIN_MEDIUM_LEFT.png");
    partDefs[FIN_MEDIUM_LEFT].hsize = partDefs[FIN_MEDIUM_LEFT].pTexture->getSizef() / 128.0f;
    DEF_ATTACH_POINT(FIN_MEDIUM_LEFT, 63, 16);
    partDefs[FIN_MEDIUM_LEFT].weight = .5f;
    partDefs[FIN_MEDIUM_LEFT].name = "Medium Fin";
    partDefs[FIN_MEDIUM_LEFT].price = 100;
    partDefs[FIN_MEDIUM_LEFT].stability = 1.5;

    partDefs[FIN_MEDIUM_RIGHT].pTexture = OGetTexture("FIN_MEDIUM_RIGHT.png");
    partDefs[FIN_MEDIUM_RIGHT].hsize = partDefs[FIN_MEDIUM_RIGHT].pTexture->getSizef() / 128.0f;
    DEF_ATTACH_POINT(FIN_MEDIUM_RIGHT, 1, 16);
    partDefs[FIN_MEDIUM_RIGHT].weight = .5f;
    partDefs[FIN_MEDIUM_RIGHT].name = "Medium Fin";
    partDefs[FIN_MEDIUM_RIGHT].price = 100;
    partDefs[FIN_MEDIUM_RIGHT].stability = 1.5;

    partDefs[PART_DECOUPLER_HORIZONTAL_LEFT].pTexture = OGetTexture("PART_DECOUPLER_HORIZONTAL_LEFT.png");
    partDefs[PART_DECOUPLER_HORIZONTAL_LEFT].hsize = partDefs[PART_DECOUPLER_HORIZONTAL_LEFT].pTexture->getSizef() / 128.0f;
    DEF_ATTACH_POINT(PART_DECOUPLER_HORIZONTAL_LEFT, 32 - 3, 16);
    DEF_ATTACH_POINT(PART_DECOUPLER_HORIZONTAL_LEFT, 32 - 22, 16);
    partDefs[PART_DECOUPLER_HORIZONTAL_LEFT].weight = .15f;
    partDefs[PART_DECOUPLER_HORIZONTAL_LEFT].name = "Decoupler";
    partDefs[PART_DECOUPLER_HORIZONTAL_LEFT].price = 45;
    partDefs[PART_DECOUPLER_HORIZONTAL_LEFT].isStaged = true;

    partDefs[PART_DECOUPLER_HORIZONTAL_RIGHT].pTexture = OGetTexture("PART_DECOUPLER_HORIZONTAL_RIGHT.png");
    partDefs[PART_DECOUPLER_HORIZONTAL_RIGHT].hsize = partDefs[PART_DECOUPLER_HORIZONTAL_RIGHT].pTexture->getSizef() / 128.0f;
    DEF_ATTACH_POINT(PART_DECOUPLER_HORIZONTAL_RIGHT, 3, 16);
    DEF_ATTACH_POINT(PART_DECOUPLER_HORIZONTAL_RIGHT, 22, 16);
    partDefs[PART_DECOUPLER_HORIZONTAL_RIGHT].weight = .15f;
    partDefs[PART_DECOUPLER_HORIZONTAL_RIGHT].name = "Decoupler";
    partDefs[PART_DECOUPLER_HORIZONTAL_RIGHT].price = 45;
    partDefs[PART_DECOUPLER_HORIZONTAL_RIGHT].isStaged = true;

    partDefs[PART_LARGE_TO_SMALL_JOINER].pTexture = OGetTexture("PART_LARGE_TO_SMALL_JOINER.png");
    partDefs[PART_LARGE_TO_SMALL_JOINER].hsize = partDefs[PART_LARGE_TO_SMALL_JOINER].pTexture->getSizef() / 128.0f;
    DEF_ATTACH_POINT(PART_LARGE_TO_SMALL_JOINER, 64, 1);
    DEF_ATTACH_POINT(PART_LARGE_TO_SMALL_JOINER, 64, 31);
    DEF_ATTACH_POINT(PART_LARGE_TO_SMALL_JOINER, 64 - 32, 31);
    DEF_ATTACH_POINT(PART_LARGE_TO_SMALL_JOINER, 64 + 32, 31);
    partDefs[PART_LARGE_TO_SMALL_JOINER].weight = 2;
    partDefs[PART_LARGE_TO_SMALL_JOINER].name = "Small to Big connector";
    partDefs[PART_LARGE_TO_SMALL_JOINER].price = 50;

    partDefs[PART_SMALL_TO_LARGE_JOINER].pTexture = OGetTexture("PART_SMALL_TO_LARGE_JOINER.png");
    partDefs[PART_SMALL_TO_LARGE_JOINER].hsize = partDefs[PART_SMALL_TO_LARGE_JOINER].pTexture->getSizef() / 128.0f;
    DEF_ATTACH_POINT(PART_SMALL_TO_LARGE_JOINER, 64, 1);
    DEF_ATTACH_POINT(PART_SMALL_TO_LARGE_JOINER, 64 - 32, 1);
    DEF_ATTACH_POINT(PART_SMALL_TO_LARGE_JOINER, 64 + 32, 1);
    DEF_ATTACH_POINT(PART_SMALL_TO_LARGE_JOINER, 64, 31);
    partDefs[PART_SMALL_TO_LARGE_JOINER].weight = 2;
    partDefs[PART_SMALL_TO_LARGE_JOINER].name = "Big to Small connector";
    partDefs[PART_SMALL_TO_LARGE_JOINER].price = 50;

    partDefs[PART_FUEL_WIDE_TALL].pTexture = OGetTexture("PART_FUEL_WIDE_TALL.png");
    partDefs[PART_FUEL_WIDE_TALL].hsize = partDefs[PART_FUEL_WIDE_TALL].pTexture->getSizef() / 128.0f;
    DEF_ATTACH_POINT(PART_FUEL_WIDE_TALL, 64, 2);
    DEF_ATTACH_POINT(PART_FUEL_WIDE_TALL, 64, 62);
    DEF_ATTACH_POINT(PART_FUEL_WIDE_TALL, 2, 32);
    DEF_ATTACH_POINT(PART_FUEL_WIDE_TALL, 126, 32);
    partDefs[PART_FUEL_WIDE_TALL].weight = 6;
    partDefs[PART_FUEL_WIDE_TALL].liquidFuel = 20;
    partDefs[PART_FUEL_WIDE_TALL].name = "Wide/Tall Liquid Fuel";
    partDefs[PART_FUEL_WIDE_TALL].price = 50;

    partDefs[PART_FUEL_WIDE_SHORT].pTexture = OGetTexture("PART_FUEL_WIDE_SHORT.png");
    partDefs[PART_FUEL_WIDE_SHORT].hsize = partDefs[PART_FUEL_WIDE_SHORT].pTexture->getSizef() / 128.0f;
    DEF_ATTACH_POINT(PART_FUEL_WIDE_SHORT, 64, 2);
    DEF_ATTACH_POINT(PART_FUEL_WIDE_SHORT, 64, 30);
    partDefs[PART_FUEL_WIDE_SHORT].weight = 4;
    partDefs[PART_FUEL_WIDE_SHORT].liquidFuel = 10;
    partDefs[PART_FUEL_WIDE_SHORT].name = "Wide/Short Liquid Fuel";
    partDefs[PART_FUEL_WIDE_SHORT].price = 50;

    partDefs[PART_FUEL_THIN_SHORT].pTexture = OGetTexture("PART_FUEL_THIN_SHORT.png");
    partDefs[PART_FUEL_THIN_SHORT].hsize = partDefs[PART_FUEL_THIN_SHORT].pTexture->getSizef() / 128.0f;
    DEF_ATTACH_POINT(PART_FUEL_THIN_SHORT, 32, 2);
    DEF_ATTACH_POINT(PART_FUEL_THIN_SHORT, 32, 30);
    partDefs[PART_FUEL_THIN_SHORT].weight = 2;
    partDefs[PART_FUEL_THIN_SHORT].liquidFuel = 5;
    partDefs[PART_FUEL_THIN_SHORT].name = "Thin/Short Liquid Fuel";
    partDefs[PART_FUEL_THIN_SHORT].price = 50;

    partDefs[PART_FUEL_THIN_TALL].pTexture = OGetTexture("PART_FUEL_THIN_TALL.png");
    partDefs[PART_FUEL_THIN_TALL].hsize = partDefs[PART_FUEL_THIN_TALL].pTexture->getSizef() / 128.0f;
    DEF_ATTACH_POINT(PART_FUEL_THIN_TALL, 32, 2);
    DEF_ATTACH_POINT(PART_FUEL_THIN_TALL, 32, 62);
    DEF_ATTACH_POINT(PART_FUEL_THIN_TALL, 2, 32);
    DEF_ATTACH_POINT(PART_FUEL_THIN_TALL, 62, 32);
    partDefs[PART_FUEL_THIN_TALL].weight = 6;
    partDefs[PART_FUEL_THIN_TALL].liquidFuel = 20;
    partDefs[PART_FUEL_THIN_TALL].name = "Thin/Tall Liquid Fuel";
    partDefs[PART_FUEL_THIN_TALL].price = 50;

    partDefs[PART_LIQUID_ROCKET_WIDE].pTexture = OGetTexture("PART_LIQUID_ROCKET_WIDE.png");
    partDefs[PART_LIQUID_ROCKET_WIDE].hsize = partDefs[PART_LIQUID_ROCKET_WIDE].pTexture->getSizef() / 128.0f;
    DEF_ATTACH_POINT(PART_LIQUID_ROCKET_WIDE, 64, 2);
    DEF_ATTACH_POINT(PART_LIQUID_ROCKET_WIDE, 64, 32);
    partDefs[PART_LIQUID_ROCKET_WIDE].weight = 2;
    partDefs[PART_LIQUID_ROCKET_WIDE].name = "Wide Liquid Fuel Rocket";
    partDefs[PART_LIQUID_ROCKET_WIDE].price = 450;
    partDefs[PART_LIQUID_ROCKET_WIDE].trust = 240;
    partDefs[PART_LIQUID_ROCKET_WIDE].isStaged = true;

    partDefs[PART_LIQUID_ROCKET_THIN].pTexture = OGetTexture("PART_LIQUID_ROCKET_THIN.png");
    partDefs[PART_LIQUID_ROCKET_THIN].hsize = partDefs[PART_LIQUID_ROCKET_THIN].pTexture->getSizef() / 128.0f;
    DEF_ATTACH_POINT(PART_LIQUID_ROCKET_THIN, 32, 2);
    DEF_ATTACH_POINT(PART_LIQUID_ROCKET_THIN, 32, 32);
    partDefs[PART_LIQUID_ROCKET_THIN].weight = 2;
    partDefs[PART_LIQUID_ROCKET_THIN].name = "Thin Liquid Fuel Rocket";
    partDefs[PART_LIQUID_ROCKET_THIN].price = 250;
    partDefs[PART_LIQUID_ROCKET_THIN].trust = 80;
    partDefs[PART_LIQUID_ROCKET_THIN].isStaged = true;

    partDefs[PART_SATELLITE].pTexture = OGetTexture("SATELLITE_1.png");
    partDefs[PART_SATELLITE].weight = 2;
}

void detachFromParent(Part* in_pPart)
{
    if (in_pPart->pParent)
    {
        if (in_pPart->parentAttachPoint != -1)
        {
            in_pPart->pParent->usedAttachPoints.erase(in_pPart->parentAttachPoint);
        }
        for (auto it = in_pPart->pParent->children.begin(); it != in_pPart->pParent->children.end(); ++it)
        {
            if (in_pPart == *it)
            {
                in_pPart->pParent->children.erase(it);
                break;
            }
        }
        in_pPart->pParent = nullptr;
    }
}

void deletePart(Part* in_pPart)
{
    if (!in_pPart) return;
    if (pMainPart == in_pPart) pMainPart = nullptr;
    if (in_pPart->pParent)
    {
        detachFromParent(in_pPart);
    }
    for (auto it = parts.begin(); it != parts.end(); ++it)
    {
        auto pPart = *it;
        if (pPart == in_pPart)
        {
            parts.erase(it);
            break;
        }
    }
    for (auto& stage : stages)
    {
        for (auto it = stage.begin(); it != stage.end(); ++it)
        {
            auto pPart = *it;
            if (pPart == in_pPart)
            {
                stage.erase(it);
                break;
            }
        }
    }
    for (auto pChild : in_pPart->children)
    {
        pChild->pParent = nullptr;
        deletePart(pChild);
    }
    delete in_pPart;
}

void deleteParts(Parts& parts)
{
    while (!parts.empty())
    {
        deletePart(parts.front());
    }
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
                if (pParent->type == PART_SOLID_ROCKET ||
                    pParent->type == PART_LIQUID_ROCKET_THIN)
                {
                    onTopSprites.push_back({pEngineCoverTexture, Matrix::CreateScale(1.0f / 64.0f) * Matrix::CreateTranslation(0, -.35f, 0) * transform});
                }
            }
            onTopSprites.push_back({partDef.pTexture, Matrix::CreateScale(1.0f / 64.0f) * transform});
        }
        else if (pPart->type == PART_DECOUPLER_WIDE)
        {
            if (pParent)
            {
                if (pParent->type == PART_LIQUID_ROCKET_WIDE)
                {
                    onTopSprites.push_back({pEngineCoverWideTexture, Matrix::CreateScale(1.0f / 64.0f) * Matrix::CreateTranslation(0, -.35f, 0) * transform});
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

Part* getLiquidFuel(Part* pPart, float& totalLeft, float& maxLiquidFuel)
{
    totalLeft += pPart->liquidFuel;
    maxLiquidFuel += partDefs[pPart->type].liquidFuel;
    if (pPart->pParent)
    {
        if (pPart->pParent->type != PART_DECOUPLER &&
            pPart->pParent->type != PART_DECOUPLER_WIDE &&
            pPart->pParent->type != PART_DECOUPLER_HORIZONTAL_LEFT &&
            pPart->pParent->type != PART_DECOUPLER_HORIZONTAL_RIGHT)
        {
            auto pParentIsTank = getLiquidFuel(pPart->pParent, totalLeft, maxLiquidFuel);
            if (pParentIsTank) return pParentIsTank;
        }
    }
    if (pPart->liquidFuel > 0)
    {
        return pPart;
    }
    return nullptr;
}

Parts toKill;

void explodePart(Part* pPart)
{
    auto altT = getWorldTransform(pPart);
    auto pTopPart = getTopParent(pPart);
    if (pMainPart && pMainPart == pTopPart)
    {
        OPlaySound("Crash.wav");
    }
    toKill.push_back(pPart);
    auto worldPos = Vector2(altT.Translation());
    for (auto i = 0; i < 20; ++i)
    {
        spawnParticles({
            worldPos + ORandVector2(Vector2(-1), Vector2(1)),
            Vector2::Zero,
            0,
            1.0f,
            Color(1, 1, 1, .5f), Color(0, 0, 0, 0),
            0, 6.0f,
            0,
            180.0f,
            pFireTexture
        }, 1, 0.0f, 360.0f, 0, 45.0f, Vector2::UnitY);
        spawnParticles({
            worldPos,
            ORandVector2(Vector2(-10), Vector2(10)),
            0,
            1.0f,
            Color(1, 1, 1, 1), Color(1, 1, 1, 1),
            .5f, .5f,
            0,
            180.0f,
            pDebrisTexture
        }, 1, 0, 360.0f, 2.0f, 45.0f, Vector2::UnitY);
    }
}

int spawn = 0;

void updatePart(Part* pPart)
{
    auto& partDef = partDefs[pPart->type];
    auto pTopParent = getTopParent(pPart);
    if (!pPart->pParent)
    {
        forces.clear();
        pTopParent->totalMass = 0;
        pTopParent->centerOfMass = Vector2::Zero;
        shakeAmount = 0;
        globalStability = 0;
    }
    else
    {
        pTopParent->centerOfMass += pPart->position * partDef.weight;
    }

    globalStability += partDef.stability;
    pTopParent->totalMass += partDef.weight;

    if (pPart->pParent)
    {
        // Copy parent's physic
        pPart->vel = pPart->pParent->vel;
        pPart->angleVelocity = pPart->pParent->angleVelocity;
    }

    extern int gameState;
    if (gameState == GAME_STATE_STAND_BY &&
        pPart->type == PART_SOLID_ROCKET &&
        spawn % 4 == 0)
    {
        auto transform = getWorldTransform(pPart);
        auto worldPos = transform.Translation();
        worldPos.x -= .15f;
        spawnParticles({
            worldPos,
            Vector2(0, .5f),
            0,
            2,
            Color(1, 1, 1, 1), Color(0, 0, 0, 0),
            .25f, .5f,
            ORandFloat(0, 360),
            30.0f,
            pSmokeTexture
        }, 1, 0, 0, 0, 0, Vector2(0, 1));
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
            case PART_LIQUID_ROCKET_THIN:
            case PART_LIQUID_ROCKET_WIDE:
            {
                float amount = 0;
                float maxLiquidFuel = 0;
                auto pTank = getLiquidFuel(pPart, amount, maxLiquidFuel);
                if (pTank && pTank->liquidFuel > 0)
                {
                    shakeAmount += 1;
                    pTank->liquidFuel -= ODT;
                    auto transform = getWorldTransform(pPart);
                    auto worldPos = transform.Translation();
                    auto forward = transform.Up();
                    forward *= -1;
                    forward.Normalize();
                    worldPos -= forward * .75f;
                    forces.push_back({forward * partDef.trust, worldPos});
                    if (pTank->liquidFuel <= 0.0f)
                    {
                        pTank->liquidFuel = 0;
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
    if (!pPart->pParent && gameState != GAME_STATE_STAND_BY)
    {
        auto dirToPlanet = -pPart->position;
        dirToPlanet.Normalize();

        // Apply different forces
        // f = ma
        // a = f / m
        pTopParent->centerOfMass /= pTopParent->totalMass;
        auto transformMe = getWorldTransform(pPart);
        auto worldCenterOfMass = Vector2::Transform(pTopParent->centerOfMass, transformMe);
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
            pPart->vel += Vector2(force.force / pTopParent->totalMass) * ODT;
            pPart->angleVelocity -= (angularEffect / pTopParent->totalMass * 100) * ODT;
        }

        float turbulence = 50.0f / std::max(1.0f, (pPart->position.Length() - PLANET_SIZE));
        turbulence *= pPart->vel.Length();
        turbulence = OLerp(turbulence, 0.0f, std::max(0.0f, std::min(1.0f, (pPart->position.Length() - PLANET_SIZE) / 2000)));
        pPart->angleVelocity += (ORandFloat(-turbulence, turbulence) / pTopParent->totalMass) * ODT;
        pPart->angle += pPart->angleVelocity * ODT;
        pPart->vel += dirToPlanet * GRAVITY * ODT;
        pPart->position += pPart->vel * ODT;
        if (pPart->angleVelocity > 0)
        {
            pPart->angleVelocity -= globalStability / pTopParent->totalMass * 4 * ODT;
            if (pPart->angleVelocity < 0)
            {
                pPart->angleVelocity = 0;
            }
        }
        else if (pPart->angleVelocity < 0)
        {
            pPart->angleVelocity += globalStability / pTopParent->totalMass * 4 * ODT;
            if (pPart->angleVelocity > 0)
            {
                pPart->angleVelocity = 0;
            }
        }
        pPart->speed = pPart->vel.Length();
        pPart->altitude = pPart->position.Length() - PLANET_SIZE;
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
                        Color(1, 1, .5f, .75f), Color(0, 0, 0, 0),
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
            case PART_LIQUID_ROCKET_WIDE:
            {
                float amount = 0, maxLiquidFuel = 0;
                auto pTank = getLiquidFuel(pPart, amount, maxLiquidFuel);
                if (pTank && pTank->liquidFuel > 0)
                {
                    auto transform = getWorldTransform(pPart);
                    auto worldPos = transform.Translation();
                    auto forward = transform.Up();
                    auto right = transform.Right();
                    forward *= -1;
                    forward.Normalize();
                    right.Normalize();
                    worldPos -= forward * .25f;
                    spawnParticles({
                        worldPos + right * .5f,
                        pPart->vel - Vector2(forward * 10.0f),
                        0,
                        .25f,
                        Color(1, 1, 1, 1), Color(0, 0, 0, 0),
                        .5f, 2.0f,
                        2.0f,
                        45.0f,
                        pBlueFireTexture
                    }, 1, 10.0f, 360.0f, 0, 0, -forward);
                    spawnParticles({
                        worldPos - right * .5f,
                        pPart->vel - Vector2(forward * 10.0f),
                        0,
                        .25f,
                        Color(1, 1, 1, 1), Color(0, 0, 0, 0),
                        .5f, 2.0f,
                        2.0f,
                        45.0f,
                        pBlueFireTexture
                    }, 1, 10.0f, 360.0f, 0, 0, -forward);
                    spawnParticles({
                        worldPos,
                        pPart->vel - Vector2(forward * 10.0f),
                        0,
                        .25f,
                        Color(1, 1, 1, 1), Color(0, 0, 0, 0),
                        .5f, 2.0f,
                        2.0f,
                        45.0f,
                        pBlueFireTexture
                    }, 1, 10.0f, 360.0f, 0, 0, -forward);
                }
                else
                {
                    if (pPart->pSound)
                    {
                        pPart->pSound->stop();
                        pPart->pSound = nullptr;
                    }
                }
                break;
            }
            case PART_LIQUID_ROCKET_THIN:
            {
                float amount = 0, maxLiquidFuel = 0;
                auto pTank = getLiquidFuel(pPart, amount, maxLiquidFuel);
                if (pTank && pTank->liquidFuel > 0)
                {
                    auto transform = getWorldTransform(pPart);
                    auto worldPos = transform.Translation();
                    auto forward = transform.Up();
                    auto right = transform.Right();
                    forward *= -1;
                    forward.Normalize();
                    right.Normalize();
                    worldPos -= forward * .25f;
                    spawnParticles({
                        worldPos,
                        pPart->vel - Vector2(forward * 10.0f),
                        0,
                        .25f,
                        Color(1, 1, 1, 1), Color(0, 0, 0, 0),
                        .5f, 2.0f,
                        2.0f,
                        45.0f,
                        pBlueFireTexture
                    }, 1, 10.0f, 360.0f, 0, 0, -forward);
                }
                else
                {
                    if (pPart->pSound)
                    {
                        pPart->pSound->stop();
                        pPart->pSound = nullptr;
                    }
                }
                break;
            }
        }
    }

    auto altT = getWorldTransform(pPart);
    auto altitude = Vector2(altT.Translation()).Length();
    if (altitude < PLANET_SIZE)
    {
        explodePart(pPart);
    }
}
