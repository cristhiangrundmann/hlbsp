#include "brush.h"
#include "hlbsp.h"
#include <stdio.h>
#include <time.h>

float calc_distance(vec3 point, Plane plane)
{
    return dot(point, plane.normal) - plane.distance;
}

int32 Environment::new_vertexNode()
{
    vertexNodes.push_back({});
    return vertexNodes.size()-1;
}

int32 Environment::new_vertex()
{
    vertices.push_back({});
    return vertices.size()-1;
}

int32 Environment::new_plane()
{
    planes.push_back({});
    return planes.size()-1;
}

int32 Environment::new_link()
{
    links.push_back({});
    return links.size()-1;
}

int32 Environment::new_face()
{
    faces.push_back({});
    return faces.size()-1;
}

int32 Environment::new_subface()
{
    subfaces.push_back({});
    return subfaces.size()-1;
}

int32 Environment::new_brush()
{
    brushes.push_back({});
    return brushes.size()-1;
}

void Environment::clear()
{
    vertexNodes.clear();
    vertexNodes.push_back({{nill, nill, nill, nill}});
    vertices.clear();
    planes.clear();
    links.clear();
    faces.clear();
    subfaces.clear();
    brushes.clear();
}

int32 Environment::init_box(vec3 min, vec3 max)
{
    int32 p = planes.size();
    int32 l = links.size();
    int32 f = faces.size();
    int32 s = subfaces.size();
    int32 b = brushes.size();

    int32 v[8];

    v[0] = add_vertex({min.x, min.y, min.z});
    v[1] = add_vertex({min.x, min.y, max.z});
    v[2] = add_vertex({min.x, max.y, min.z});
    v[3] = add_vertex({min.x, max.y, max.z});
    v[4] = add_vertex({max.x, min.y, min.z});
    v[5] = add_vertex({max.x, min.y, max.z});
    v[6] = add_vertex({max.x, max.y, min.z});
    v[7] = add_vertex({max.x, max.y, max.z});

    planes.push_back({{+1, 0, 0}, +min.x});
    planes.push_back({{-1, 0, 0}, -max.x});
    planes.push_back({{0, +1, 0}, +min.y});
    planes.push_back({{0, -1, 0}, -max.y});
    planes.push_back({{0, 0, +1}, +min.z});
    planes.push_back({{0, 0, -1}, -max.z});
    
    //face 0
    links.push_back({v[0], l+1});
    links.push_back({v[2], l+2});
    links.push_back({v[3], l+3});
    links.push_back({v[1], l+0});

    //face 1
    links.push_back({v[4], l+5});
    links.push_back({v[5], l+6});
    links.push_back({v[7], l+7});
    links.push_back({v[6], l+4});

    //face 2
    links.push_back({v[0], l+ 9});
    links.push_back({v[1], l+10});
    links.push_back({v[5], l+11});
    links.push_back({v[4], l+ 8});

    //face 3
    links.push_back({v[2], l+13});
    links.push_back({v[6], l+14});
    links.push_back({v[7], l+15});
    links.push_back({v[3], l+12});

    //face 4
    links.push_back({v[0], l+17});
    links.push_back({v[4], l+18});
    links.push_back({v[6], l+19});
    links.push_back({v[2], l+16});

    //face 5
    links.push_back({v[1], l+21});
    links.push_back({v[3], l+22});
    links.push_back({v[7], l+23});
    links.push_back({v[5], l+20});

    //subfaces
    links.push_back({s+0, l+24});
    links.push_back({s+1, l+25});
    links.push_back({s+2, l+26});
    links.push_back({s+3, l+27});
    links.push_back({s+4, l+28});
    links.push_back({s+5, l+29});

    //faces
    links.push_back({f+0, l+31});
    links.push_back({f+1, l+32});
    links.push_back({f+2, l+33});
    links.push_back({f+3, l+34});
    links.push_back({f+4, l+35});
    links.push_back({f+5, l+30});
    
    faces.push_back({l+0,  l+24, p+0});
    faces.push_back({l+4,  l+25, p+1});
    faces.push_back({l+8,  l+26, p+2});
    faces.push_back({l+12, l+27, p+3});
    faces.push_back({l+16, l+28, p+4});
    faces.push_back({l+20, l+29, p+5});

    int32 m = SUBFACE_SOLID | SUBFACE_EMPTY;

    subfaces.push_back({l+ 0, {nill, nill}, m});
    subfaces.push_back({l+ 4, {nill, nill}, m});
    subfaces.push_back({l+ 8, {nill, nill}, m});
    subfaces.push_back({l+12, {nill, nill}, m});
    subfaces.push_back({l+16, {nill, nill}, m});
    subfaces.push_back({l+20, {nill, nill}, m});

    brushes.push_back({l+30, nill, {nill, nill}, 0});
    
    return b;
}

