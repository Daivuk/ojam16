#include <onut/Font.h>
#include <onut/Input.h>
#include <onut/Renderer.h>
#include <onut/SpriteBatch.h>
#include <onut/Sound.h>

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
Part* pHoverPart = nullptr;

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
    pMainPart->type = 0;
    parts.push_back(pMainPart);

    stages.clear();
    stages.push_back({pMainPart});
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
        if (oInput->mousePosf.x < SCROLL_VIEW_W)
        {
            scrollPos -= oInput->getStateValue(OMouseZ);
        }
        else
        {
            --editorZoom;
            if (editorZoom < 0) editorZoom = 0;
        }
    }
    if (oInput->getStateValue(OMouseZ) > 0)
    {
        if (oInput->mousePosf.x < SCROLL_VIEW_W)
        {
            scrollPos -= oInput->getStateValue(OMouseZ);
        }
        else
        {
            ++editorZoom;
            if (editorZoom > 3) editorZoom = 3;
        }
    }
    if (scrollPos < 0) scrollPos = 0;
}

int pickScrollView()
{
    int mouseHoverPartInScrollView = -1;
    float y = 20;
    for (auto i = 0; i < (int)partDefs.size(); ++i)
    {
        auto& partDef = partDefs[i];
        if (partDef.type == PART_TYPE_PAYLOAD)
        {
            continue;
        }
        y += 32.0f;
        Vector2 pos(SCROLL_VIEW_W / 2, y + partDef.hsize.y * 64.f - scrollPos);
        Rect rect(pos - Vector2(partDef.pTexture->getSizef() / 2), partDef.pTexture->getSizef());
        if (rect.Contains(oInput->mousePosf))
        {
            mouseHoverPartInScrollView = i;
            break;
        }
        y += partDef.hsize.y * 64.f * 2;
        y += 16.0f;
        y += 16.0f;
    }
    return mouseHoverPartInScrollView;
}

