#include "meshes.h"
#include <onut/Random.h>
#include <onut/Renderer.h>

Mesh starMesh;
Mesh atmosphereMesh;
Mesh planetMesh;
Mesh launchStationMesh;
Mesh cloudMesh;

void createMeshes()
{
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
