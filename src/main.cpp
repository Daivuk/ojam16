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
#include <onut/Music.h>
#include <onut/Files.h>
#include <onut/ContentManager.h>

#include <vector>

#include "meshes.h"
#include "part.h"
#include "editor.h"
#include "particle.h"

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
OAnimVector2 cameraOffset;
OAnimVector2 cameraShaking;
int stageCount;

#define MINIMAP_SIZE 192

OMusicRef pMusic;
std::string currentMusic = "";

void updateMusic()
{
    if (pMusic)
    {
        if (pMusic->isDone())
        {
            pMusic = OMusic::createFromFile(oContentManager->findResourceFile(currentMusic), nullptr);
            pMusic->play();
        }
    }
}

void playMusic(const std::string& filename)
{
    if (currentMusic == filename) return;
    currentMusic = filename;
    pMusic = OMusic::createFromFile(oContentManager->findResourceFile(currentMusic), nullptr);
    pMusic->play();
}

int WINAPI WinMain(_In_ HINSTANCE hInstance,
                   _In_opt_ HINSTANCE hPrevInstance,
                   _In_ LPSTR lpCmdLine,
                   _In_ int nShowCmd)
{
    oSettings->setIsResizableWindow(true);
    oSettings->setIsFixedStep(true);
    oSettings->setGameName("Ottawa Game Jam 2016");
    oSettings->setResolution({800, 600});

    ORun(init, update, render, postRender);

    return 0;
}

void init()
{
    oTiming->setUpdateFps(30);
    g_pFont = OGetFont("font.fnt");
    uint32_t white = 0xFFFFFFFF;
    pWhiteTexture = OTexture::createFromData((uint8_t*)&white, {1, 1}, false);
    pMiniMap = OTexture::createRenderTarget({MINIMAP_SIZE, MINIMAP_SIZE}, false);
    createMeshes();
    
    initPartDefs();
    resetEditor();

    playMusic("OJAM2016_Music_Build.mp3");
}

extern float shakeAmount;

void updateCamera()
{
    if (!pMainPart) return;
    auto targetCamera = vehiculeRect(pMainPart).Center() - pMainPart->position;
    targetCamera = Vector2::Transform(targetCamera, Matrix::CreateRotationZ(pMainPart->angle));
    targetCamera += pMainPart->position;

    //cameraPos += (targetCamera - cameraPos) * ODT * 5.0f;
    cameraPos = targetCamera + cameraOffset.get() + cameraShaking.get();

    shakeAmount /= ((pMainPart->altitude < 1 ? 1 : pMainPart->altitude) / 100);
    shakeAmount = std::min(1.0f, shakeAmount);
    if (!cameraShaking.isPlaying())
    {
        Vector2 dir = ORandVector2(-Vector2::One, Vector2::One);
        cameraShaking.playFromCurrent(dir * shakeAmount * .05f, .05f, OTweenEaseOut);
    }
}

void decouple(Part* pPart)
{
    int side = 0;
    if (pPart->type == PART_DECOUPLER_HORIZONTAL_LEFT) side = -1;
    if (pPart->type == PART_DECOUPLER_HORIZONTAL_RIGHT) side = 1;
    auto mtransform = getWorldTransform(pPart);
    auto forward = mtransform.Up();
    auto right = mtransform.Right();
    right.Normalize();
    for (auto pChild : pPart->children)
    {
        auto transform = getWorldTransform(pChild);
        auto forward = transform.Up();
        right.Normalize();
        forward *= -1;
        forward.Normalize();
        auto currentDir = pChild->vel;
        currentDir.Normalize();
        pChild->angleVelocity += ORandFloat(-1, 1);
        if (side == 0)
        {
            pChild->vel -= currentDir;
        }
        else if (side == -1)
        {
            pChild->vel -= right * 2;
            pPart->vel -= right;
        }
        else if (side == 1)
        {
            pChild->vel += right * 2;
            pPart->vel += right;
        }
        pChild->angle = std::atan2f(forward.x, -forward.y);
        pChild->position = transform.Translation();
        pChild->pParent = nullptr;
        parts.push_back(pChild);
    }
    if (pPart->pParent)
    {
        auto currentDir = pPart->pParent->vel;
        currentDir.Normalize();
        auto pTopParent = getTopParent(pPart->pParent);
        if (side == 0)
        {
            pTopParent->vel += currentDir;
        }
        else if (side == -1)
        {
            pTopParent->vel -= right;
        }
        else if (side == 1)
        {
            pTopParent->vel += right;
        }
        for (auto it = pPart->pParent->children.begin(); it != pPart->pParent->children.end(); ++it)
        {
            if (*it == pPart)
            {
                pPart->pParent->children.erase(it);
                break;
            }
        }
    }
    forward *= -1;
    forward.Normalize();
    pPart->angle = std::atan2f(forward.x, -forward.y);
    pPart->position = mtransform.Translation();
    pPart->pParent = nullptr;
    pPart->angleVelocity += ORandFloat(-1, 1);
    pPart->children.clear();
    parts.push_back(pPart);

    extern OTextureRef pSmokeTexture;
    spawnParticles({
        pPart->position,
        pPart->vel - Vector2(right) * ORandFloat(.15f, .25f),
        0,
        1,
        Color(1, 1, 1, 1), Color(0, 0, 0, 0),
        .5f, 0.65f,
        2.0f,
        45.0f,
        pSmokeTexture
    }, 3, 30, 360.0f, 0, 0, -right);
    spawnParticles({
        pPart->position,
        pPart->vel + Vector2(right) * ORandFloat(.15f, .25f),
        0,
        1,
        Color(1, 1, 1, 1), Color(0, 0, 0, 0),
        .5f, 0.65f,
        2.0f,
        45.0f,
        pSmokeTexture
    }, 3, 30, 360.0f, 0, 0, right);
}
//Vector2 position;
//Vector2 vel;
//float life;
//float duration;
//Color colorFrom, colorTo;
//float sizeFrom, sizeTo;
//float angle;
//float angleVel;
//OTextureRef pTexture;

