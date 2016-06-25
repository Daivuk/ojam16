#include "meshes.h"
#include <onut/Random.h>
#include <onut/Renderer.h>
#include <vector>
#include <onut/Curve.h>

Mesh starMesh;
Mesh atmosphereMesh;
Mesh planetMesh;
Mesh launchStationMesh;
Mesh cloudMesh;

Mesh solidRocketMesh;
Mesh coneMesh;

using Vertices = std::vector<Mesh::Vertex>;
using Indices = std::vector<uint16_t>;

void createCircle(Vertices& vertices, Indices& indices, const Vector2& center, float radius, const Color& color, int sides)
{
    vertices.push_back({center, Vector2::Zero, color});
    int vertexOffset = vertices.size();
    for (int i = 0; i < sides; ++i)
    {
        Mesh::Vertex vertex;
        float angle = ((float)i / (float)sides) * DirectX::XM_2PI;
        vertex.position.x = std::cosf(angle) * PLANET_SIZE;
        vertex.position.y = -std::sinf(angle) * PLANET_SIZE;
        vertex.color = PLANET_COLOR;
        vertices.push_back(vertex);
        indices.push_back(vertexOffset);
        indices.push_back(vertexOffset + i + 1);
        indices.push_back(vertexOffset + ((i + 1) % sides) + 1);
    }
}

void createCylinder(Vertices& vertices, Indices& indices, const Vector2& base, float radius, float height, const Color& color)
{
    Mesh::Vertex vertex;
    int vertexOffset = vertices.size();

    auto darkerColor = OLerp(color, Color(0, .1f, .2f, 1), .4f);
    auto darkColor = OLerp(color, Color(0, .1f, .2f, 1), .25f);

    vertex.position = {base.x - radius, base.y - height};
    vertex.color = darkerColor;
    vertices.push_back(vertex);
    vertex.position = {base.x - radius, base.y};
    vertex.color = darkerColor;
    vertices.push_back(vertex);

    vertex.position = {base.x + radius * .3f, base.y - height};
    vertex.color = color;
    vertices.push_back(vertex);
    vertex.position = {base.x + radius * .3f, base.y};
    vertex.color = color;
    vertices.push_back(vertex);

    vertex.position = {base.x + radius, base.y - height};
    vertex.color = darkColor;
    vertices.push_back(vertex);
    vertex.position = {base.x + radius, base.y};
    vertex.color = darkColor;
    vertices.push_back(vertex);

    indices.push_back(vertexOffset);
    indices.push_back(vertexOffset + 1);
    indices.push_back(vertexOffset + 2);
    indices.push_back(vertexOffset + 1);
    indices.push_back(vertexOffset + 3);
    indices.push_back(vertexOffset + 2);
    indices.push_back(vertexOffset + 2);
    indices.push_back(vertexOffset + 1 + 2);
    indices.push_back(vertexOffset + 2 + 2);
    indices.push_back(vertexOffset + 1 + 2);
    indices.push_back(vertexOffset + 3 + 2);
    indices.push_back(vertexOffset + 2 + 2);
}

void createCone(Vertices& vertices, Indices& indices, const Vector2& base, float radius, float height, const Color& color)
{
    Mesh::Vertex vertex;
    int vertexOffset = vertices.size();

    auto darkerColor = OLerp(color, Color(0, .1f, .2f, 1), .4f);
    auto darkColor = OLerp(color, Color(0, .1f, .2f, 1), .25f);

    vertex.position = {base.x - radius + height * .75f, base.y - height};
    vertex.color = darkerColor;
    vertices.push_back(vertex);
    vertex.position = {base.x - radius, base.y};
    vertex.color = darkerColor;
    vertices.push_back(vertex);

    vertex.position = {base.x, base.y - height};
    vertex.color = color;
    vertices.push_back(vertex);
    vertex.position = {base.x + radius * .3f, base.y};
    vertex.color = color;
    vertices.push_back(vertex);

    vertex.position = {base.x + radius - height * .75f, base.y - height};
    vertex.color = darkColor;
    vertices.push_back(vertex);
    vertex.position = {base.x + radius, base.y};
    vertex.color = darkColor;
    vertices.push_back(vertex);

    indices.push_back(vertexOffset);
    indices.push_back(vertexOffset + 1);
    indices.push_back(vertexOffset + 2);
    indices.push_back(vertexOffset + 1);
    indices.push_back(vertexOffset + 3);
    indices.push_back(vertexOffset + 2);
    indices.push_back(vertexOffset + 2);
    indices.push_back(vertexOffset + 1 + 2);
    indices.push_back(vertexOffset + 2 + 2);
    indices.push_back(vertexOffset + 1 + 2);
    indices.push_back(vertexOffset + 3 + 2);
    indices.push_back(vertexOffset + 2 + 2);
}