int32 Environment::add_vertex(vec3 vertex)
{
    //logarithmic time - fast
    int32 node = 0;
    for(int32 i = 0; i < 3; i++)
    for(int32 j = 0; j < 16; j++)
    {
        int32 data0 = ((int32*)&vertex)[i];
        int32 data1 = (data0 >> (2*j)) & 0x3;

        if(i == 2 && j == 15)
        {
            int32 value = vertexNodes[node].down[data1];
            if(value == nill)
            {
                value = new_vertex();
                vertices[value] = vertex;
                vertexNodes[node].down[data1] = value;
            }
            return value;
        }

        int32 next = vertexNodes[node].down[data1];
        if(next == nill)
        {
            next = new_vertexNode();
            vertexNodes[node].down[data1] = next;
            vertexNodes[next] = {nill, nill, nill, nill};
        }
        node = next;
    }
    
    /*
    //linear time - slow
    for(int32 i = 0; i < vertices.size(); i++)
        if(vertices[i] == vertex)
            return i;

    int32 n = new_vertex();
    vertices[n] = vertex;
    return n;
    */
}

int32 Environment::add_vertex(int32 vertex_a, float dist_a, int32 vertex_b, float dist_b)
{
    if(vertex_a > vertex_b)
    {
        int32 vc = vertex_a;
        float dc = dist_a;
        vertex_a = vertex_b;
        dist_a = dist_b;
        vertex_b = vc;
        dist_b = dc;
    }

    vec3 p = (dist_a * vertices[vertex_b] - dist_b * vertices[vertex_a])/(dist_a - dist_b);

    return add_vertex(p);
}

void Environment::add_link(int32 *link, int32 data)
{
    int32 n = new_link();

    if(*link != nill)
    {
        links[n].lNext = links[*link].lNext;
        links[*link].lNext = n;
    }
    else links[n].lNext = n;

    *link = n;
    links[n].data = data;
}

bool Environment::check_face(int32 lastLink, int32 plane, bool *side)
{
    int32 link = lastLink;
    
    do
    {
        int32 next = links[link].lNext;

        vec3 a = vertices[links[link].data];
        vec3 b = vertices[links[next].data];

        float dist_a = calc_distance(a, planes[plane]);
        float dist_b = calc_distance(b, planes[plane]);

        *side = dist_a < 0;
        if((dist_a < 0) != (dist_b < 0)) return true;

        link = next;
    } while(link != lastLink);

    return false;
}

void Environment::cut_face(int32 lastLink, int32 newLinks[2], int32 newVertices[2], int32 plane)
{
    int32 link = lastLink;

    do
    {
        int32 next = links[link].lNext;

        vec3 a = vertices[links[link].data];
        vec3 b = vertices[links[next].data];

        float dist_a = calc_distance(a, planes[plane]);
        float dist_b = calc_distance(b, planes[plane]);

        add_link(&newLinks[dist_a < 0], links[link].data);

        if((dist_a < 0) != (dist_b < 0))
        {
            int32 c = add_vertex(links[link].data, dist_a, links[next].data, dist_b);
            add_link(&newLinks[dist_a < 0], c);
            add_link(&newLinks[dist_b < 0], c);
            newVertices[dist_a < 0] = c;
        }

        link = next;
    } while(link != lastLink);
}

void Environment::cut_subface(int32 *link[2], int32 subface, int32 plane)
{
    bool side;
    bool cut = check_face(subfaces[subface].lEdges, plane, &side);

    if(cut)
    {
        if(subfaces[subface].children[0] == nill)
        {
            int32 newLinks[2] = {nill, nill};
            int32 newVertices[2];

            cut_face(subfaces[subface].lEdges, newLinks, newVertices, plane);

            for(int32 i = 0; i < 2; i++)
            {
                int32 s = new_subface();
                subfaces[subface].children[i] = s;
                subfaces[s] = {newLinks[i], {nill, nill}, subfaces[subface].contents};
                add_link(link[i], s);
            }
        }
        else
            for(int32 i = 0; i < 2; i++)
                cut_subface(link, subfaces[subface].children[i], plane);
    }
    else
        add_link(link[side], subface);
}