void activateNextStage()
{
    if (!pMainPart) return;
    auto cameraBefore = vehiculeRect(pMainPart).Center() - pMainPart->position;
    if (stages.size() > 1)
    {
        auto currentState = stages.back();
        stages.erase(stages.end() - 1);
        auto& newStage = stages.back();
        for (auto pPart : newStage)
        {
            pPart->isActive = true;
            if (pPart->type == PART_DECOUPLER ||
                pPart->type == PART_DECOUPLER_WIDE ||
                pPart->type == PART_DECOUPLER_HORIZONTAL_LEFT ||
                pPart->type == PART_DECOUPLER_HORIZONTAL_RIGHT)
            {
                decouple(pPart);
            }
        }
        auto cameraAfter = vehiculeRect(pMainPart).Center() - pMainPart->position;
        auto cameraOffsetf = cameraBefore - cameraAfter;
        cameraOffsetf = Vector2::Transform(cameraOffsetf, Matrix::CreateRotationZ(pMainPart->angle));
        cameraOffset.play(cameraOffsetf, Vector2::Zero, 1, OTweenEaseOut);
    }
    else
    {
        // End game?
    }
}

void controlTheFuckingRocket()
{
    if (!pMainPart) return;
    if (OInputPressed(OKeyLeft))
    {
        pMainPart->angleVelocity -= (10 + getTotalStability(pMainPart)) / getTotalMass(pMainPart) * ODT;
    }
    else if (OInputPressed(OKeyRight))
    {
        pMainPart->angleVelocity += (10 + getTotalStability(pMainPart)) / getTotalMass(pMainPart) * ODT;
    }
}

void update()
{
    updateMusic();
    switch (gameState)
    {
        case GAME_STATE_EDITOR:
        {
            if (OInputJustPressed(OKeyEscape))
            {
                resetEditor();
            }
            else if (OInputJustPressed(OKeySpaceBar) && pMainPart)
            {
                gameState = GAME_STATE_STAND_BY;
                auto vrect = vehiculeRect(pMainPart);
                pMainPart->position = {0, -PLANET_SIZE - vrect.w};
                pMainPart->angle = 0;
                vrect = vehiculeRect(pMainPart);
                cameraPos = vrect.Center();
                stageCount = (int)stages.size();
                stages.push_back({}); // Add empty stage at the end so we can start with nothing happening
                zoom = 256.0f / (vrect.w / 2);
                zoom = std::min(64.0f, zoom);
                extern Part* pHoverPart;
                pHoverPart = nullptr;
                playMusic("OJAM2016_Music_Launch.mp3");
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
                activateNextStage();
                gameState = GAME_STATE_FLIGHT;
            }
            updateCamera();
            break;
        }
        case GAME_STATE_FLIGHT:
        {
            if (OInputJustPressed(OKeySpaceBar))
            {
                activateNextStage();
            }
            controlTheFuckingRocket();
            toKill.clear();
            for (auto pPart : parts) updatePart(pPart);
            for (auto pToKill : toKill)
            {
                deletePart(pToKill);
            }
            toKill.clear();
            updateCamera();
            break;
        }
    }
    updateParticles();
}

