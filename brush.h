#ifndef BRUSH_H
#define BRUSH_H

#include <vector>
#include <glm/glm.hpp>
#include <stdint.h>

using namespace glm;
using std::vector;

typedef int32_t int32;
const int32 nill = -1;

struct VertexNode
{
    int32 down[4];
};

struct Plane
{
    vec3 normal;
    float distance;
};

struct Link
{
    int32 data;
    int32 lNext;
};

struct Face
{
    int32 lEdges;
    int32 lSubfaces;
    int32 plane;
};

#define SUBFACE_EMPTY 1
#define SUBFACE_SOLID 2

struct Subface
{
    int32 lEdges;
    int32 children[2];
    int32 contents;
};

struct Brush
{
    int32 lFaces;
    int32 plane;
    int32 children[2];
    int32 contents;
};

struct Environment
{
    vector<VertexNode> vertexTree;
    vector<vec3> vertices;
    vector<Plane> planes;
    vector<Link> links;
    vector<Face> faces;
    vector<Subface> subfaces;
    vector<Brush> brushes;

    int32 new_vertexNode();
    int32 new_vertex();
    int32 new_plane();
    int32 new_link();
    int32 new_face();
    int32 new_subface();
    int32 new_brush();

    void clear();
    int32 init_box(vec3 min, vec3 max);
    int32 add_vertex(vec3 vertex);
    int32 add_vertex(int32 vertex_a, float dist_a, int32 vertex_b, float dist_b);
    void add_link(int32 *link, int32 data);

    bool check_face(int32 lastLink, int32 plane, bool *side);
    void cut_face(int32 lastLink, int32 newLinks[2], int32 newVertices[2], int32 plane);
    void cut_subface(int32 *link[2], int32 subface, int32 plane);
    void cut_brush(int32 brush, int32 plane);

    bool save(const char *filename);
    bool load(const char *filename);
};

#endif