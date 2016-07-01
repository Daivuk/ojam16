#include <onut/Curve.h>
#include <onut/CSV.h>
#include <onut/SpriteBatch.h>
#include <onut/Renderer.h>
#include <onut/Timing.h>
#include <onut/Random.h>
#include <onut/Sound.h>

#include "part.h"
#include "particle.h"
#include "defines.h"

std::vector<PartDef> partDefs;
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

std::unordered_map<std::string, int> PART_TYPES_MAP = {
    {"PAYLOAD", PART_TYPE_PAYLOAD},
    {"BOOSTER", PART_TYPE_BOOSTER},
    {"DECOUPLER", PART_TYPE_DECOUPLER},
    {"AERODYNAMIC", PART_TYPE_AERODYNAMIC},
    {"FUEL", PART_TYPE_FUEL},
    {"SATELLITE", PART_TYPE_SATELLITE},
    {"ENGINE", PART_TYPE_ENGINE},
};

std::unordered_map<std::string, int> PART_ATTACH_DIR_MAP = {
    {"up", PART_ATTACH_DIR_UP},
    {"down", PART_ATTACH_DIR_DOWN},
    {"left", PART_ATTACH_DIR_LEFT},
    {"right", PART_ATTACH_DIR_RIGHT},
};

std::unordered_map<int, int> partDefsMap;

void initPartDefs()
{
    pEngineCoverTexture = OGetTexture("PART_ENGINE_COVER.png");
    pEngineCoverWideTexture = OGetTexture("PART_ENGINE_COVER_WIDE.png");
    pFireTexture = OGetTexture("PARTICLE_FIRE.png");
    pSmokeTexture = OGetTexture("PARTICLE_SMOKE.png");
    pBlueFireTexture = OGetTexture("PARTICLE_BLUE_FLAME.png");
    pDebrisTexture = OGetTexture("PARTICLE_DEBRIS.png");

    auto pPartsCSV = OGetCSV("ojam16 - parts.csv");
    auto pAttachPointsCSV = OGetCSV("ojam16 - attachPoints.csv");
    try
    {
        auto partCount = pPartsCSV->getRowCount();
        for (int i = 0; i < partCount; ++i)
        {
            PartDef partDef;
            partDef.pTexture = OGetTexture(pPartsCSV->getValue("image", i));
            auto& engineCover = pPartsCSV->getValue("engineCover", i);
            if (!engineCover.empty())
            {
                partDef.pEngineCoverTexture = OGetTexture(pPartsCSV->getValue("engineCover", i));
            }
            partDef.hsize = partDef.pTexture->getSizef() / 128.0f;
            partDef.type = PART_TYPES_MAP[pPartsCSV->getValue("type", i)];
            partDef.weight = pPartsCSV->getFloat("mass", i);
            partDef.name = pPartsCSV->getValue("name", i);
            partDef.price = pPartsCSV->getInt("price", i);
            partDef.isStaged = pPartsCSV->getValue("staged", i) == "TRUE";
            partDef.trust = pPartsCSV->getFloat("trust", i);
            partDef.burn = pPartsCSV->getFloat("burn", i);
            if (partDef.type == PART_TYPE_BOOSTER)
            {
                partDef.solidFuel = pPartsCSV->getFloat("fuel", i);
            }
            else
            {
                partDef.liquidFuel = pPartsCSV->getFloat("fuel", i);
            }
            partDef.stability = pPartsCSV->getFloat("stability", i);
            partDef.id = pPartsCSV->getInt("id", i);
            partDefsMap[partDef.id] = i;
            partDefs.push_back(partDef);
        }
        auto attachPointCount = pAttachPointsCSV->getRowCount();
        for (int i = 0; i < attachPointCount; ++i)
        {
            auto partId = pAttachPointsCSV->getInt("partId", i);
            auto& partDef = partDefs[partDefsMap[partId]];
            auto x = pAttachPointsCSV->getFloat("x", i);
            auto y = pAttachPointsCSV->getFloat("y", i);
            partDef.attachPoints.push_back((Vector2(x, y) - Vector2(partDef.pTexture->getSizef() / 2)) / 64);
            auto& dir = pAttachPointsCSV->getValue("direction", i);
            partDef.attachPointsDir.push_back(PART_ATTACH_DIR_MAP[dir]);
        }
    }
    catch (...)
    {
        assert(false);
    }
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
        if (partDef.type == PART_TYPE_DECOUPLER)
        {
            if (pParent)
            {
                auto& parentPartDef = partDefs[pParent->type];
                if (parentPartDef.type == PART_TYPE_BOOSTER ||
                    parentPartDef.type == PART_TYPE_ENGINE)
                {
                    onTopSprites.push_back({partDef.pEngineCoverTexture, Matrix::CreateScale(1.0f / 64.0f) * Matrix::CreateTranslation(0, -.35f, 0) * transform});
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
        auto& parentPartDef = partDefs[pPart->pParent->type];
        if (parentPartDef.type != PART_TYPE_DECOUPLER)
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
        partDef.type == PART_TYPE_BOOSTER &&
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
        switch (partDef.type)
        {
            case PART_TYPE_BOOSTER:
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
            case PART_TYPE_ENGINE:
            {
                float amount = 0;
                float maxLiquidFuel = 0;
                auto pTank = getLiquidFuel(pPart, amount, maxLiquidFuel);
                if (pTank && pTank->liquidFuel > 0)
                {
                    shakeAmount += 1;
                    pTank->liquidFuel -= partDef.burn * ODT;
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
        //pPart->angleVelocity += (ORandFloat(-turbulence, turbulence) / pTopParent->totalMass) * ODT;
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
        switch (partDef.type)
        {
            case PART_TYPE_BOOSTER:
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
            case PART_TYPE_ENGINE:
            {
                float amount = 0, maxLiquidFuel = 0;
                auto pTank = getLiquidFuel(pPart, amount, maxLiquidFuel);
                if (partDef.burn > .5f && partDef.burn <= 1.5f)
                {
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
                }
                else if (partDef.burn > 2.5f && partDef.burn <= 3.5f)
                {
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
