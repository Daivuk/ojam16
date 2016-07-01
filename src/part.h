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
    OTextureRef pEngineCoverTexture;
    Vector2 hsize;
    float weight;
    float liquidFuel = 0;
    float solidFuel = 0;
    float stability = 0;
    float trust = 0;
    float burn = 0;
    bool isStaged = false;
    std::vector<Vector2> attachPoints;
    std::vector<int> attachPointsDir;
    int price;
    std::string name;
    int type;
    int id;
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

#define PART_TYPE_PAYLOAD 0
#define PART_TYPE_BOOSTER 1
#define PART_TYPE_DECOUPLER 2
#define PART_TYPE_AERODYNAMIC 3
#define PART_TYPE_FUEL 4
#define PART_TYPE_SATELLITE 5
#define PART_TYPE_ENGINE 6

#define PART_ATTACH_DIR_UP 0
#define PART_ATTACH_DIR_DOWN 1
#define PART_ATTACH_DIR_LEFT 2
#define PART_ATTACH_DIR_RIGHT 3

extern std::vector<PartDef> partDefs;
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
