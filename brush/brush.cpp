#include "brush.hpp"

namespace Brush
{
using std::vector;


BspData::~BspData()
{
    if(data) delete[] data;
}

bool BspData::readBsp(const char *filename)
{  
    FILE *fp = fopen(filename, "rb");
    if(!fp) return false;

    fseek(fp, 0, SEEK_END);
    size = ftell(fp);
    fseek(fp, 0, SEEK_SET);
    data = new char[size];
    int k = fread(data, size, 1, fp);
    fclose(fp);

    header =    (HEADER*)data;
    planes =    (PLANE*)&data[header->lump[LUMP_PLANES].pos];
    models =    (MODEL*)&data[header->lump[LUMP_MODELS].pos];
    clipnodes = (CLIPNODE*)&data[header->lump[LUMP_CLIPNODES].pos];

    num_planes = header->lump[LUMP_PLANES].size / sizeof(PLANE);
    num_models = header->lump[LUMP_MODELS].size / sizeof(MODEL);
    num_clipnodes = header->lump[LUMP_CLIPNODES].size / sizeof(CLIPNODE);

    if(header->version != 30) return false;

    return true;
}

float distance(Plane &plane, vec3 &vertex)
{
    vec3 v = vertex;
    vec3 n = plane.normal;
    float d = plane.distance;

    return v.x*n.x + v.y*n.y + v.z*n.z - d;
}

vec3 *intersect(Environment &env, Plane &plane, vec3 *a, vec3 *b)
{
    if(a > b)
    {
        vec3 *c = a;
        a = b;
        b = c;
    }

    float da = distance(plane, *a);
    float db = distance(plane, *b);

    vec3 p = 
    {
        (da*b->x - db*a->x)/(da - db),
        (da*b->y - db*a->y)/(da - db),
        (da*b->z - db*a->z)/(da - db),
    };

    vec3 n = plane.normal;
    float d = plane.distance;

    float dp = p.x*n.x + p.y*n.y + p.z*n.z - d;

    p = 
    {
        p.x - dp*n.x,
        p.y - dp*n.y,
        p.z - dp*n.z,
    };

    return env.addVertex(p);
}

bool Face::check(Plane &plane, int &side)
{
    for(Link<vec3*>::Iterator it = edges; !it.end(); ++it)
    {
        float d1 = distance(plane, *it.link->data);
        float d2 = distance(plane, *it.link->next->data);

        side = d1 < 0;

        if((d1 < 0) != (d2 < 0)) return true;
    }

    return false;
}

void Face::cut(Environment &env, Plane *plane, int side, Link<vec3*> *newEdges[2], int skip)
{
    if(sub[side].plane) throw "Side already cut";
    sub[side].plane = plane;

    Face *subFaces[2]{};

    for(int i = 0; i < 2; i++)
    {
        if(i == skip) continue;
        subFaces[i] = new Face;
        subFaces[i]->plane = this->plane;
        env.faces.push_back(subFaces[i]);
        sub[side].faces[i] = subFaces[i];
    }

    for(Link<vec3*>::Iterator it = edges; !it.end(); ++it)
    {
        vec3 *v1 = it.link->data;
        vec3 *v2 = it.link->next->data;

        float d1 = distance(*plane, *v1);
        float d2 = distance(*plane, *v2);

        int b1 = d1 < 0;
        int b2 = d2 < 0;

        Link<vec3*> *&edges1 = sub[side].faces[b1]->edges;
        Link<vec3*> *&edges2 = sub[side].faces[b2]->edges;

        if(b1 != skip) env.insertLink(edges1, v1);

        if(b1 != b2)
        {
            vec3 *p = intersect(env, *plane, v1, v2);
            if(b1 != skip)
            {
                env.insertLink(edges1, p);
                newEdges[b1] = edges1;
            }
            if(b2 != skip) env.insertLink(edges2, p);
        }
    }

    Plane *redPlane = sub[side^1].plane;

    if(!redPlane) return;

    int s[2];
    bool c[2];
    for(int i = 0; i < 2; i++) c[i] = sub[side^1].faces[i]->check(*plane, s[i]);

    if(c[0] && c[1])
    {
        Link<vec3*> *newEdges[2]{};

        for(int i = 0; i < 2; i++)
            sub[side^1].faces[i]->cut(env, plane, side, newEdges);
        
        for(int i = 0; i < 2; i++)
        {
            sub[side].faces[i]->sub[side^1].plane = redPlane;
            for(int j = 0; j < 2; j++)
                sub[side].faces[i]->sub[side^1].faces[j] = sub[side^1].faces[j]->sub[side].faces[i];
        }
    }
    else if(c[0] || c[1])
    {
        for(int i = 0; i < 2; i++)
        {
            if(!c[i]) continue;

            Link<vec3*> *newEdges[2]{};
            sub[side^1].faces[i]->sub[side].faces[s[i^1]^1] = sub[side].faces[s[i^1]^1];
            sub[side^1].faces[i]->cut(env, plane, side, newEdges, s[i^1]^1);
            sub[side].faces[s[i^1]]->sub[side^1].plane = redPlane;
            sub[side].faces[s[i^1]]->sub[side^1].faces[i] = sub[side^1].faces[i]->sub[side].faces[s[i^1]];
            sub[side].faces[s[i^1]]->sub[side^1].faces[i^1] = sub[side^1].faces[i^1];
        }
    }
    else
    {
        sub[side].plane = redPlane;
        for(int i = 0; i < 2; i++)
            sub[side].faces[i] = sub[side^1].faces[i];
    }
}

void Brush::cut(Environment &env, Plane *plane)
{
    if(!faces) throw "Empty brush";
    if(sub.plane) throw "Brush already cut";
    sub.plane = plane;

    for(int i = 0; i < 2; i++)
    {
        sub.brushes[i] = new Brush;
        env.brushes.push_back(sub.brushes[i]);
    }

    struct Pair { vec3 *a{}, *b{}; };

    vector<Pair> pairs;

    for(Link<SignedFace>::Iterator it = faces; !it.end(); ++it)
    {
        int &faceSide = it.link->data.side;
        Face *&face = it.link->data.face;

        int side;

        if(face->check(*plane, side))
        {
            Link<vec3*> *newEdges[2]{};
            face->cut(env, plane, faceSide, newEdges);
            for(int i = 0; i < 2; i++)
                env.insertLink(sub.brushes[i]->faces, {faceSide, face->sub[faceSide].faces[i]});
            Link<vec3*> *l = newEdges[faceSide^1];
            pairs.push_back({l->data, l->next->data});
        }
        else env.insertLink(sub.brushes[side]->faces, {faceSide, face});
    }

    Link<vec3*> *edges{};

    for(size_t i = 0; i < pairs.size(); i++)
    {
        for(size_t j = i+1; j < pairs.size(); j++)
        {
            if(pairs[i].b == pairs[j].a)
            {
                Pair tmp = pairs[j];
                pairs[j] = pairs[i+1];
                pairs[i+1] = tmp;
                break;
            }
        }
        env.insertLink(edges, pairs[i].a);
    }

    if(edges)
    {
        Face *face = new Face;
        env.faces.push_back(face);
        face->plane = plane;
        face->edges = edges;
        for(int i = 0; i < 2; i++)
            env.insertLink(sub.brushes[i]->faces, {i, face});
    }
}

Brush *Brush::initBox(Environment &env, vec3 min, vec3 max)
{
    Brush *brush = new Brush;
    env.brushes.push_back(brush);

    vec3 *v[8] = 
    {
        env.addVertex({min.x, min.y, min.z}),
        env.addVertex({min.x, min.y, max.z}),
        env.addVertex({min.x, max.y, min.z}),
        env.addVertex({min.x, max.y, max.z}),
        env.addVertex({max.x, min.y, min.z}),
        env.addVertex({max.x, min.y, max.z}),
        env.addVertex({max.x, max.y, min.z}),
        env.addVertex({max.x, max.y, max.z})
    };

    Face *face{};

    face = new Face;
    env.faces.push_back(face);
    env.insertLink(face->edges, v[2]);
    env.insertLink(face->edges, v[3]);
    env.insertLink(face->edges, v[1]);
    env.insertLink(face->edges, v[0]);
    env.insertLink(brush->faces, {0, face});

    face = new Face;
    env.faces.push_back(face);
    env.insertLink(face->edges, v[4]);
    env.insertLink(face->edges, v[5]);
    env.insertLink(face->edges, v[7]);
    env.insertLink(face->edges, v[6]);
    env.insertLink(brush->faces, {0, face});

    face = new Face;
    env.faces.push_back(face);
    env.insertLink(face->edges, v[0]);
    env.insertLink(face->edges, v[1]);
    env.insertLink(face->edges, v[5]);
    env.insertLink(face->edges, v[4]);
    env.insertLink(brush->faces, {0, face});

    face = new Face;
    env.faces.push_back(face);
    env.insertLink(face->edges, v[6]);
    env.insertLink(face->edges, v[7]);
    env.insertLink(face->edges, v[3]);
    env.insertLink(face->edges, v[2]);
    env.insertLink(brush->faces, {0, face});

    face = new Face;
    env.faces.push_back(face);
    env.insertLink(face->edges, v[4]);
    env.insertLink(face->edges, v[6]);
    env.insertLink(face->edges, v[2]);
    env.insertLink(face->edges, v[0]);
    env.insertLink(brush->faces, {0, face});

    face = new Face;
    env.faces.push_back(face);
    env.insertLink(face->edges, v[3]);
    env.insertLink(face->edges, v[7]);
    env.insertLink(face->edges, v[5]);
    env.insertLink(face->edges, v[1]);
    env.insertLink(brush->faces, {0, face});

    return brush;
}

vec3 *Environment::addVertex(vec3 vertex)
{
    Node *node = &root;

    for(int32_t i = 0; i < 3; i++)
    for(int32_t j = 0; j < 16; j++)
    {
        int32_t data0 = ((int32_t*)&vertex)[i];
        int32_t data1 = (data0 >> (2*j)) & 0b11;

        if(i == 2 && j == 15)
        {
            vec3 *value = reinterpret_cast<vec3*>(node->down[data1]);
            if(!value)
            {
                value = new vec3(vertex);
                vertices.push_back(value);
                node->down[data1] = reinterpret_cast<Node*>(value);
            }
            return value;
        }

        Node *next = node->down[data1];
        if(!next)
        {
            next = new Node{};
            nodes.push_back(next);
            node->down[data1] = next;
        }
        node = next;
    }

    return nullptr;
}

void Face::paint(Brush *brush, int side)
{
    if(sub[side^1].plane)
    {
        for(int i = 0; i < 2; i++)
            sub[side^1].faces[i]->paint(brush, side);
        return;
    }
    sub[side].brush = brush;
}

void Brush::paint()
{
    if(sub.plane)
    {
        for(int i = 0; i < 2; i++)
            sub.brushes[i]->paint();
        return;
    }

    for(Link<SignedFace>::Iterator it = faces; !it.end(); ++it)
        it.link->data.face->paint(this, it.link->data.side);
}

void Environment::insertLink(Link<vec3*> *&link, vec3 *data)
{
    if(link && link->data == data) return;
    Link<vec3*>::insertLink(link, data);
    edges.push_back(link);
}

void Environment::insertLink(Link<SignedFace> *&link, SignedFace data)
{
    Link<SignedFace>::insertLink(link, data);
    signedFaces.push_back(link);
}

template<class T>
void Environment::remove(vector<T*> &vec)
{
    for(auto it = vec.begin(); it != vec.end(); ++it)
        delete *it;
}

Environment::~Environment()
{
    remove(vertices);
    remove(edges);
    remove(signedFaces);
    remove(faces);
    remove(brushes);
    remove(nodes);
}

void processBsp2(BspData &bspData, Environment &env, Brush *brush, int32_t clipnode)
{
    if(bspData.clipnodes[clipnode].plane >= bspData.num_planes)
    {
        //throw "Invalid plane index";
        return;
    }

    brush->cut(env, &(env.planes[bspData.clipnodes[clipnode].plane]));

    for(int32_t i = 0; i < 2; i++)
    {
        Brush *child = brush->sub.brushes[i];
        if(!child->faces) continue;

        int32_t next = bspData.clipnodes[clipnode].next[i];
        if(next >= 0)
            processBsp2(bspData, env, child, next);
        else
        {
            child->contents = next;
        }
    }
}

void processBsp(Environment &env, BspData &bspData)
{
    for(int i = 0; i < bspData.num_planes; i++)
        env.planes.push_back({bspData.planes[i].normal, bspData.planes[i].dist});

    for(int32_t m = 0; m < bspData.num_models*0+1; m++)
    {
        for(int h = 0; h < 4; h++)
        {
            env.hulls[h] = Brush::initBox(env, bspData.models[m].min, bspData.models[m].max);
            processBsp2(bspData, env, env.hulls[h], bspData.models[m].hull[h]);
            env.hulls[h]->paint();
        }
    }
}

}