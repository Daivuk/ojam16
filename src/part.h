#pragma once
#include <onut/Texture.h>
#include <vector>
#include <string>
#include <set>

struct Part;
using Parts = std::vector<Part*>;

struct PartDef
{
    OTextureRef pTexture;
    Vector2 hsize;
    float weight;
    std::vector<Vector2> attachPoints;
    int price;
    std::string name;
};

struct Part
{
    Vector2 position;
    float angle = 0;
    int type;
    Parts children;
    bool fixed = false;
    std::set<int> usedAttachPoints;
};

#define PART_TOP_CONE 0
#define PART_SOLID_ROCKET 1
#define PART_DECOUPLER 2
#define PART_CONE 3
#define PART_COUNT 4

extern PartDef partDefs[PART_COUNT];
extern Parts parts;
extern Part* pMainPart;

void deleteParts(Parts& parts);
void initPartDefs();
void drawParts(const Matrix& parentTransform, Parts& parts, Part* pParent = nullptr);
void drawAnchors(const Matrix& parentTransform, Parts& parts);
void drawOnTops();
Rect vehiculeRect(Part* pPart, const Vector2& parentPos = Vector2::Zero);