void Environment::cut_brush(int32 brush, int32 plane)
{
    brushes[brush].plane = plane;
    int32 newFaces[2] = {nill, nill};

    for(int32 i = 0; i < 2; i++)
    {
        int32 child = new_brush();
        brushes[brush].children[i] = child;
        brushes[child] = {nill, nill, {nill, nill}, 0};
    }

    struct Pair { int32 data[2]; };
    vector<Pair> pairs;

    int32 lastLink = brushes[brush].lFaces;
    int32 link = lastLink;
    bool side;

    do
    {
        int32 ind = links[link].data;
        bool sign = ind < 0;

        int32 face = sign ? ~ind : ind;

        if(check_face(faces[face].lEdges, plane, &side))
        {
            int32 newLinks[2] = {nill, nill};
            int32 newVertices[2];

            cut_face(faces[face].lEdges, newLinks, newVertices, plane);

            int32 subLinks[2] = {nill, nill};

            int32 fs[2];
            for(int32 i = 0; i < 2; i++)
            {
                int32 f = new_face();
                fs[i] = f;
                faces[f] = {newLinks[i], nill, faces[face].plane};
                add_link(&newFaces[i], sign ? ~f : f);
            }

            if(newVertices[0] != newVertices[1])
                pairs.push_back({newVertices[1 ^ sign], newVertices[0 ^ sign]});

            int32 lastLink = faces[face].lSubfaces;
            int32 link = lastLink;

            if(link != nill)
            do
            {
                int32 *ptr[2] = {&subLinks[0], &subLinks[1]};
                cut_subface(ptr, links[link].data, plane);
                link = links[link].lNext;
            } while(link != lastLink);

            for(int32 i = 0; i < 2; i++)
                faces[fs[i]].lSubfaces = subLinks[i];

        }
        else
            add_link(&newFaces[side], ind);

        link = links[link].lNext;
    } while(link != lastLink);

    lastLink = nill;

    for(int32 i = 0; i < pairs.size(); i++)
    {
        for(int32 j = i+1; j < pairs.size(); j++)
        {
            if(pairs[i].data[1] == pairs[j].data[0])
            {
                Pair tmp = pairs[j];
                pairs[j] = pairs[i+1];
                pairs[i+1] = tmp;
            }
        }

        add_link(&lastLink, pairs[i].data[0]);
    }

    if(lastLink != nill)
    {
        int32 f = new_face();
        int32 s = new_subface();
        int32 k = nill;
        add_link(&k, s);
        faces[f] = {lastLink, k, plane};
        subfaces[s] = {lastLink, {nill, nill}, 0};

        add_link(&newFaces[0], f);
        add_link(&newFaces[1], ~f);
    }
    else brushes[brushes[brush].children[side^1]].contents = CONTENTS_DEGENERATE;

    for(int32 i = 0; i < 2; i++)
        brushes[brushes[brush].children[i]].lFaces = newFaces[i];
}


void Environment::visibility_simplify()
{
    for(int32 i = subfaces.size()-1; i >= 0; i--)
    {
        if(subfaces[i].children[0] == nill) continue;

        int32 c[2];
        for(int32 j = 0; j < 2; j++)
            c[j] = subfaces[subfaces[i].children[j]].contents;

        if(c[0] == c[1]) subfaces[i].contents = c[0];
    }
}

void Environment::visibility_rec(int32 subface, int32 contents)
{
    if(subfaces[subface].children[0] == nill)
        subfaces[subface].contents |= contents;
    else
        for(int32 i = 0; i < 2; i++)
            visibility_rec(subfaces[subface].children[i], contents);
}

void Environment::visibility(int32 brush)
{
    int32 contents = brushes[brush].contents;
    if(contents == CONTENTS_DEGENERATE) return;

    if(contents == 0)
    {
        for(int32 i = 0; i < 2; i++)
            visibility(brushes[brush].children[i]);
        return;
    }

    int32 m = 0;
    if(contents == CONTENTS_SOLID) m = SUBFACE_SOLID;
    else m = SUBFACE_EMPTY;

    int32 lastF = brushes[brush].lFaces;
    int32 f = lastF;

    do
    {
        int32 ind = links[f].data;
        bool sign = ind < 0;
        int32 face = sign ? ~ind : ind;

        int32 lastS = faces[face].lSubfaces;
        int32 s = lastS;

        if(s != nill)
        do
        {
            int32 subface = links[s].data;
            visibility_rec(subface, m);
            s = links[s].lNext;
        } while(s != lastS);

        f = links[f].lNext;
    } while(f != lastF);
}

