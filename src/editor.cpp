#include <onut/Font.h>
#include <onut/Input.h>
#include <onut/Renderer.h>
#include <onut/SpriteBatch.h>

#include "editor.h"
#include "defines.h"
#include "meshes.h"
#include "part.h"

float scrollPos = 0;
float scrollTarget = 0;
float scrollViewHeight = 600;

Vector2 editorCamPos;
float ZOOM_LEVELS[] = {16, 32, 64, 96};
int editorZoom = 2;
int holdingPart = -1;
Part* pTargetPart = nullptr;
int targetAttachPoint = -1;
int targetAttachPointSelf = -1;

Vector2 mousePosOnDown;
bool isPanning = false;
Vector2 camPosOnDown;
bool isHoldingValid = false;

#define SCROLL_VIEW_W 150
#define SNAP_DIST 0.25f * 0.25f

extern OFontRef g_pFont;

void resetEditor()
{
    scrollPos = 0;
    scrollTarget = 0;
    scrollViewHeight = 600;

    editorCamPos = Vector2::Zero;
    editorZoom = 2;
    holdingPart = -1;
    pTargetPart = nullptr;
    targetAttachPoint = -1;
    targetAttachPointSelf = -1;

    isPanning = false;
    isHoldingValid = false;

    deleteParts(parts);
    pMainPart = new Part();
    pMainPart->angle = 0;
    pMainPart->type = PART_TOP_CONE;
    parts.push_back(pMainPart);
}

void doPanningZoom()
{
    if (OInputJustPressed(OMouse3))
    {
        isPanning = true;
        mousePosOnDown = oInput->mousePosf;
        camPosOnDown = editorCamPos;
    }
    if (isPanning)
    {
        auto mouseDiff = oInput->mousePosf - mousePosOnDown;
        mouseDiff /= ZOOM_LEVELS[editorZoom];
        editorCamPos = camPosOnDown + mouseDiff;
        if (OInputJustReleased(OMouse3))
        {
            isPanning = false;
        }
    }
    if (oInput->getStateValue(OMouseZ) < 0)
    {
        --editorZoom;
        if (editorZoom < 0) editorZoom = 0;
    }
    if (oInput->getStateValue(OMouseZ) > 0)
    {
        ++editorZoom;
        if (editorZoom > 3) editorZoom = 3;
    }
}

int pickScrollView()
{
    int mouseHoverPartInScrollView = -1;
    float y = 20;
    bool first = true;
    int index = 0;
    for (auto& partDef : partDefs)
    {
        if (first)
        {
            first = false; // Skip the payload
            ++index;
            continue;
        }
        y += 32.0f;
        Vector2 pos(SCROLL_VIEW_W / 2, y + partDef.hsize.y * 64.f);
        Rect rect(pos - Vector2(partDef.pTexture->getSizef() / 2), partDef.pTexture->getSizef());
        if (rect.Contains(oInput->mousePosf))
        {
            mouseHoverPartInScrollView = index;
            break;
        }
        y += partDef.hsize.y * 64.f * 2;
        y += 16.0f;
        y += 16.0f;
        ++index;
    }
    return mouseHoverPartInScrollView;
}

Part* snapToParts(const Vector2& point, const Matrix& parentTransform, const Parts& parts, float& closest, int& attachIndex, const Vector2& reference)
{
    Part* pRet = nullptr;
    for (auto pPart : parts)
    {
        auto& partDef = partDefs[pPart->type];
        Matrix transform = parentTransform * Matrix::CreateRotationZ(pPart->angle) * Matrix::CreateTranslation(pPart->position);
        int index = 0;
        for (auto& attachPoint : partDef.attachPoints)
        {
            if (pPart->usedAttachPoints.find(index) != pPart->usedAttachPoints.end())
            {
                ++index;
                continue;
            }
            if (std::fabsf(reference.x) > std::fabsf(reference.y))
            {
                if ((reference.x < 0 && attachPoint.x < 0) ||
                    (reference.x > 0 && attachPoint.x > 0) ||
                    std::fabsf(attachPoint.y) > std::fabsf(attachPoint.x))
                {
                    ++index;
                    continue;
                }
            }
            else
            {
                if ((reference.y < 0 && attachPoint.y < 0) ||
                    (reference.y > 0 && attachPoint.y > 0) ||
                    std::fabsf(attachPoint.x) > std::fabsf(attachPoint.y))
                {
                    ++index;
                    continue;
                }
            }
            auto attachPointTransformed = Vector2::Transform(attachPoint, transform);
            auto dist = Vector2::DistanceSquared(attachPointTransformed, point);
            if (dist < closest && dist <= SNAP_DIST)
            {
                pRet = pPart;
                closest = dist;
                attachIndex = index;
            }
            ++index;
        }
        auto pNePart = snapToParts(point, transform, pPart->children, closest, attachIndex, reference);
        if (pNePart) pRet = pNePart;
    }
    return pRet;
}