void drawWorld()
{
    oRenderer->setupFor2D();
    oRenderer->renderStates.primitiveMode = OPrimitivePointList;
    drawMesh(Matrix::CreateScale(OScreenWf / 800.0f), starMesh);
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
    //extern Vector2 centerOfMass;
    //oSpriteBatch->drawCross(centerOfMass + parts[0]->position, .05f, Color(0, .5f, 1, 1));
    //oSpriteBatch->drawOutterOutlineRect(vehiculeRect(parts[0]), .1f, Color(1, 1, 0));
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

    static Vector2 orbit[60];

    //--- Draw it in the top right corner
    oSpriteBatch->begin();
    oSpriteBatch->drawRect(pMiniMap, {OScreenWf - MINIMAP_SIZE, 0, MINIMAP_SIZE, MINIMAP_SIZE});
    oSpriteBatch->end();
}

void drawStages()
{
    // Draw stages on the right
    oSpriteBatch->begin();
    oSpriteBatch->drawRect(nullptr, {0, 0, 132.f, OScreenHf}, Color(0, 0, 0, .5f));
    Vector2 stageTextPos(20.0f, 20.0f);
    int stageId = stageCount;
    for (auto& stage : stages)
    {
        g_pFont->draw("--- Stage " + std::to_string(stageId) + " ---", stageTextPos, OTopLeft, Color(1, 1, 1));
        stageTextPos.y += 16;
        for (auto pPart : stage)
        {
            auto& partDef = partDefs[pPart->type];
            g_pFont->draw(partDef.name, stageTextPos, OTopLeft, Color(0, 1, 1));
            stageTextPos.y += 16;
            if (pPart->type == PART_SOLID_ROCKET)
            {
                oSpriteBatch->drawRect(nullptr, {stageTextPos.x, stageTextPos.y + 1, (pPart->solidFuel) / (partDef.solidFuel)* 100.0f, 14.f}, Color(1, 1, 0, 1));
                stageTextPos.y += 16;
            }
            if (pPart->type == PART_LIQUID_ROCKET_WIDE ||
                pPart->type == PART_LIQUID_ROCKET_THIN)
            {
                float liquidFuel = 0;
                float maxLiquidFuel = 0;
                getLiquidFuel(pPart, liquidFuel, maxLiquidFuel);
                if (maxLiquidFuel > 0)
                {
                    oSpriteBatch->drawRect(nullptr, {stageTextPos.x, stageTextPos.y + 1, liquidFuel / maxLiquidFuel * 100.0f, 14.f}, Color(1, 1, 0, 1));
                    stageTextPos.y += 16;
                }
            }
        }
        --stageId;
    }
    oSpriteBatch->end();
}

void render()
{
    oRenderer->clear({0, 0, 0, 1});
    oSpriteBatch->changeFiltering(OFilterNearest);
    oRenderer->renderStates.sampleFiltering = OFilterNearest;

    switch (gameState)
    {
        case GAME_STATE_EDITOR:
        {
            drawEditor();
            break;
        }
        case GAME_STATE_STAND_BY:
        {
            drawWorld();
            oSpriteBatch->begin();
            oRenderer->set2DCameraOffCenter(cameraPos, zoom);
            drawParticles();
            oSpriteBatch->end();
            drawParts();
            drawStages();
            drawMiniMap();
            break;
        }
        case GAME_STATE_FLIGHT:
        {
            drawWorld();
            drawParts();
            oSpriteBatch->begin();
            oRenderer->set2DCameraOffCenter(cameraPos, zoom);
            drawParticles();
            oSpriteBatch->end();
            drawStages();
            drawMiniMap();
            break;
        }
    }
    oSpriteBatch->begin();
    static float altitude = 0;
    static float speed = 0;
    if (pMainPart)
    {
        altitude = pMainPart->altitude;
        speed = pMainPart->speed;
    }
    oSpriteBatch->drawRect(nullptr, {OScreenCenterXf - 50, 0, 100, 32}, Color(0, 0, 0, .5f));
    g_pFont->draw("ALT: " + std::to_string((int)altitude) + " m", {OScreenCenterXf, 0}, OTop, Color(1, .5f, 0, 1));
    g_pFont->draw("SPD: " + std::to_string((int)speed) + " m/s", {OScreenCenterXf, 16.0f}, OTop, Color(1, .5f, 0, 1));
    g_pFont->draw("FPS: " + std::to_string(oTiming->getFPS()), Vector2::Zero, OTopLeft, Color(0, .8f, 0, 1));
    oSpriteBatch->end();
}

void postRender()
{
}
