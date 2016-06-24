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

void init();
void update();
void render();
void postRender();

OFontRef g_pFont;

struct Mesh
{
    struct Vertex
    {
        Vector2 position;
        Vector2 uv;
        Color color;
    };

    OVertexBufferRef pVB;
    OIndexBufferRef pIB;
    uint32_t indexCount;
};

Mesh planetMesh;
Mesh launchStationMesh;
Mesh cloudMesh;

OTextureRef pWhiteTexture;

#define PLANET_SIZE 100
#define PLANET_SIDES 360

#define ATMOSPHERES_COUNT 4

static const Color PLANET_COLOR = Color(0, 1, 0, 1).AdjustedSaturation(.5f);
static const Color ATMOSPHERE_BASE_COLOR = Color(0, .75f, 1, 1).AdjustedSaturation(.5f);
static const Color ATMOSPHERE_COLORS[ATMOSPHERES_COUNT] = {
    ATMOSPHERE_BASE_COLOR,
    ATMOSPHERE_BASE_COLOR * 0.75f,
    ATMOSPHERE_BASE_COLOR * 0.5f,
    ATMOSPHERE_BASE_COLOR * .25f
};

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

    // Create planet mesh + atmosphere n shit
    {
        Mesh::Vertex vertices[PLANET_SIDES + 1];
        uint16_t indices[PLANET_SIDES * 3];
        vertices[0].position = Vector2::Zero;
        vertices[0].color = PLANET_COLOR;
        for (int i = 0; i < PLANET_SIDES; ++i)
        {
            auto& vertex = vertices[i + 1];
            float angle = ((float)i / (float)PLANET_SIDES) * DirectX::XM_2PI;
            vertex.position.x = std::cosf(angle) * PLANET_SIZE;
            vertex.position.y = std::sinf(angle) * PLANET_SIZE;
            vertex.color = PLANET_COLOR;
            indices[i * 3 + 0] = 0;
            indices[i * 3 + 1] = i + 1;
            indices[i * 3 + 2] = ((i + 1) % PLANET_SIDES) + 1;
        }
        planetMesh.pVB = OVertexBuffer::createStatic(vertices, sizeof(vertices));
        planetMesh.pIB = OIndexBuffer::createStatic(indices, sizeof(indices));
        planetMesh.indexCount = PLANET_SIDES * 3;
    }
}

void update()
{
}

void drawMesh(const Matrix& transform, const Mesh& mesh)
{
    oRenderer->renderStates.textures[0] = pWhiteTexture;
    oRenderer->renderStates.world = transform;
    oRenderer->renderStates.vertexBuffer = mesh.pVB;
    oRenderer->renderStates.indexBuffer = mesh.pIB;
    oRenderer->drawIndexed(planetMesh.indexCount);
}

void render()
{
    oRenderer->clear({0, 0, 0, 1});
    oRenderer->setupFor2D();

    //--- Draw the world
    drawMesh(Matrix::CreateTranslation(400, 300, 0), planetMesh);

    g_pFont->draw("FPS: " + std::to_string(oTiming->getFPS()), Vector2::Zero, OTopLeft, Color(0, .8f, 0, 1));
}

void postRender()
{
}
