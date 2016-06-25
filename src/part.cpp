#include <onut/SpriteBatch.h>
#include <onut/Renderer.h>
#include <onut/Timing.h>

#include "part.h"

PartDef partDefs[PART_COUNT];
Parts parts;
Part* pMainPart = nullptr;
std::vector<std::vector<Part*>> stages;

#define GRAVITY 3.0f

OTextureRef pEngineCoverTexture;

#define DEF_ATTACH_POINT(__part__, __x__, __y__) partDefs[__part__].attachPoints.push_back((Vector2(__x__, __y__) - Vector2(partDefs[__part__].pTexture->getSizef() / 2)) / 64)

void initPartDefs()
{
    pEngineCoverTexture = OGetTexture("engineCover.png");

    partDefs[PART_TOP_CONE].pTexture = OGetTexture("PART_TOP_CONE.png");
    partDefs[PART_TOP_CONE].hsize = partDefs[PART_TOP_CONE].pTexture->getSizef() / 128.0f;
    DEF_ATTACH_POINT(PART_TOP_CONE, 32, 75);
    partDefs[PART_TOP_CONE].weight = 2;
    partDefs[PART_TOP_CONE].name = "Payload";
    partDefs[PART_TOP_CONE].price = 0;
    partDefs[PART_TOP_CONE].isStaged = true;

    partDefs[PART_SOLID_ROCKET].pTexture = OGetTexture("PART_SOLID_ROCKET.png");
    partDefs[PART_SOLID_ROCKET].hsize = partDefs[PART_SOLID_ROCKET].pTexture->getSizef() / 128.0f;
    DEF_ATTACH_POINT(PART_SOLID_ROCKET, 32, 4);
    DEF_ATTACH_POINT(PART_SOLID_ROCKET, 32, 122);
    DEF_ATTACH_POINT(PART_SOLID_ROCKET, 2, 54);
    DEF_ATTACH_POINT(PART_SOLID_ROCKET, 62, 54);
    partDefs[PART_SOLID_ROCKET].weight = 5;
    partDefs[PART_SOLID_ROCKET].name = "Solid Fuel Rocket";
    partDefs[PART_SOLID_ROCKET].price = 200;
    partDefs[PART_SOLID_ROCKET].isStaged = true;

    partDefs[PART_DECOUPLER].pTexture = OGetTexture("PART_DECOUPLER.png");
    partDefs[PART_DECOUPLER].hsize = partDefs[PART_DECOUPLER].pTexture->getSizef() / 128.0f;
    DEF_ATTACH_POINT(PART_DECOUPLER, 31, 4);
    DEF_ATTACH_POINT(PART_DECOUPLER, 31, 8);
    partDefs[PART_DECOUPLER].weight = .25f;
    partDefs[PART_DECOUPLER].name = "Decoupler";
    partDefs[PART_DECOUPLER].price = 75;
    partDefs[PART_DECOUPLER].isStaged = true;

    partDefs[PART_CONE].pTexture = OGetTexture("PART_CONE.png");
    partDefs[PART_CONE].hsize = partDefs[PART_CONE].pTexture->getSizef() / 128.0f;
    DEF_ATTACH_POINT(PART_CONE, 32, 40);
    partDefs[PART_CONE].weight = .5f;
    partDefs[PART_CONE].name = "Aerodynamic Cone";
    partDefs[PART_CONE].price = 50;
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

void drawParts(const Matrix& parentTransform, Parts& parts, Part* pParent)
{
    for (auto pPart : parts)
    {
        auto& partDef = partDefs[pPart->type];
        Matrix transform = parentTransform * Matrix::CreateRotationZ(pPart->angle) * Matrix::CreateTranslation(pPart->position);
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
                    onTopSprites.push_back({pEngineCoverTexture, Matrix::CreateScale(1.0f / 64.0f) * Matrix::CreateTranslation(0, -.25f, 0) * transform});
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
        Matrix transform = parentTransform * Matrix::CreateRotationZ(pPart->angle) * Matrix::CreateTranslation(pPart->position);
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

void updatePart(Part* pPart)
{
    if (pPart->isActive)
    {
        auto& partDef = partDefs[pPart->type];
        switch (pPart->type)
        {
            case PART_SOLID_ROCKET:
            {
                break;
            }
        }
    }

    if (pPart->pParent)
    {
        // Copy parent's physic
        pPart->vel = pPart->pParent->vel;
        pPart->angleVelocity = pPart->pParent->angleVelocity;
    }
    else
    {
        auto dirToPlanet = pPart->position;
        dirToPlanet.Normalize();
        pPart->vel += dirToPlanet * GRAVITY;
        pPart->position += pPart->vel * ODT;
    }

    for (auto pChild : pPart->children)
    {
        updatePart(pChild);
    }
}
