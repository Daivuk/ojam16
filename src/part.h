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
    float totalMass = 0;
    Vector2 centerOfMass;
    float speed = 0;
    float altitude = 0;
    int parentAttachPoint = -1;
    OSoundInstanceRef pSound;
};

#define PART_TOP_CONE 0
#define PART_SOLID_ROCKET 1
#define PART_DECOUPLER 2
#define PART_CONE 3
#define FIN_SMALL_LEFT 4
#define FIN_SMALL_RIGHT 5
#define PART_DECOUPLER_HORIZONTAL_LEFT 6
#define PART_DECOUPLER_HORIZONTAL_RIGHT 7
#define PART_LARGE_TO_SMALL_JOINER 8
#define PART_FUEL_WIDE_TALL 9
#define PART_FUEL_WIDE_SHORT 10
#define PART_LIQUID_ROCKET_WIDE 11
#define PART_FUEL_THIN_SHORT 12
#define PART_FUEL_THIN_TALL 13
#define PART_LIQUID_ROCKET_THIN 14
#define PART_SMALL_TO_LARGE_JOINER 15
#define PART_DECOUPLER_WIDE 16
#define FIN_MEDIUM_LEFT 17
#define FIN_MEDIUM_RIGHT 18
#define PART_SATELLITE 19
#define PART_COUNT 19

extern PartDef partDefs[PART_COUNT + 1];
extern Parts parts;
extern Part* pMainPart;
extern std::vector<std::vector<Part*>> stages;
extern float globalStability;
extern Parts toKill;

void deleteParts(Parts& parts);
void initPartDefs();
void drawParts(const Matrix& parentTransform, Parts& parts, Part* pParent = nullptr);
void drawAnchors(const Matrix& parentTransform, Parts& parts);
void drawOnTops();
Rect vehiculeRect(Part* pPart, const Vector2& parentPos = Vector2::Zero);
Part* mouseHoverPart(Part* pPart, const Vector2& mousePos, const Vector2& parentPos = Vector2::Zero);
void updatePart(Part* pPart);
Matrix getWorldTransform(Part* pPart);
Part* getTopParent(Part* pPart);
float getTotalMass(Part* pPart);
float getTotalStability(Part* pPart);
Part* getLiquidFuel(Part* pPart, float& totalLeft, float& maxLiquidFuel);
void deletePart(Part* in_pPart);
void explodePart(Part* pPart);