void BspExport::proc_bsp(BspData &bspData, int32 brush, int32 clipnode)
{
    //??? c2a2a & c3a2c seem to have out of bounds plane indices ???
    if(bspData.clipnodes[clipnode].plane >= bspData.num_planes)
    {
        env.brushes[brush].contents = CONTENTS_DEGENERATE;
        return;
    }

    env.cut_brush(brush, bspData.clipnodes[clipnode].plane);

    for(int32 i = 0; i < 2; i++)
    {
        int32 child = env.brushes[brush].children[i];

        if(env.brushes[child].contents == CONTENTS_DEGENERATE) continue;
        
        int32 next = bspData.clipnodes[clipnode].next[i];
        if(next >= 0)
            proc_bsp(bspData, child, next);
        else
            env.brushes[child].contents = next;
    }
}

bool BspExport::read_bsp(const char *filename)
{   
    printf("Processing map: %s\n", filename);
    clock_t time0 = clock();

    BspData bspData;

    FILE *fp = fopen(filename, "rb");
    if(!fp) return false;

    fseek(fp, 0, SEEK_END);
    bspData.bsp_size = ftell(fp);
    fseek(fp, 0, SEEK_SET);
    bspData.bsp_data = new char[bspData.bsp_size];
    int k = fread(bspData.bsp_data, bspData.bsp_size, 1, fp);
    fclose(fp);

    bspData.header =    (HEADER*)bspData.bsp_data;
    bspData.planes =    (PLANE*)&bspData.bsp_data[bspData.header->lump[LUMP_PLANES].pos];
    bspData.models =    (MODEL*)&bspData.bsp_data[bspData.header->lump[LUMP_MODELS].pos];
    bspData.clipnodes = (CLIPNODE*)&bspData.bsp_data[bspData.header->lump[LUMP_CLIPNODES].pos];

    bspData.num_planes = bspData.header->lump[LUMP_PLANES].size / sizeof(PLANE);
    bspData.num_models = bspData.header->lump[LUMP_MODELS].size / sizeof(MODEL);
    bspData.num_clipnodes = bspData.header->lump[LUMP_CLIPNODES].size / sizeof(CLIPNODE);

    if(bspData.header->version != 30) return false;

    env.clear();
    models.clear();

    for(int i = 0; i < bspData.num_planes; i++)
        env.planes.push_back({bspData.planes[i].normal, bspData.planes[i].dist});

    for(int32 m = 0; m < bspData.num_models; m++)
    {
        float thickness = 16;
        vec3 thick = {thickness, thickness, thickness};
        vec3 min = bspData.models[m].min - thick;
        vec3 max = bspData.models[m].max + thick;

        int32 b;
        b = env.init_box(min, max);
        env.init_box(min, max);
        env.init_box(min, max);
        env.init_box(min, max);

        for(int i = 0; i < 4; i++)
        {
            proc_bsp(bspData, b+i, bspData.models[m].hull[i]);
            env.visibility(b+i);
        }

        models.push_back({b, b+1, b+2, b+3});
    }

    env.visibility_simplify();

    clock_t time1 = clock();
    float time_diff = (float)(time1 - time0) / CLOCKS_PER_SEC;

    printf("Time: %fs\n", time_diff);
    delete[] bspData.bsp_data;
    return true;
}