void createEngine(Vertices& vertices, Indices& indices, const Vector2& base, float radius, float height, const Color& color)
{
    Mesh::Vertex vertex;
    int vertexOffset = vertices.size();

    auto darkerColor = OLerp(color, Color(0, .1f, .2f, 1), .4f);
    auto darkColor = OLerp(color, Color(0, .1f, .2f, 1), .25f);

    vertex.position = {base.x - radius + height * .75f, base.y};
    vertex.color = darkerColor;
    vertices.push_back(vertex);
    vertex.position = {base.x - radius, base.y + height};
    vertex.color = darkerColor;
    vertices.push_back(vertex);

    vertex.position = {base.x + radius * .3f, base.y};
    vertex.color = color;
    vertices.push_back(vertex);
    vertex.position = {base.x + radius * .3f, base.y + height};
    vertex.color = color;
    vertices.push_back(vertex);

    vertex.position = {base.x + radius - height * .75f, base.y};
    vertex.color = darkColor;
    vertices.push_back(vertex);
    vertex.position = {base.x + radius, base.y + height};
    vertex.color = darkColor;
    vertices.push_back(vertex);

    indices.push_back(vertexOffset);
    indices.push_back(vertexOffset + 1);
    indices.push_back(vertexOffset + 2);
    indices.push_back(vertexOffset + 1);
    indices.push_back(vertexOffset + 3);
    indices.push_back(vertexOffset + 2);
    indices.push_back(vertexOffset + 2);
    indices.push_back(vertexOffset + 1 + 2);
    indices.push_back(vertexOffset + 2 + 2);
    indices.push_back(vertexOffset + 1 + 2);
    indices.push_back(vertexOffset + 3 + 2);
    indices.push_back(vertexOffset + 2 + 2);
}

void createAtmospheres()
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
                vertex.color = ATMOSPHERE_COLORS[a + 1];
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

void createPlanet()
{
    Vertices vertices;
    Indices indices;
    createCircle(vertices, indices, {0, 0}, PLANET_SIZE, PLANET_COLOR, PLANET_SIDES);
    planetMesh.pVB = OVertexBuffer::createStatic(vertices.data(), vertices.size() * sizeof(Mesh::Vertex));
    planetMesh.pIB = OIndexBuffer::createStatic(indices.data(), indices.size() * sizeof(uint16_t));
    planetMesh.indexCount = indices.size();
}

void createStars()
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

void createSolidRocket()
{
    Vertices vertices;
    Indices indices;
    createCylinder(vertices, indices, {0, 0}, .35f, 1.0f, ROCKET_COLOR);
    createEngine(vertices, indices, {0, 0}, .25f, 0.20f, ENGINE_COLOR);
    solidRocketMesh.pVB = OVertexBuffer::createStatic(vertices.data(), vertices.size() * sizeof(Mesh::Vertex));
    solidRocketMesh.pIB = OIndexBuffer::createStatic(indices.data(), indices.size() * sizeof(uint16_t));
    solidRocketMesh.indexCount = indices.size();
}

void createCone()
{
    Vertices vertices;
    Indices indices;
    createCone(vertices, indices, {0, 0}, .35f, 0.4f, ROCKET_COLOR);
    coneMesh.pVB = OVertexBuffer::createStatic(vertices.data(), vertices.size() * sizeof(Mesh::Vertex));
    coneMesh.pIB = OIndexBuffer::createStatic(indices.data(), indices.size() * sizeof(uint16_t));
    coneMesh.indexCount = indices.size();
}

void createMeshes()
{
    createAtmospheres();
    createPlanet();
    createStars();
    createSolidRocket();
    createCone();
}

extern OTextureRef pWhiteTexture;

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
