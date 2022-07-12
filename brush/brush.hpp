#pragma once

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <vector>

namespace Brush {

#define LUMP_ENTITIES      0
#define LUMP_PLANES        1
#define LUMP_TEXTURES      2
#define LUMP_VERTICES      3
#define LUMP_VISIBILITY    4
#define LUMP_NODES         5
#define LUMP_TEXINFO       6
#define LUMP_FACES         7
#define LUMP_LIGHTING      8
#define LUMP_CLIPNODES     9
#define LUMP_LEAVES       10
#define LUMP_MARKSURFACES 11
#define LUMP_EDGES        12
#define LUMP_SURFEDGES    13
#define LUMP_MODELS       14
#define HEADER_LUMPS      15

#define CONTENTS_EMPTY        -1
#define CONTENTS_SOLID        -2
#define CONTENTS_WATER        -3
#define CONTENTS_SLIME        -4
#define CONTENTS_LAVA         -5
#define CONTENTS_SKY          -6
#define CONTENTS_ORIGIN       -7
#define CONTENTS_CLIP         -8
#define CONTENTS_CURRENT_0    -9
#define CONTENTS_CURRENT_90   -10
#define CONTENTS_CURRENT_180  -11
#define CONTENTS_CURRENT_270  -12
#define CONTENTS_CURRENT_UP   -13
#define CONTENTS_CURRENT_DOWN -14
#define CONTENTS_TRANSLUCENT  -15

#define MAX_MAP_HULLS 4

struct vec3
{
    float x, y, z;
};

struct LUMP
{
    int32_t pos;
    int32_t size;
};

struct HEADER
{
    int32_t version;
    LUMP lump[HEADER_LUMPS];
};

struct CLIPNODE
{
    int32_t plane;
    int16_t next[2];
};

struct MODEL
{
    vec3 min, max;
    vec3 org;
    int32_t hull[MAX_MAP_HULLS];
    int32_t visleafs;
    int32_t firstface, faces;
};

struct PLANE
{
    vec3 normal;
    float dist;
    int32_t type;
};


struct BspData
{
    size_t size = 0, num_planes = 0, num_models = 0, num_clipnodes = 0;
    char *data = nullptr;
    HEADER *header = nullptr;
    PLANE *planes = nullptr;
    MODEL *models = nullptr;
    CLIPNODE *clipnodes = nullptr;

    ~BspData();

    bool readBsp(const char *filename);
};

template<class T> struct Link;
struct Plane;
struct Face;
struct SignedFace;
struct Brush;

struct Environment
{
    struct Node
    {
         Node *down[4]{};
    };

    std::vector<vec3*> vertices;
    std::vector<Link<vec3*>*> edges;
    std::vector<Link<SignedFace>*> signedFaces;
    std::vector<Plane> planes;
    std::vector<Face*> faces;
    std::vector<Brush*> brushes;
    std::vector<Node*> nodes;
    Node root{};

    Brush *hulls[4]{};

    vec3 *addVertex(vec3 vertex);

    void insertLink(Link<vec3*> *&link, vec3 *data);

    void insertLink(Link<SignedFace> *&link, SignedFace data);

    template<class T>
    void remove(std::vector<T*> &vec);

    ~Environment();
};

template<class T>
struct Link
{
    Link<T> *next{};
    T data{};

    static void insertLink(Link<T> *&link, T data)
    {
        if(link)
        {
            Link<T> *next = new Link<T>;
            next->data = data;
            next->next = link->next;
            link->next = next;
            link = next;
        }
        else
        {
            link = new Link<T>;
            link->data = data;
            link->next = link;   
        }
    }

    struct Iterator
    {
    private:
        Link<T> *first{};
    public:
        Link<T> *link{};

        Iterator(Link<T> *link)
        {
            this->link = link;
        }

        bool end()
        {
            if(first == link) return true;
            return false;
        }

        void operator++()
        {
            if(!first) first = link;
            link = link->next;
        }

    };
};

struct Plane
{
    vec3 normal{};
    float distance{};
};

float distance(Plane &plane, vec3 &vertex);
vec3 *intersect(Environment &env, Plane &plane, vec3 *a, vec3 *b);

struct Face
{
    Link<vec3*> *edges{};
    Plane *plane{};

    struct SubFace
    {
        Plane *plane{};
        Face *faces[2]{};
        Brush *brush{};
    } sub[2];

    bool check(Plane &plane, int &side);
    void cut(Environment &env, Plane *plane, int side, Link<vec3*> *newEdges[2], int skip = -1);
    void paint(Brush *brush, int side);
};

struct SignedFace
{
    int side{};
    Face *face{};
};

struct Brush
{
    Link<SignedFace> *faces{};
    int contents{};

    struct SubBrush
    {
        Plane *plane{};
        Brush *brushes[2]{};
    } sub;

    void cut(Environment &env, Plane *plane);
    static Brush *initBox(Environment &env, vec3 min, vec3 max);
    void paint();
};

void processBsp(Environment &env, BspData &bspData);

}