bool BspExport::save_col(const char *filename)
{
    printf("Saving collision: %s\n", filename);
    FILE *fp = fopen(filename, "wb");
    if(!fp) return false;

    ColHeader header;
    header.version = 30; //same as half life's bsp's

    int32 cur_pos = sizeof(header);
    int32 cur_size = sizeof(VertexNode)*env.vertexNodes.size();
    header.lumps[COLLUMP_VERTEXNODES] = {cur_pos, cur_size};

    cur_pos += cur_size;
    cur_size = sizeof(vec3)*env.vertices.size();
    header.lumps[COLLUMP_VERTICES] = {cur_pos, cur_size};

    cur_pos += cur_size;
    cur_size = sizeof(Plane)*env.planes.size();
    header.lumps[COLLUMP_PLANES] = {cur_pos, cur_size};

    cur_pos += cur_size;
    cur_size = sizeof(Link)*env.links.size();
    header.lumps[COLLUMP_LINKS] = {cur_pos, cur_size};

    cur_pos += cur_size;
    cur_size = sizeof(Face)*env.faces.size();
    header.lumps[COLLUMP_FACES] = {cur_pos, cur_size};

    cur_pos += cur_size;
    cur_size = sizeof(Subface)*env.subfaces.size();
    header.lumps[COLLUMP_SUBFACES] = {cur_pos, cur_size};

    cur_pos += cur_size;
    cur_size = sizeof(Brush)*env.brushes.size();
    header.lumps[COLLUMP_BRUSHES] = {cur_pos, cur_size};

    cur_pos += cur_size;
    cur_size = sizeof(ColModel)*models.size();
    header.lumps[COLLUMP_MODELS] = {cur_pos, cur_size};

    if(1 !=  fwrite(&header, sizeof(ColHeader), 1, fp)) return false;
    if(1 !=  fwrite(env.vertexNodes.data(), sizeof(VertexNode)*env.vertexNodes.size(), 1, fp)) return false;
    if(1 !=  fwrite(env.vertices.data(), sizeof(vec3)*env.vertices.size(), 1, fp)) return false;
    if(1 !=  fwrite(env.planes.data(), sizeof(Plane)*env.planes.size(), 1, fp)) return false;
    if(1 !=  fwrite(env.links.data(), sizeof(Link)*env.links.size(), 1, fp)) return false;
    if(1 !=  fwrite(env.faces.data(), sizeof(Face)*env.faces.size(), 1, fp)) return false;
    if(1 !=  fwrite(env.subfaces.data(), sizeof(Subface)*env.subfaces.size(), 1, fp)) return false;
    if(1 !=  fwrite(env.brushes.data(), sizeof(Brush)*env.brushes.size(), 1, fp)) return false;
    if(1 !=  fwrite(models.data(), sizeof(ColModel)*models.size(), 1, fp)) return false;

    fclose(fp);
    return true;
}

bool BspExport::load_col(const char *filename)
{
    printf("Loading collision: %s\n", filename);
    FILE *fp = fopen(filename, "rb");
    if(!fp) return false;

    fseek(fp, 0, SEEK_END);
    int32 size = ftell(fp);
    fseek(fp, 0, SEEK_SET);

    char *data = new char[size];

    if(1 != fread(data, size, 1, fp)) return false;

    fclose(fp);

    ColHeader *header = (ColHeader*)data;

    if(header->version != 30) return false;

    env.clear();
    models.clear();

    int32 cur_pos = header->lumps[COLLUMP_VERTEXNODES].pos;
    int32 cur_size = header->lumps[COLLUMP_VERTEXNODES].size;
    env.vertexNodes = vector<VertexNode>((VertexNode*)&data[cur_pos], (VertexNode*)&data[cur_pos+cur_size]);

    cur_pos = header->lumps[COLLUMP_VERTICES].pos;
    cur_size = header->lumps[COLLUMP_VERTICES].size;
    env.vertices = vector<vec3>((vec3*)&data[cur_pos], (vec3*)&data[cur_pos+cur_size]);

    cur_pos = header->lumps[COLLUMP_PLANES].pos;
    cur_size = header->lumps[COLLUMP_PLANES].size;
    env.planes = vector<Plane>((Plane*)&data[cur_pos], (Plane*)&data[cur_pos+cur_size]);

    cur_pos = header->lumps[COLLUMP_LINKS].pos;
    cur_size = header->lumps[COLLUMP_LINKS].size;
    env.links = vector<Link>((Link*)&data[cur_pos], (Link*)&data[cur_pos+cur_size]);

    cur_pos = header->lumps[COLLUMP_FACES].pos;
    cur_size = header->lumps[COLLUMP_FACES].size;
    env.faces = vector<Face>((Face*)&data[cur_pos], (Face*)&data[cur_pos+cur_size]);

    cur_pos = header->lumps[COLLUMP_SUBFACES].pos;
    cur_size = header->lumps[COLLUMP_SUBFACES].size;
    env.subfaces = vector<Subface>((Subface*)&data[cur_pos], (Subface*)&data[cur_pos+cur_size]);

    cur_pos = header->lumps[COLLUMP_BRUSHES].pos;
    cur_size = header->lumps[COLLUMP_BRUSHES].size;
    env.brushes = vector<Brush>((Brush*)&data[cur_pos], (Brush*)&data[cur_pos+cur_size]);

    cur_pos = header->lumps[COLLUMP_MODELS].pos;
    cur_size = header->lumps[COLLUMP_MODELS].size;
    models = vector<ColModel>((ColModel*)&data[cur_pos], (ColModel*)&data[cur_pos+cur_size]);

    delete[] data;
    return true;   
}
