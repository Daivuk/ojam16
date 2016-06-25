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
#include "meshes.h"

void init();
void update();
void render();
void postRender();

OFontRef g_pFont;
OTextureRef pWhiteTexture;

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
    createMeshes();
}

void update()
{
}

void drawMeshIndexed(const Matrix& transform, const Mesh& mesh)
{
    oRenderer->renderStates.textures[0] = pWhiteTexture;
    oRenderer->renderStates.world = transform;
    oRenderer->renderStates.vertexBuffer = mesh.pVB;
    oRenderer->renderStates.indexBuffer = mesh.pIB;
    oRenderer->drawIndexed(mesh.indexCount);
}

void drawMesh(const Matrix& transform, const Mesh& mesh)
{
    oRenderer->renderStates.textures[0] = pWhiteTexture;
    oRenderer->renderStates.world = transform;
    oRenderer->renderStates.vertexBuffer = mesh.pVB;
    oRenderer->draw(mesh.indexCount);
}

void render()
{
    oRenderer->clear({0, 0, 0, 1});

    //--- Draw the world
    oRenderer->setupFor2D();
    oRenderer->renderStates.primitiveMode = OPrimitivePointList;
    drawMesh(Matrix::Identity, starMesh);
    oRenderer->renderStates.primitiveMode = OPrimitiveTriangleList;
    oRenderer->set2DCameraOffCenter({0, -PLANET_SIZE}, ZOOM);
    drawMeshIndexed(Matrix::Identity, atmosphereMesh);
    drawMeshIndexed(Matrix::Identity, planetMesh);

    g_pFont->draw("FPS: " + std::to_string(oTiming->getFPS()), Vector2::Zero, OTopLeft, Color(0, .8f, 0, 1));
}

void postRender()
{
}
