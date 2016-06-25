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
#include <vector>
#include <onut/Anim.h>
#include <onut/Input.h>

void init();
void update();
void render();
void postRender();

OFontRef g_pFont;
OTextureRef pWhiteTexture;

struct Part;
using Parts = std::vector<Part*>;

struct Part
{
    Vector2 position;
    float angle;
    Mesh* pMesh;
    Parts children;
    bool fixed = false;
};

Parts parts;
Part* pMainPart = nullptr;
OAnimFloat zoomAnim;
float zoom = .01f;

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

Part* createSolidRocket(const Vector2& position, float angle)
{
    Part* pPart = new Part();
    pPart->position = position;
    pPart->angle = angle;
    pPart->pMesh = &solidRocketMesh;
    return pPart;
}

Part* createTopCone(const Vector2& position, float angle)
{
    Part* pPart = new Part();
    pPart->position = position;
    pPart->angle = angle;
    pPart->pMesh = &coneMesh;
    return pPart;
}

void createSampleRocket()
{
    auto pTopCone = createTopCone({0, -PLANET_SIZE - 2.5f}, 0.0f);
    auto pSolidRocket1 = createSolidRocket({0, 1.0f}, 0.0f);
    auto pSolidRocket2 = createSolidRocket({0, 1.25f}, 0.0f);

    pMainPart = pTopCone;
    parts.push_back(pTopCone);
    pTopCone->children.push_back(pSolidRocket1);
    pSolidRocket1->children.push_back(pSolidRocket2);
}

void init()
{
    oTiming->setUpdateFps(60);
    g_pFont = OGetFont("font.fnt");
    uint32_t white = 0xFFFFFFFF;
    pWhiteTexture = OTexture::createFromData((uint8_t*)&white, {1, 1}, false);
    createMeshes();
    
    createSampleRocket();

    zoomAnim.queue(1, 1);
    zoomAnim.queue(ZOOM, 5.0f, OTweenEaseBoth);
    zoomAnim.play();
}

void update()
{
    if (OInputJustPressed(OKeySpaceBar))
    {
        zoomAnim.stop(true);
    }
    zoom = zoomAnim.get();
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

void drawParts(const Parts& parts, const Matrix& parentTransform)
{
    for (auto pPart : parts)
    {
        Matrix transform = parentTransform * Matrix::CreateRotationZ(pPart->angle) * Matrix::CreateTranslation(pPart->position);
        drawMeshIndexed(transform, *pPart->pMesh);
        drawParts(pPart->children, transform);
    }
}

void render()
{
    oRenderer->clear({0, 0, 0, 1});

    //--- Draw the world
    oRenderer->setupFor2D();
    oRenderer->renderStates.primitiveMode = OPrimitivePointList;
    drawMesh(Matrix::Identity, starMesh);
    oRenderer->renderStates.primitiveMode = OPrimitiveTriangleList;
    oRenderer->set2DCameraOffCenter({0, -PLANET_SIZE}, std::powf((zoom - .01f) / ZOOM, 3) * ZOOM + .01f);
    drawMeshIndexed(Matrix::Identity, atmosphereMesh);
    drawMeshIndexed(Matrix::Identity, planetMesh);

    //--- Draw parts
    drawParts(parts, Matrix::Identity);

    g_pFont->draw("FPS: " + std::to_string(oTiming->getFPS()), Vector2::Zero, OTopLeft, Color(0, .8f, 0, 1));
}

void postRender()
{
}
