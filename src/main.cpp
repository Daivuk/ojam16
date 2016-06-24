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

Mesh starMesh;
Mesh atmosphereMesh;
Mesh planetMesh;
Mesh launchStationMesh;
Mesh cloudMesh;

OTextureRef pWhiteTexture;

#define PLANET_SIZE 100
#define PLANET_SIDES 360
#define ZOOM 100
#define ATMOSPHERES_COUNT 4
#define ATMOSPHERES_SCALE 0.05f
#define STAR_COUNT 300

static const Color PLANET_COLOR = Color(0, .5f, 0, 1).AdjustedSaturation(.5f);
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

    // Create planet mesh + atmospheres n shit
    {
        Mesh::Vertex vertices[PLANET_SIDES * 2 * ATMOSPHERES_COUNT];
        uint16_t indices[PLANET_SIDES * 6 * ATMOSPHERES_COUNT];
        for (int a = 0; a < ATMOSPHERES_COUNT; ++a)
        {
            auto vertOffset = PLANET_SIDES * 2 * a;
            auto indexOffset = PLANET_SIDES * 6 * a;
            float d1 = ((float)a + 1);
            float d2 = ((float)a);
            d1 *= d1;
            d2 *= d2;
            d1 = PLANET_SIZE + PLANET_SIZE * ATMOSPHERES_SCALE * d1;
            d2 = PLANET_SIZE + PLANET_SIZE * ATMOSPHERES_SCALE * d2;
            for (int i = 0; i < PLANET_SIDES; ++i)
            {
                float angle = ((float)i / (float)PLANET_SIDES) * DirectX::XM_2PI;
                {
                    auto& vertex = vertices[vertOffset + i * 2 + 0];
                    vertex.position.x = std::cosf(angle) * d1;
                    vertex.position.y = -std::sinf(angle) * d1;
                    vertex.color = ATMOSPHERE_COLORS[a];
                }
                {
                    auto& vertex = vertices[vertOffset + i * 2 + 1];
                    vertex.position.x = std::cosf(angle) * d2;
                    vertex.position.y = -std::sinf(angle) * d2;
                    vertex.color = ATMOSPHERE_COLORS[a];
                }
                indices[indexOffset + i * 6 + 0] = vertOffset + i * 2;
                indices[indexOffset + i * 6 + 1] = vertOffset + ((i + 1) % PLANET_SIDES) * 2;
                indices[indexOffset + i * 6 + 2] = vertOffset + i * 2 + 1;
                indices[indexOffset + i * 6 + 3] = vertOffset + ((i + 1) % PLANET_SIDES) * 2;
                indices[indexOffset + i * 6 + 4] = vertOffset + ((i + 1) % PLANET_SIDES) * 2 + 1;
                indices[indexOffset + i * 6 + 5] = vertOffset + i * 2 + 1;
            }
        }
        atmosphereMesh.pVB = OVertexBuffer::createStatic(vertices, sizeof(vertices));
        atmosphereMesh.pIB = OIndexBuffer::createStatic(indices, sizeof(indices));
        atmosphereMesh.indexCount = sizeof(indices) / 2;
    }
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
            vertex.position.y = -std::sinf(angle) * PLANET_SIZE;
            vertex.color = PLANET_COLOR;
            indices[i * 3 + 0] = 0;
            indices[i * 3 + 1] = i + 1;
            indices[i * 3 + 2] = ((i + 1) % PLANET_SIDES) + 1;
        }
        planetMesh.pVB = OVertexBuffer::createStatic(vertices, sizeof(vertices));
        planetMesh.pIB = OIndexBuffer::createStatic(indices, sizeof(indices));
        planetMesh.indexCount = sizeof(indices) / 2;
    }

    //--- Stars
    {
        Mesh::Vertex vertices[STAR_COUNT];
        for (int i = 0; i < STAR_COUNT; ++i)
        {
            auto& vertex = vertices[i];
            vertex.position = ORandVector2(OScreenf);
            vertex.color = Color::White * ORandFloat(.1f, .8f);
        }
        starMesh.pVB = OVertexBuffer::createStatic(vertices, sizeof(vertices));
        starMesh.indexCount = STAR_COUNT;
    }
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
    oRenderer->set2DCameraOffCenter({0, 0}, 2);
    drawMeshIndexed(Matrix::Identity, atmosphereMesh);
    drawMeshIndexed(Matrix::Identity, planetMesh);

    g_pFont->draw("FPS: " + std::to_string(oTiming->getFPS()), Vector2::Zero, OTopLeft, Color(0, .8f, 0, 1));
}

void postRender()
{
}
