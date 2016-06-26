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
#include <onut/Sound.h>

#include <vector>
#include <iomanip>
#include <sstream>

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
bool hasStableOrbit = false;
OAnimFloat orbitIndicatorAnim;
float endTimer = 0.0f;
float scafoldingPos = 0;

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
    pMusic->setVolume(.75f);
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
    orbitIndicatorAnim.play(.5f, 1.0f, .35f, OTweenEaseBoth, OPingPongLoop);
    
    initPartDefs();
    resetEditor();

    playMusic("OJAM2016_Music_Build.mp3");
}

extern float shakeAmount;
std::vector<Vector2> plotPoints;

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
    OPlayRandomSound({"Decouple01.wav", "Decouple02.wav", "Decouple03.wav", "Decouple04.wav"}, 2);
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
        OPlaySound("NextStageSignal.wav");
        for (auto pPart : newStage)
        {
            pPart->isActive = true;
            if (pPart->type == PART_SOLID_ROCKET)
            {
                OPlayRandomSound({"RocketFire01.wav", "RocketFire02.wav", "RocketFire03.wav", "RocketFire04.wav"}, 2);
                OPlayRandomSound({"RocketFire01.wav", "RocketFire02.wav", "RocketFire03.wav", "RocketFire04.wav"}, 2, 0, 0.75f);
            }
            else if (pPart->type == PART_LIQUID_ROCKET_THIN)
            {
                pPart->pSound = OGetSound("LiquidEngineLoop.wav")->createInstance();
                pPart->pSound->setLoop(true);
                pPart->pSound->play();
            }
            else if (pPart->type == PART_LIQUID_ROCKET_WIDE)
            {
                pPart->pSound = OGetSound("LiquidEngineLoop.wav")->createInstance();
                pPart->pSound->setVolume(3);
                pPart->pSound->setLoop(true);
                pPart->pSound->play();
            }
            else if (pPart->type == PART_TOP_CONE)
            {
                if (hasStableOrbit)
                {
                    switch (ORandInt(0, 3))
                    {
                        case 0:
                            partDefs[PART_SATELLITE].pTexture = OGetTexture("SATELLITE_1.png");
                            break;
                        case 1:
                            partDefs[PART_SATELLITE].pTexture = OGetTexture("SATELLITE_2.png");
                            break;
                        case 2:
                            partDefs[PART_SATELLITE].pTexture = OGetTexture("SATELLITE_3.png");
                            break;
                        case 3:
                            partDefs[PART_SATELLITE].pTexture = OGetTexture("SATELLITE_4.png");
                            break;
                    }
                    partDefs[PART_SATELLITE].hsize = partDefs[PART_SATELLITE].pTexture->getSizef() / 128.0f;
                    pPart->type = PART_SATELLITE;
                    playMusic("SatelliteLoop.mp3");
                }
                else
                {
                    explodePart(pPart);
                    return;
                }
            }
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
        pMainPart->angleVelocity -= (10 + getTotalStability(pMainPart) * 4) / getTotalMass(pMainPart) * ODT;
    }
    else if (OInputPressed(OKeyRight))
    {
        pMainPart->angleVelocity += (10 + getTotalStability(pMainPart) * 4) / getTotalMass(pMainPart) * ODT;
    }
}

int voiceTrigger = 0;
OMusicRef pCurrentVoice;

void updateVoices()
{
    ++voiceTrigger;
    if (voiceTrigger == 300)
    {
        voiceTrigger = 0;
        std::stringstream ss;
        ss << "SpySatellite_Voice_Secrets_" << std::setw(2) << std::setfill('0') << ORandInt(1, 66) << ".mp3";
        pCurrentVoice = OMusic::createFromFile(oContentManager->findResourceFile(ss.str()), nullptr);
        pCurrentVoice->play();
    }
}

float getSpaceDistance()
{
    float d1 = ((float)ATMOSPHERES_COUNT);
    d1 *= d1;
    d1 = PLANET_SIZE + PLANET_SIZE * ATMOSPHERES_SCALE * d1;
    return d1;
}