void doSnappingLogic()
{
    pTargetPart = nullptr;
    float closest = std::numeric_limits<float>::max();
    auto transform = Matrix::CreateTranslation(editorCamPos) * Matrix::CreateScale(ZOOM_LEVELS[editorZoom]) * Matrix::CreateTranslation((OScreenWf - SCROLL_VIEW_W) / 2 + SCROLL_VIEW_W, OScreenHf / 2, 0);
    auto invTransform = transform.Invert();
    Vector2 worldMouse = Vector2::Transform(oInput->mousePosf, invTransform);
    auto& partDef = partDefs[holdingPart];
    int index = 0;
    for (auto& attachPoint : partDef.attachPoints)
    {
        auto point = worldMouse + attachPoint;
        int otherAttachPoint;
        auto pOtherPart = snapToParts(point, Matrix::Identity, parts, closest, otherAttachPoint, attachPoint);
        if (pOtherPart)
        {
            pTargetPart = pOtherPart;
            targetAttachPoint = otherAttachPoint;
            targetAttachPointSelf = index;
        }
        ++index;
    }
    isHoldingValid = pTargetPart != nullptr;
}

void updateEditor(float dt)
{
    doPanningZoom();
    if (holdingPart != -1)
    {
        if (OInputJustPressed(OMouse2))
        {
            holdingPart = -1;
        }
        else
        {
            doSnappingLogic();
            if (isHoldingValid && OInputJustPressed(OMouse1))
            {
                // Place the part bitch! YEAAAAAAAAAAAAAAAAAAAAA EDITORRRSZZ
                auto& partDef = partDefs[holdingPart];
                auto& targetPartDef = partDefs[pTargetPart->type];
                auto pPart = new Part();
                pPart->type = holdingPart;
                pPart->position = targetPartDef.attachPoints[targetAttachPoint] - partDef.attachPoints[targetAttachPointSelf];
                pPart->usedAttachPoints.insert(targetAttachPointSelf);
                pTargetPart->usedAttachPoints.insert(targetAttachPoint);
                pTargetPart->children.push_back(pPart);
                holdingPart = -1;
            }
        }
    }
    else
    {
        int mouseHoverPartInScrollView = pickScrollView();
        if (mouseHoverPartInScrollView != -1 &&
            OInputJustPressed(OMouse1))
        {
            holdingPart = mouseHoverPartInScrollView;
        }
    }
}

void drawEditor()
{
    oRenderer->setupFor2D();
    oRenderer->renderStates.primitiveMode = OPrimitivePointList;
    drawMesh(Matrix::Identity, starMesh);
    oRenderer->renderStates.primitiveMode = OPrimitiveTriangleList;

    // Draw rocket view
    auto partTransform = Matrix::CreateTranslation(editorCamPos) * Matrix::CreateScale(ZOOM_LEVELS[editorZoom]) * Matrix::CreateTranslation((OScreenWf - SCROLL_VIEW_W) / 2 + SCROLL_VIEW_W, OScreenHf / 2, 0);
    oSpriteBatch->begin(partTransform);
    drawParts(Matrix::Identity, parts);
    drawOnTops();
    oSpriteBatch->end();
    if (holdingPart != -1)
    {
        drawAnchors(partTransform, parts);
    }

    // Draw scrollview
    oSpriteBatch->begin(Matrix::CreateTranslation(0, -scrollPos, 0));
    oSpriteBatch->changeBlendMode(OBlendAlpha);
    oSpriteBatch->drawRect(nullptr, Rect(0, 0, SCROLL_VIEW_W, OScreenHf), Color::Black);
    float y = 20;
    bool first = true;
    for (auto& partDef : partDefs)
    {
        if (first)
        {
            first = false; // Skip the payload
            continue;
        }
        g_pFont->draw(partDef.name + "\nPrice: " + std::to_string(partDef.price) + " $",
                      Vector2(SCROLL_VIEW_W / 2, y), OTop);
        y += 32.0f;
        Vector2 pos(SCROLL_VIEW_W / 2, y + partDef.hsize.y * 64.f);
        oSpriteBatch->drawSprite(partDef.pTexture, pos);
        y += partDef.hsize.y * 64.f * 2;
        y += 16.0f;
        oSpriteBatch->drawRect(nullptr, Rect(0, y, SCROLL_VIEW_W, 1), Color(1, 1, 1, .3f));
        y += 16.0f;
    }
    oSpriteBatch->drawRect(nullptr, Rect(SCROLL_VIEW_W, 0, 2, OScreenHf));
    oSpriteBatch->end();

    // Draw holding item
    if (holdingPart != -1)
    {
        auto transform = Matrix::CreateTranslation(editorCamPos) * Matrix::CreateScale(ZOOM_LEVELS[editorZoom]) * Matrix::CreateTranslation((OScreenWf - SCROLL_VIEW_W) / 2 + SCROLL_VIEW_W, OScreenHf / 2, 0);
        auto invTransform = transform.Invert();
        Vector2 worldMouse = Vector2::Transform(oInput->mousePosf, invTransform);
        auto& partDef = partDefs[holdingPart];
        oSpriteBatch->begin(transform);
        oSpriteBatch->drawRect(partDef.pTexture,
                               Rect(worldMouse - partDef.hsize, partDef.hsize * 2.0f),
                               isHoldingValid ? Color(0, 1, 0, 1) : Color(1, 0, 0, 1));
        oSpriteBatch->end();
    }
}