Part* snapToParts(const Vector2& point, const Matrix& parentTransform, const Parts& parts, float& closest, int& attachIndex, const Vector2& reference, int dir)
{
    Part* pRet = nullptr;
    for (auto pPart : parts)
    {
        auto& partDef = partDefs[pPart->type];
        Matrix transform = parentTransform * Matrix::CreateRotationZ(pPart->angle) * Matrix::CreateTranslation(pPart->position);
        int index = 0;
        for (auto& attachPoint : partDef.attachPoints)
        {
            bool leftRight = std::fabsf(attachPoint.x) > std::fabsf(attachPoint.y);
            if ((partDef.attachPointsDir[index] == PART_ATTACH_DIR_UP &&
                dir != PART_ATTACH_DIR_DOWN) ||
                (partDef.attachPointsDir[index] == PART_ATTACH_DIR_DOWN &&
                dir != PART_ATTACH_DIR_UP) ||
                (partDef.attachPointsDir[index] == PART_ATTACH_DIR_LEFT &&
                dir != PART_ATTACH_DIR_RIGHT) ||
                (partDef.attachPointsDir[index] == PART_ATTACH_DIR_RIGHT &&
                dir != PART_ATTACH_DIR_LEFT) ||
                pPart->usedAttachPoints.find(index) != pPart->usedAttachPoints.end())
            {
                ++index;
                continue;
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
        auto pNePart = snapToParts(point, transform, pPart->children, closest, attachIndex, reference, dir);
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
        auto pOtherPart = snapToParts(point, Matrix::Identity, parts, closest, otherAttachPoint, attachPoint, partDef.attachPointsDir[index]);
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

void trimStages()
{
    for (auto it = stages.begin(); it != stages.end();)
    {
        auto& stage = *it;
        if (stage.empty())
        {
            it = stages.erase(it);
            continue;
        }
        ++it;
    }
}

void updateEditor(float dt)
{
    pHoverPart = nullptr;
    doPanningZoom();
    if (holdingPart != -1)
    {
        if (OInputJustPressed(OMouse2))
        {
            holdingPart = -1;
            OPlaySound("Build_CancelPart.wav");
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
                pPart->pParent = pTargetPart;
                pPart->solidFuel = partDef.solidFuel;
                pPart->liquidFuel = partDef.liquidFuel;
                pPart->parentAttachPoint = targetAttachPoint;
                pTargetPart->usedAttachPoints.insert(targetAttachPoint);
                pTargetPart->children.push_back(pPart);
                holdingPart = -1;

                if (partDef.isStaged)
                {
                    // Add to a new stage
                    stages.push_back({pPart});
                }

                OPlayRandomSound({"Build_AddPart01.wav", "Build_AddPart02.wav", "Build_AddPart03.wav", "Build_AddPart04.wav"});
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
            OPlayRandomSound({"Build_PickupPart01.wav", "Build_PickupPart02.wav", "Build_PickupPart03.wav", "Build_PickupPart04.wav"});
        }
        if (holdingPart == -1)
        {
            auto transform = Matrix::CreateTranslation(editorCamPos) * Matrix::CreateScale(ZOOM_LEVELS[editorZoom]) * Matrix::CreateTranslation((OScreenWf - SCROLL_VIEW_W) / 2 + SCROLL_VIEW_W, OScreenHf / 2, 0);
            auto invTransform = transform.Invert();
            Vector2 worldMouse = Vector2::Transform(oInput->mousePosf, invTransform);

            pHoverPart = mouseHoverPart(parts[0], worldMouse);
        }
    }
    if (pHoverPart)
    {
        if (OInputJustPressed(OKeyDown))
        {
            int stageIndex = 0;
            for (auto& stage : stages)
            {
                bool found = false;
                for (auto it = stage.begin(); it != stage.end(); ++it)
                {
                    auto pPart = *it;
                    if (pPart == pHoverPart)
                    {
                        stage.erase(it);
                        found = true;
                        break;
                    }
                }
                if (found)
                {
                    break;
                }
                ++stageIndex;
            }
            ++stageIndex;
            OPlaySound("Build_ChangeStage.wav");
            if ((int)stages.size() <= stageIndex)
            {
                stages.push_back({pHoverPart});
            }
            else
            {
                stages[stageIndex].push_back(pHoverPart);
            }
            trimStages();
        }
        else if (OInputJustPressed(OKeyUp))
        {
            int stageIndex = 0;
            for (auto& stage : stages)
            {
                bool found = false;
                for (auto it = stage.begin(); it != stage.end(); ++it)
                {
                    auto pPart = *it;
                    if (pPart == pHoverPart)
                    {
                        stage.erase(it);
                        found = true;
                        break;
                    }
                }
                if (found)
                {
                    break;
                }
                ++stageIndex;
            }
            --stageIndex;
            OPlaySound("Build_ChangeStage.wav");
            if (stageIndex < 0)
            {
                stages.insert(stages.begin(), {pHoverPart});
            }
            else
            {
                stages[stageIndex].push_back(pHoverPart);
            }
            trimStages();
        }
        else if (OInputJustPressed(OKeyDelete))
        {
            if (pHoverPart != pMainPart)
            {
                deletePart(pHoverPart);
                pHoverPart = nullptr;
                OPlaySound("Build_RemovePart.wav");
            }
        }
    }
}

void drawEditor()
{
    oRenderer->setupFor2D();
    oRenderer->renderStates.primitiveMode = OPrimitivePointList;
    drawMesh(Matrix::CreateScale(OScreenWf / 800.0f), starMesh);
    oRenderer->renderStates.primitiveMode = OPrimitiveTriangleList;

    // Draw rocket view
    auto partTransform = Matrix::CreateTranslation(editorCamPos) * Matrix::CreateScale(ZOOM_LEVELS[editorZoom]) * Matrix::CreateTranslation((OScreenWf - SCROLL_VIEW_W) / 2 + SCROLL_VIEW_W, OScreenHf / 2, 0);
    oSpriteBatch->begin(partTransform);
    drawParts(Matrix::Identity, parts);
    drawOnTops();
    oSpriteBatch->end();
    //if (holdingPart != -1)
    //{
    //    drawAnchors(partTransform, parts);
    //}

    // Draw scrollview
    oSpriteBatch->begin();
    oSpriteBatch->drawRect(nullptr, Rect(0, 0, SCROLL_VIEW_W, OScreenHf * 1000), Color::Black);
    oSpriteBatch->drawRect(nullptr, Rect(SCROLL_VIEW_W, 0, 2, OScreenHf));
    oSpriteBatch->end();
    oSpriteBatch->begin(Matrix::CreateTranslation(0, -scrollPos, 0));
    oSpriteBatch->changeBlendMode(OBlendAlpha);
    float y = 20;
    for (auto i = 0; i < (int)partDefs.size(); ++i)
    {
        auto& partDef = partDefs[i];
        if (partDef.type == PART_TYPE_PAYLOAD)
        {
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

    // Draw stages on the right
    oSpriteBatch->begin();
    Vector2 stageTextPos(OScreenWf - 20.0f, 20.0f);
    int stageId = (int)stages.size();
    for (auto& stage : stages)
    {
        g_pFont->draw("--- Stage " + std::to_string(stageId) + " ---", stageTextPos, OTopRight, Color(1, 1, 1));
        stageTextPos.y += 16;
        for (auto pPart : stage)
        {
            auto& partDef = partDefs[pPart->type];
            if (pPart == pHoverPart)
            {
                g_pFont->draw("--> " + partDef.name, stageTextPos, OTopRight, Color(1, 0, 1));
            }
            else
            {
                g_pFont->draw(partDef.name, stageTextPos, OTopRight, Color(0, 1, 1));
            }
            stageTextPos.y += 16;
        }
        stageTextPos.y += 16;
        --stageId;
    }
    oSpriteBatch->end();

    // Help tooltips
    g_pFont->draw("PRESS ^990ESC^999 TO CLEAR", {OScreenWf / 2, OScreenHf - 24}, OBottom);
    g_pFont->draw("PRESS ^990SPACE BAR^999 TO LAUNCH", {OScreenWf / 2, OScreenHf - 8}, OBottom);
    if (pHoverPart)
    {
        if (pHoverPart != pMainPart) g_pFont->draw("PRESS ^909DELETE^999 REMOVE PART", {OScreenWf / 2, OScreenHf - 24 - 32}, OBottom);
        g_pFont->draw("PRESS ^909UP/DOWN^999 TO MOVE STAGE", {OScreenWf / 2, OScreenHf - 8 - 32}, OBottom);
    }
}
