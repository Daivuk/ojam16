#include <onut/SpriteBatch.h>
#include <onut/Renderer.h>

#include "part.h"

PartDef partDefs[PART_COUNT];
Parts parts;
Part* pMainPart = nullptr;

OTextureRef pEngineCoverTexture;

#define DEF_ATTACH_POINT(__part__, __x__, __y__) partDefs[__part__].attachPoints.push_back((Vector2(__x__, __y__) - Vector2(partDefs[__part__].pTexture->getSizef() / 2)) / 64)

void initPartDefs()
{
    pEngineCoverTexture = OGetTexture("engineCover.png");

    partDefs[PART_TOP_CONE].pTexture = OGetTexture("PART_TOP_CONE.png");
    partDefs[PART_TOP_CONE].hsize = partDefs[PART_TOP_CONE].pTexture->getSizef() / 128.0f;
    DEF_ATTACH_POINT(PART_TOP_CONE, 32, 75);
    partDefs[PART_TOP_CONE].weight = 1;
    partDefs[PART_TOP_CONE].name = "Payload";
    partDefs[PART_TOP_CONE].price = 300;

    partDefs[PART_SOLID_ROCKET].pTexture = OGetTexture("PART_SOLID_ROCKET.png");
    partDefs[PART_SOLID_ROCKET].hsize = partDefs[PART_SOLID_ROCKET].pTexture->getSizef() / 128.0f;
    DEF_ATTACH_POINT(PART_SOLID_ROCKET, 32, 4);
    DEF_ATTACH_POINT(PART_SOLID_ROCKET, 32, 122);
    DEF_ATTACH_POINT(PART_SOLID_ROCKET, 2, 54);
    DEF_ATTACH_POINT(PART_SOLID_ROCKET, 62, 54);
    partDefs[PART_SOLID_ROCKET].weight = 1;
    partDefs[PART_SOLID_ROCKET].name = "Solid Fuel Rocket";
    partDefs[PART_SOLID_ROCKET].price = 150;

    partDefs[PART_DECOUPLER].pTexture = OGetTexture("PART_DECOUPLER.png");
    partDefs[PART_DECOUPLER].hsize = partDefs[PART_DECOUPLER].pTexture->getSizef() / 128.0f;
    DEF_ATTACH_POINT(PART_DECOUPLER, 31, 4);
    DEF_ATTACH_POINT(PART_DECOUPLER, 31, 8);
    partDefs[PART_DECOUPLER].weight = .25f;
    partDefs[PART_DECOUPLER].name = "Decoupler";
    partDefs[PART_DECOUPLER].price = 50;
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

using OnTopSprites = std::vector<OnTopSprite>;
OnTopSprites onTopSprites;

void drawOnTops()
{
    for (auto& onTopSprite : onTopSprites)
    {
        oSpriteBatch->drawSprite(onTopSprite.pTexture,
                                 onTopSprite.transform,
                                 Color::White);
    }
    onTopSprites.clear();
}

void drawParts(const Matrix& parentTransform, Parts& parts, Part* pParent)
{
    for (auto pPart : parts)
    {
        auto& partDef = partDefs[pPart->type];
        Matrix transform = parentTransform * Matrix::CreateRotationZ(pPart->angle) * Matrix::CreateTranslation(pPart->position);
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