void updateOrbit()
{
    plotPoints.clear();
    hasStableOrbit = false;
    // Draw the orbit
    if (pMainPart)
    {
        // Prepare our data
        auto vel = pMainPart->vel;
        auto velSpeed = vel.Length();
        auto velDir = vel;
        auto position = pMainPart->position;
        auto dirToCenter = position;
        dirToCenter.Normalize();
        velDir.Normalize();

        float currentDot = velDir.Dot(dirToCenter);
        if (currentDot < 1 && currentDot > -1)
        {
            float time = 0.0f;
            float step = .1f;
            float testDot = currentDot;
            Vector2 highestPoint = position;
            while (plotPoints.size() < 2 && time < 300.0f)
            {
                while (((currentDot >= 0 && testDot >= 0) ||
                    (currentDot <= 0 && testDot <= 0)) && time < 300.0f)
                {
                    highestPoint += vel;
                    velDir = vel;
                    velDir.Normalize();
                    Vector2 Vg = highestPoint;
                    Vg.Normalize();
                    testDot = velDir.Dot(Vg);
                    vel -= Vg * GRAVITY;
                    time += step;
                }
                plotPoints.push_back(highestPoint);
                currentDot = testDot;
            }
            // Get next one!
            if (time < 300.0f)
            {
                if (plotPoints[0].Length() >= getSpaceDistance() &&
                    plotPoints[1].Length() >= getSpaceDistance())
                {
                    hasStableOrbit = true;
                }
                plotPoints.push_back(-plotPoints[0]);
                plotPoints.push_back(-plotPoints[1]);
            }
        }
    }
}

extern int spawn;

