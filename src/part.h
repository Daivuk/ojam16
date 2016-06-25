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
    float liquidFuel = 0;
    float solidFuel = 0;
    float stability = 0;
    float trust = 0;
    bool isStaged = false;
    std::vector<Vector2> attachPoints;
    int price;
    std::string name;
};

struct Part
{
    Vector2 position;
    Vector2 vel;
    float angle = 0;
    float angleVelocity = 0;
    int type;
    float liquidFuel = 0;
    float solidFuel = 0;
    Parts children;
    bool fixed = false;
    bool isActive = false;
    std::set<int> usedAttachPoints;
    Part* pParent = nullptr;
};

#define PART_TOP_CONE 0
#define PART_SOLID_ROCKET 1
#define PART_DECOUPLER 2
#define PART_CONE 3
#define PART_COUNT 4

extern PartDef partDefs[PART_COUNT];
extern Parts parts;
extern Part* pMainPart;
extern std::vector<std::vector<Part*>> stages;

void deleteParts(Parts& parts);
void initPartDefs();
void drawParts(const Matrix& parentTransform, Parts& parts, Part* pParent = nullptr);
void drawAnchors(const Matrix& parentTransform, Parts& parts);
void drawOnTops();
Rect vehiculeRect(Part* pPart, const Vector2& parentPos = Vector2::Zero);
Part* mouseHoverPart(Part* pPart, const Vector2& mousePos, const Vector2& parentPos = Vector2::Zero);
void updatePart(Part* pPart);
Matrix getWorldTransform(Part* pPart);
