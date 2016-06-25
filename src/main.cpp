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

OAnimFloat zoomAnim;
float zoom = .01f;
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
                parts[0]->position = {0, -PLANET_SIZE - vehiculeRect(parts[0]).w};
                zoomAnim.queue(1, 1);
                zoomAnim.queue(ZOOM, 5.0f, OTweenEaseBoth);
                zoomAnim.play();
                cameraPos = vehiculeRect(parts[0]).Center();
            }
            else
            {
                updateEditor(ODT);
            }
            break;
        }
        case GAME_STATE_STAND_BY:
        {
            if (OInputJustPressed(OKeySpaceBar))
            {
                zoomAnim.stop(true);
            }
            zoom = zoomAnim.get();
            updateCamera();
            break;
        }
        case GAME_STATE_FLIGHT:
        {
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
    oRenderer->set2DCameraOffCenter(cameraPos, std::powf((zoom - .01f) / ZOOM, 3) * ZOOM + .01f);
    drawMeshIndexed(Matrix::Identity, atmosphereMesh);
    drawMeshIndexed(Matrix::Identity, planetMesh);
}

void drawParts()
{
    oSpriteBatch->begin();
    oRenderer->set2DCameraOffCenter(cameraPos, std::powf((zoom - .01f) / ZOOM, 3) * ZOOM + .01f);
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