void update()
{
    spawn++;
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
                voiceTrigger = 200;
                auto vrect = vehiculeRect(pMainPart);
                scafoldingPos = vrect.z / 2;
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
                plotPoints.clear();
                hasStableOrbit = false;
                shakeAmount = 0;
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
                endTimer = 5.0f;
            }
            for (auto pPart : parts) updatePart(pPart);
            updateCamera();
            updateVoices();
            if (OInputJustPressed(OKeyEscape))
            {
                resetEditor();
                gameState = GAME_STATE_EDITOR;
                playMusic("OJAM2016_Music_Build.mp3");
            }
            break;
        }
        case GAME_STATE_FLIGHT:
        {
            if (OInputJustPressed(OKeySpaceBar))
            {
                activateNextStage();
            }
            controlTheFuckingRocket();
            for (auto pPart : parts) updatePart(pPart);
            for (auto pToKill : toKill)
            {
                deletePart(pToKill);
            }
            toKill.clear();
            updateCamera();
            updateVoices();
            updateOrbit();
            if (pMainPart)
            {
                if (pMainPart->type == PART_SATELLITE) endTimer -= ODT;
            }
            else
            {
                endTimer -= ODT;
            }
            if (endTimer <= 0.f || OInputJustPressed(OKeyEscape))
            {
                resetEditor();
                gameState = GAME_STATE_EDITOR;
                playMusic("OJAM2016_Music_Build.mp3");
            }
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

    oSpriteBatch->begin();
    oRenderer->set2DCameraOffCenter(cameraPos, zoom);
    oSpriteBatch->drawRect(OGetTexture("BGstuff.png"),
                           Rect(
                           -346.0f / 64.0f,
                           -PLANET_SIZE - 168.0f / 64.0f,
                           791.0f / 64.0f,
                           588.0f / 64.0f));
    oSpriteBatch->drawRect(OGetTexture("BGscaffold.png"),
                           Rect(
                           -51.0f / 64.0f + .5f,// + scafoldingPos,
                           -PLANET_SIZE - 660.0f / 64.0f,
                           339.0f / 64.0f,
                           681.0f / 64.0f));
    oSpriteBatch->end();
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

    // Draw the orbit
    if (pMainPart)
    {
        // Prepare our data
        auto position = pMainPart->position;
        oPrimitiveBatch->begin(OPrimitiveLineStrip);
        oRenderer->set2DCameraOffCenter(Vector2::Zero, zoomf);
        oPrimitiveBatch->draw(position + Vector2(-1000, 0), Color(1, 0, 1));
        oPrimitiveBatch->draw(position + Vector2(0, 1000), Color(1, 0, 1));
        oPrimitiveBatch->draw(position + Vector2(1000, 0), Color(1, 0, 1));
        oPrimitiveBatch->draw(position + Vector2(0, -1000), Color(1, 0, 1));
        oPrimitiveBatch->draw(position + Vector2(-1000, 0), Color(1, 0, 1));
        oPrimitiveBatch->end();
    }

    if (plotPoints.size() == 4)
    {
        oPrimitiveBatch->begin(OPrimitiveLineStrip);
        oRenderer->set2DCameraOffCenter(Vector2::Zero, zoomf);
        Color orbitColor = Color(.75f, .75f, .75f, 1);
        if (hasStableOrbit) orbitColor *= orbitIndicatorAnim.get();
        for (int i = 0; i < 4; ++i)
        {
            Vector2 p0 = plotPoints[i];
            Vector2 p3 = plotPoints[(i + 1) % 4];
            Vector2 p1 = p0 + p3 * (2.0f / 3);
            Vector2 p2 = p3 + p0 * (2.0f / 3);
            for (int i = 0; i < 100; ++i)
            {
                float t = (float)i / 100.0f;
                float invT = 1 - t;
                auto pt =
                    p0 * invT * invT * invT +
                    3 * p1 * t * invT * invT +
                    3 * p2 * t * t * invT +
                    p3 * t * t * t;
                oPrimitiveBatch->draw(pt, orbitColor);
            }
        }
        oPrimitiveBatch->end();
        /*
        oPrimitiveBatch->begin(OPrimitiveLineList);
        oRenderer->set2DCameraOffCenter(Vector2::Zero, zoomf);
        for (int i = 0; i < 2; ++i)
        {
            Vector2 p0 = plotPoints[i];
            Vector2 p3 = plotPoints[(i + 2) % 4];
            oPrimitiveBatch->draw(p0);
            oPrimitiveBatch->draw(p3);
        }
        oPrimitiveBatch->end();

        int i = 0;
        for (auto& pt : plotPoints)
        {
            Color color;
            switch (i++)
            {
                case 0: color = Color(1, 0, 0); break;
                case 1: color = Color(0, 1, 0); break;
                case 2: color = Color(0, 1, 1); break;
                case 3: color = Color(1, 1, 0); break;
            }
            oPrimitiveBatch->begin(OPrimitiveLineStrip);
            oRenderer->set2DCameraOffCenter(Vector2::Zero, zoomf);
            oPrimitiveBatch->draw(pt + Vector2(-1000, 0), color);
            oPrimitiveBatch->draw(pt + Vector2(1000, 0), color);
            oPrimitiveBatch->draw(pt + Vector2(0, -1000), color);
            oPrimitiveBatch->draw(pt + Vector2(0, 1000), color);
            oPrimitiveBatch->end();
        }*/
    }

    oRenderer->renderStates.renderTarget = nullptr;
    oRenderer->renderStates.viewport.pop();

    //--- Draw it in the top right corner
    oSpriteBatch->begin();
    oSpriteBatch->drawRect(pMiniMap, {OScreenWf - MINIMAP_SIZE, 0, MINIMAP_SIZE, MINIMAP_SIZE});
    oSpriteBatch->drawRect(nullptr, {OScreenWf - MINIMAP_SIZE, OScreenHf - MINIMAP_SIZE, MINIMAP_SIZE, MINIMAP_SIZE}, Color(0, 0, 0, .5f));
    oSpriteBatch->end();
}

