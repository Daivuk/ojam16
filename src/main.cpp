#include <Windows.h>
#include <onut/onut.h>
#include <onut/Settings.h>
#include <onut/Font.h>
#include <onut/Timing.h>
#include <onut/Renderer.h>
#include <onut/SpriteBatch.h>
#include <onut/Random.h>
#include <onut/PrimitiveBatch.h>
#include <onut/VertexBuffer.h>
#include <onut/IndexBuffer.h>
#include <onut/Texture.h>
#include <onut/Anim.h>
#include <onut/Input.h>

#include <vector>

#include "meshes.h"
#include "part.h"
#include "editor.h"

void init();
void update();
void render();
void postRender();

OFontRef g_pFont;
OTextureRef pWhiteTexture;
OTextureRef pMiniMap;

float zoom = 64;
int gameState = GAME_STATE_EDITOR;
Vector2 cameraPos;

#define MINIMAP_SIZE 192

int WINAPI WinMain(_In_ HINSTANCE hInstance,
                   _In_opt_ HINSTANCE hPrevInstance,
                   _In_ LPSTR lpCmdLine,
                   _In_ int nShowCmd)
{
    oSettings->setIsFixedStep(true);
    oSettings->setGameName("Ottawa Game Jam 2016");
    oSettings->setResolution({800, 600});

    ORun(init, update, render, postRender);

    return 0;
}

void init()
{
    oTiming->setUpdateFps(60);
    g_pFont = OGetFont("font.fnt");
    uint32_t white = 0xFFFFFFFF;
    pWhiteTexture = OTexture::createFromData((uint8_t*)&white, {1, 1}, false);
    pMiniMap = OTexture::createRenderTarget({MINIMAP_SIZE, MINIMAP_SIZE}, false);
    createMeshes();
    
    initPartDefs();
    resetEditor();
}

void updateCamera()
{
    auto targetCamera = vehiculeRect(parts[0]).Center();
    cameraPos += (targetCamera - cameraPos) * ODT * 5.0f;
}

void decouple(Part* pPart)
{
    for (auto pChild : pPart->children)
    {
        parts.push_back(pChild);
        pChild->angleVelocity += ORandFloat(-1, 1);
        auto currentDir = pChild->vel;
        currentDir.Normalize();
        pChild->vel -= currentDir * 2;
        pChild->pParent = nullptr;
    }
    if (pPart->pParent)
    {
        auto currentDir = pPart->pParent->vel;
        pPart->pParent->vel += currentDir * 2;
        for (auto it = pPart->pParent->children.begin(); it != pPart->pParent->children.end(); ++it)
        {
            if (*it == pPart)
            {
                pPart->pParent->children.erase(it);
                break;
            }
        }
    }
    pPart->pParent = nullptr;
    pPart->angleVelocity += ORandFloat(-1, 1);
    pPart->children.clear();
    parts.push_back(pPart);
}

void activateNextStage()
{
    auto currentState = stages.back();
    stages.erase(stages.end() - 1);
    if (!stages.empty())
    {
        auto& newStage = stages.back();
        for (auto pPart : newStage)
        {
            pPart->isActive = true;
            if (pPart->type == PART_DECOUPLER)
            {
                decouple(pPart);
            }
        }
    }
}

void update()
{
    switch (gameState)
    {
        case GAME_STATE_EDITOR:
        {
            if (OInputJustPressed(OKeyEscape))
            {
                resetEditor();
            }
            else if (OInputJustPressed(OKeySpaceBar))
            {
                gameState = GAME_STATE_STAND_BY;
                auto vrect = vehiculeRect(parts[0]);
                parts[0]->position = {0, -PLANET_SIZE - vrect.w};
                vrect = vehiculeRect(parts[0]);
                cameraPos = vrect.Center();
                stages.push_back({}); // Add empty stage at the end so we can start with nothing happening
                zoom = 256.0f / (vrect.w / 2);
                zoom = std::min(64.0f, zoom);
            }
            else
            {
                updateEditor(ODT);
            }
            break;
        }
        case GAME_STATE_STAND_BY:
        {
            updateCamera();
            if (OInputJustPressed(OKeySpaceBar))
            {
                activateNextStage();
            }
            break;
        }
        case GAME_STATE_FLIGHT:
        {
            updateCamera();
            if (OInputJustPressed(OKeySpaceBar))
            {
                activateNextStage();
            }
            for (auto pPart : parts) updatePart(pPart);
            break;
        }
    }
}

void drawWorld()
{
    oRenderer->setupFor2D();
    oRenderer->renderStates.primitiveMode = OPrimitivePointList;
    drawMesh(Matrix::Identity, starMesh);
    oRenderer->renderStates.primitiveMode = OPrimitiveTriangleList;
    oRenderer->set2DCameraOffCenter(cameraPos, zoom);
    drawMeshIndexed(Matrix::Identity, atmosphereMesh);
    drawMeshIndexed(Matrix::Identity, planetMesh);
}

void drawParts()
{
    oSpriteBatch->begin();
    oRenderer->set2DCameraOffCenter(cameraPos, zoom);
    drawParts(Matrix::Identity, parts);
    drawOnTops();
    oSpriteBatch->drawOutterOutlineRect(vehiculeRect(parts[0]), .1f, Color(1, 1, 0));
    oSpriteBatch->end();
}

void drawMiniMap()
{
    //--- Update minimap content
    oRenderer->renderStates.renderTarget = pMiniMap;
    oRenderer->clear(Color::Black);
    oRenderer->renderStates.primitiveMode = OPrimitiveTriangleList;
    float zoomf = ((float)MINIMAP_SIZE / (float)PLANET_SIZE) / 8;
    oRenderer->set2DCameraOffCenter(Vector2::Zero, zoomf);
    oRenderer->renderStates.viewport.push({0, 0, MINIMAP_SIZE, MINIMAP_SIZE});
    drawMeshIndexed(Matrix::Identity, atmosphereMesh);
    drawMeshIndexed(Matrix::Identity, planetMesh);
    oRenderer->renderStates.renderTarget = nullptr;
    oRenderer->renderStates.viewport.pop();

    //--- Draw it in the top right corner
    oSpriteBatch->begin();
    oSpriteBatch->drawRect(pMiniMap, {OScreenWf - MINIMAP_SIZE, 0, MINIMAP_SIZE, MINIMAP_SIZE});
    oSpriteBatch->end();
}

void render()
{
    oRenderer->clear({0, 0, 0, 1});

    switch (gameState)
    {
        case GAME_STATE_EDITOR:
        {
            drawEditor();
            g_pFont->draw("PRESS ^990ESC^999 TO CLEAR", {OScreenWf / 2, OScreenHf-24}, OBottom);
            g_pFont->draw("PRESS ^990SPACE BAR^999 TO LAUNCH", {OScreenWf / 2, OScreenHf-8}, OBottom);
            break;
        }
        case GAME_STATE_STAND_BY:
        {
            drawWorld();
            drawParts();
            drawMiniMap();
            break;
        }
        case GAME_STATE_FLIGHT:
        {
            drawWorld();
            drawParts();
            drawMiniMap();
            break;
        }
    }

    //--- Draw parts
    g_pFont->draw("FPS: " + std::to_string(oTiming->getFPS()), Vector2::Zero, OTopLeft, Color(0, .8f, 0, 1));
}

void postRender()
{
}
