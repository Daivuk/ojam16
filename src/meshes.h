#pragma once
#include <onut/VertexBuffer.h>
#include <onut/IndexBuffer.h>
#include "defines.h"

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

extern Mesh starMesh;
extern Mesh atmosphereMesh;
extern Mesh planetMesh;
extern Mesh launchStationMesh;
extern Mesh cloudMesh;

void createMeshes();