void drawHUD()
{
    if (pMainPart)
    {
        Vector2 radarPos = Vector2(OScreenWf - MINIMAP_SIZE / 2, OScreenHf - MINIMAP_SIZE / 2);
        float radarSize = 60;
        auto planetVector = pMainPart->position;
        float shipAngle = std::atan2f(planetVector.x, -planetVector.y);
        oPrimitiveBatch->begin(OPrimitiveLineStrip);
        for (int i = 0; i <= 360; i += 15)
        {
            float angle = DirectX::XMConvertToRadians((float)i) + shipAngle;
            oPrimitiveBatch->draw(radarPos + Vector2(std::cosf(angle) * radarSize, std::sinf(angle) * radarSize));
        }
        oPrimitiveBatch->end();
        oPrimitiveBatch->begin(OPrimitiveLineList);
        for (int i = 0; i <= 360; i += 15)
        {
            float angle = DirectX::XMConvertToRadians((float)i) + shipAngle;
            float dist = 3;
            if (i % 45) dist = 6;
            oPrimitiveBatch->draw(radarPos + Vector2(std::cosf(angle) * radarSize, std::sinf(angle) * radarSize));
            oPrimitiveBatch->draw(radarPos + Vector2(std::cosf(angle) * (radarSize + dist), std::sinf(angle) * (radarSize + dist)));
        }
        oPrimitiveBatch->end();
        oPrimitiveBatch->begin(OPrimitiveTriangleList);
        Color planetColor(0, .5f, 0, .75f);
        planetColor = planetColor.AdjustedSaturation(.5f);
        for (int i = 0; i < 180; i += 15)
        {
            float angle1 = DirectX::XMConvertToRadians((float)i) + shipAngle;
            float angle2 = DirectX::XMConvertToRadians((float)i + 15) + shipAngle;
            oPrimitiveBatch->draw(radarPos + Vector2(std::cosf(angle1) * (radarSize - 2), std::sinf(angle1) * (radarSize - 2)), planetColor);
            oPrimitiveBatch->draw(radarPos + Vector2(std::cosf(angle2) * (radarSize - 2), std::sinf(angle2) * (radarSize - 2)), planetColor);
            oPrimitiveBatch->draw(radarPos, planetColor);
        }
        auto arrowColor = Color(1, 0, 1, .75f);
        oPrimitiveBatch->end();
        oPrimitiveBatch->begin(OPrimitiveLineList);
        oPrimitiveBatch->draw(radarPos, arrowColor);
        auto a90 = DirectX::XM_PI / 2;
        oPrimitiveBatch->draw(radarPos - Vector2(std::cosf(pMainPart->angle + a90) * radarSize, std::sinf(pMainPart->angle + a90) * radarSize), arrowColor);
        oPrimitiveBatch->draw(radarPos - Vector2(std::cosf(pMainPart->angle + a90) * radarSize, std::sinf(pMainPart->angle + a90) * radarSize), arrowColor);
        oPrimitiveBatch->draw(radarPos - Vector2(std::cosf(pMainPart->angle + a90 + .1f) * (radarSize - 10), std::sinf(pMainPart->angle + a90 + .1f) * (radarSize - 10)), arrowColor);
        oPrimitiveBatch->draw(radarPos - Vector2(std::cosf(pMainPart->angle + a90) * radarSize, std::sinf(pMainPart->angle + a90) * radarSize), arrowColor);
        oPrimitiveBatch->draw(radarPos - Vector2(std::cosf(pMainPart->angle + a90 - .1f) * (radarSize - 10), std::sinf(pMainPart->angle + a90 - .1f) * (radarSize - 10)), arrowColor);
        oPrimitiveBatch->end();
    }
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
            drawParts();
            oSpriteBatch->begin();
            oRenderer->set2DCameraOffCenter(cameraPos, zoom);
            drawParticles();
            oSpriteBatch->end();
            drawStages();
            drawMiniMap();
            drawHUD();
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
            drawHUD();
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
    Color altColor = Color(1.5, 1, 0, 1);
    oSpriteBatch->drawRect(nullptr, {OScreenCenterXf - 50, 0, 100, 32}, Color(0, 0, 0, .5f));
    g_pFont->draw("ALT: " + std::to_string((int)altitude) + " m", {OScreenCenterXf, 4}, OTop, altColor);
    g_pFont->draw("SPD: " + std::to_string((int)speed) + " m/s", {OScreenCenterXf, 20.0f}, OTop, altColor);
    g_pFont->draw("FPS: " + std::to_string(oTiming->getFPS()), Vector2::Zero, OTopLeft, Color(0, .8f, 0, 1));

    if (hasStableOrbit)
    {
        g_pFont->draw("STABLE ORBIT", {OScreenWf - MINIMAP_SIZE / 2, 0}, OTop, Color(orbitIndicatorAnim.get()));
    }

    oSpriteBatch->end();
}

void postRender()
{
}
