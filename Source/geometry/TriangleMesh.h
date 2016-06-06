#ifndef GEOMETRY_TRIANGLE_MESH_HPP
#   define GEOMETRY_TRIANGLE_MESH_HPP

#include <assert.h>
#include <string.h>
#include <fstream>
#include <iomanip>
#include <valarray>
#include <limits>
#include <vector>
#include <set>
#include <unordered_map>

#include "Triangle.h"

#define USE_OPENMP
//! Mesh made by a group of triangles

class TriangleMesh
{
    public:
		friend class MeshObj;
		TriangleMesh() { }
        TriangleMesh(const TriangleMesh& m):totalArea_(m.totalArea_),
                vertices_(m.vertices_), normals_(m.normals_),
                triangles_(m.triangles_), vtxAreas_(m.vtxAreas_)
        { }

        struct NeighborRec
        {
            size_t   num;    // how many neighbor faces (3 at most)
            unsigned id[3];
        };

        void clear()
        {
            vertices_.clear();
            normals_.clear();
            triangles_.clear();
        }

        //! Return the ID of the added vertices
        int add_vertex(const D3DXVECTOR3& vtx)
        {
            vertices_.push_back(vtx);
            return vertices_.size() - 1;
        }

        //! Return the ID of added vertices
        int add_vertex_normal(const D3DXVECTOR3& vtx, const D3DXVECTOR3& nml)
        {
            vertices_.push_back(vtx);
            normals_.push_back(nml);
            return vertices_.size() - 1;
        }

        //! Return the current number of triangles
        int add_triangle(unsigned int v0, unsigned int v1, unsigned int v2)
        {
            assert(v0 != v1 && v0 != v2 && v1 != v2);
            assert(v0 < vertices_.size() &&
                   v1 < vertices_.size() &&
                   v2 < vertices_.size());
            triangles_.push_back(Tuple3ui(v0, v1, v2));
            return triangles_.size();
        }

        int num_vertices() const { return vertices_.size(); }
        int num_triangles() const { return triangles_.size(); }

        std::vector< D3DXVECTOR3 >& vertices() 
        {  return vertices_; }

        std::vector<Tuple3ui>& triangles()
        {  return triangles_; }

        const std::vector< D3DXVECTOR3 >& vertices() const
        {  return vertices_; }

        const std::vector<Tuple3ui>& surface_indices() const
        {  return triangles_; }

        const std::vector<Tuple3ui>& triangles() const
        {  return triangles_; }

        const std::vector< D3DXVECTOR3 >& normals() const
        {  return normals_; }

        const std::valarray<float>& vertex_areas() const
        {  return vtxAreas_; }

        const Tuple3ui& triangle_ids(unsigned tid) const
        {
            assert(tid < triangles_.size());
            return triangles_[tid];
        }

        const D3DXVECTOR3& vertex(unsigned vid) const
        {  
            assert(vid < vertices_.size());
            return vertices_[vid];
        }

        const D3DXVECTOR3* vertex_ptr(unsigned vid) const
        {  
            assert(vid < vertices_.size());
            return &(vertices_[vid]);
        }

        const D3DXVECTOR3& normal(unsigned vid) const
        {
            assert(vid < vertices_.size() && normals_.size() == vertices_.size());
            return normals_[vid];
        }

        const D3DXVECTOR3* normal_ptr(unsigned vid) const
        {
            assert(vid < vertices_.size() && normals_.size() == vertices_.size());
            return &(normals_[vid]);
        }

        bool has_normals() const 
        {
            return !normals_.empty() && vertices_.size() == normals_.size();
        }

        bool empty() const 
        {  return vertices_.empty(); }

        float total_area() const 
        {  return totalArea_; }
        float triangle_area(unsigned tglId) const
        {   
            return Triangle::area(
                    vertices_[triangles_[tglId].x],
                    vertices_[triangles_[tglId].y],
                    vertices_[triangles_[tglId].z]);
        }

        void bounding_box(D3DXVECTOR3& low, D3DXVECTOR3& up) const;

        void generate_normals();
        void generate_pseudo_normals();
        //! Save the current mesh to a file
        int save_mesh_txt(const char* file) const;
        int load_mesh_txt(const char* file);
        void update_vertex_areas();

        void translate_x(float dx);
        void translate_y(float dy);
        void translate_z(float dz);

        //! Get the triangle face neighbors; for each triangle, return
        //  a list of its neighbor triangles
        void get_face_neighborship(std::vector<NeighborRec>&) const;
        //! Get the table of neighbors of all vertices
        void get_vtx_neighborship(std::vector< std::set<int> >&) const;
        //! Get the adjacent triangles of each vertex
        void get_vtx_tgls(std::vector< std::set<int> >&) const;

    private:
        float                           totalArea_;
        std::vector< D3DXVECTOR3 >    vertices_;
        std::vector< D3DXVECTOR3 >   normals_;      // vertex normals
        std::vector<Tuple3ui>       triangles_;    // indices of triangle vertices
        std::valarray<float>            vtxAreas_;     // area of each triangles
};

// ---------------------------------------------------------------------------------

void TriangleMesh::translate_x(float dx)
{
#ifdef USE_OPENMP
    #pragma omp parallel for default(none) schedule(dynamic, 20000) shared(dx)
    for(int i = 0;i < (int)vertices_.size();++ i)
#else
    for(size_t i = 0;i < vertices_.size();++ i)
#endif
        vertices_[i].x += dx;
}


void TriangleMesh::translate_y(float dy)
{
#ifdef USE_OPENMP
    #pragma omp parallel for default(none) schedule(dynamic, 20000) shared(dy)
    for(int i = 0;i < (int)vertices_.size();++ i)
#else
    for(size_t i = 0;i < vertices_.size();++ i)
#endif
        vertices_[i].y += dy;
}


void TriangleMesh::translate_z(float dz)
{
#ifdef USE_OPENMP
    #pragma omp parallel for default(none) schedule(dynamic, 20000) shared(dz)
    for(int i = 0;i < (int)vertices_.size();++ i)
#else
    for(size_t i = 0;i < vertices_.size();++ i)
#endif
        vertices_[i].z += dz;
}


void TriangleMesh::get_face_neighborship(std::vector<NeighborRec>& neighbors) const
{
    std::unordered_map< int, std::unordered_map<int, int> > hash;
    int v0, v1;

    neighbors.resize(triangles_.size());
#ifdef USE_OPENMP
    #pragma omp parallel for default(none) schedule(dynamic, 20000) shared(neighbors)
    for(int i = 0;i < (int)triangles_.size();++ i)
#else
    for(size_t i = 0;i < triangles_.size();++ i)
#endif
        neighbors[i].num = 0;

    for(size_t i = 0;i < triangles_.size();++ i)
    {
        // edge 0
        v0 = triangles_[i].x;
        v1 = triangles_[i].y;
        if ( v0 > v1 ) std::swap(v0, v1);   // v0 < v1
        if ( hash.count(v0) && hash[v0].count(v1) )
        {
            int fid = hash[v0][v1];
            neighbors[i].id[neighbors[i].num ++] = fid;
            neighbors[fid].id[neighbors[fid].num ++] = i;
        }
        else
            hash[v0][v1] = i;

        // edge 1
        v0 = triangles_[i].y;
        v1 = triangles_[i].z;
        if ( v0 > v1 ) std::swap(v0, v1);
        if ( hash.count(v0) && hash[v0].count(v1) )
        {
            int fid = hash[v0][v1];
            neighbors[i].id[neighbors[i].num ++] = fid;
            neighbors[fid].id[neighbors[fid].num ++] = i;
        }
        else
            hash[v0][v1] = i;

        // edge 2
        v0 = triangles_[i].z;
        v1 = triangles_[i].x;
        if ( v0 > v1 ) std::swap(v0, v1);
        if ( hash.count(v0) && hash[v0].count(v1) )
        {
            int fid = hash[v0][v1];
            neighbors[i].id[neighbors[i].num ++] = fid;
            neighbors[fid].id[neighbors[fid].num ++] = i;
        }
        else
            hash[v0][v1] = i;
    }
}


void TriangleMesh::update_vertex_areas()
{
    const float s = (float)1 / (float)3;
    totalArea_ = 0;
    vtxAreas_.resize(vertices_.size(), (float)0);
    for(int i = 0;i < (int)triangles_.size();++ i)
    {
        float area = Triangle::area(
                vertices_[triangles_[i].x],
                vertices_[triangles_[i].y],
                vertices_[triangles_[i].z]); 
        totalArea_ += area;
        area *= s;

        for(int j = 0;j < 3;++ j)
            vtxAreas_[triangles_[i][j]] += area;
    }
}


void TriangleMesh::generate_pseudo_normals()
{
    std::vector< D3DXVECTOR3 > tglnmls(triangles_.size());
    for(size_t i = 0;i < triangles_.size();++ i)
    {
        tglnmls[i] = Triangle::normal(
                vertices_[triangles_[i].x],
                vertices_[triangles_[i].y],
                vertices_[triangles_[i].z]);
        if ( D3DXVec3LengthSq(&tglnmls[i]) < 1E-18 )
        {
            fprintf(stderr, "ERROR: triangle has zero area: %.18g\n",
                    D3DXVec3LengthSq(&tglnmls[i]));
            exit(1);
        }
		D3DXVec3Normalize(&tglnmls[i], &tglnmls[i]);
    }

    normals_.resize(vertices_.size());
    memset(&normals_[0], 0, sizeof(D3DXVECTOR3)*vertices_.size());
    for(size_t i = 0;i < triangles_.size();++ i)
    {
        const D3DXVECTOR3& nml = tglnmls[i];
        normals_[triangles_[i].x] += nml * Triangle::angle(
                vertices_[triangles_[i].z],
                vertices_[triangles_[i].x],
                vertices_[triangles_[i].y]);
        normals_[triangles_[i].y] += nml * Triangle::angle(
                vertices_[triangles_[i].x],
                vertices_[triangles_[i].y],
                vertices_[triangles_[i].z]);
        normals_[triangles_[i].z] += nml * Triangle::angle(
                vertices_[triangles_[i].y],
                vertices_[triangles_[i].z],
                vertices_[triangles_[i].x]);
    }

    for(size_t i = 0;i < vertices_.size();++ i)
		D3DXVec3Normalize(&normals_[i], &normals_[i]);
}


void TriangleMesh::generate_normals()
{
    vtxAreas_.resize(vertices_.size(), (float)0);
    normals_.resize(vertices_.size());
    memset(&normals_[0], 0, sizeof(D3DXVECTOR3)*vertices_.size());
    totalArea_ = 0;

    for(int i = 0;i < (int)triangles_.size();++ i)
    {
        D3DXVECTOR3& p0 = vertices_[triangles_[i].x];
        D3DXVECTOR3& p1 = vertices_[triangles_[i].y];
        D3DXVECTOR3& p2 = vertices_[triangles_[i].z];
		D3DXVECTOR3 p1p0 = p1 - p0;
		D3DXVECTOR3 p2p0 = p2 - p0;
        D3DXVECTOR3 nml;
		D3DXVec3Cross(&nml, &p1p0, &p2p0);
		nml *= 0.5f;
		float area = D3DXVec3Length(&nml);
        totalArea_ += area;

        for(int j = 0;j < 3;++ j)
        {
            vtxAreas_[triangles_[i][j]] += area;
            normals_[triangles_[i][j]] += nml;
        }
    }

    const float alpha = (float)1 / (float)3;
#ifdef USE_OPENMP
    #pragma omp parallel for default(none) schedule(dynamic, 10000)
    for(int i = 0;i < (int)vertices_.size();++ i)
#else
    for(int i = 0;i < (int)vertices_.size();++ i)
#endif
    {
		D3DXVec3Normalize(&normals_[i], &normals_[i]);
        vtxAreas_[i] *= alpha;
    }
}

/*!
 * number of vertices [N/F] number of triangles
 * v0.x v0.y v0.z
 * v1.x v1.y v1.z
 * ......
 * triangle0.v0 triangle0.v1 triangle0.v2
 * triangle1.v0 triangle1.v1 triangle1.v2
 * triangle2.v0 triangle2.v1 triangle2.v2
 * ......
 */

int TriangleMesh::save_mesh_txt(const char* file) const
{
    using namespace std;

    ofstream fout(file);
    if ( !fout.good() ) return -1;

    bool nml_out;
    fout << vertices_.size();
    if ( !vertices_.empty() && vertices_.size() == normals_.size() )
    {
        fout << " N ";
        nml_out = true;
    }
    else
    {
        fout << " F ";
        nml_out = false;
    }
    fout << triangles_.size() << endl;

    fout << setprecision(16);
    for(size_t i = 0;i < vertices_.size();++ i)
        fout << vertices_[i].x 
             << ' ' << vertices_[i].y 
             << ' ' << vertices_[i].z << endl;
    // output normals
    if ( nml_out )
        for(size_t i = 0;i < normals_.size();++ i)
            fout << normals_[i].x << ' ' 
                 << normals_[i].y << ' ' 
                 << normals_[i].z << endl;

    for(size_t i = 0;i < triangles_.size();++ i)
        fout << triangles_[i].x << ' ' 
             << triangles_[i].y << ' ' 
             << triangles_[i].z << endl;

    fout.flush();
    fout.close();
    return 0;
}


int TriangleMesh::load_mesh_txt(const char* file)
{
    using namespace std;

    ifstream fin(file);
    if ( !fin.good() ) return -1;

    int nvtx, ntgl;
    char C;
    fin >> nvtx >> C >> ntgl;
    clear();
    float a, b, c;
    unsigned int v0, v1, v2;
    for(int i = 0;i < nvtx;++ i)
    {
        fin >> a >> b >> c;
        vertices_.push_back(D3DXVECTOR3(a, b, c));
    }
    if ( fin.fail() ) return -1;
    if ( C == 'N' )
    {
        for(int i = 0;i < nvtx;++ i)
        {
            fin >> a >> b >> c;
            normals_.push_back(D3DXVECTOR3(a, b, c));
        }
        if ( fin.fail() ) return -1;
    }
    for(int i = 0;i < ntgl;++ i)
    {
        fin >> v0 >> v1 >> v2;
        triangles_.push_back(Tuple3ui(v0, v1, v2));
    }
    if ( fin.fail() ) return -1;

    fin.close();
    return 0;
}


void TriangleMesh::bounding_box(D3DXVECTOR3& low, D3DXVECTOR3& up) const
{
    const float inf = std::numeric_limits<float>::infinity();
    low = D3DXVECTOR3(inf, inf, inf);
    up = D3DXVECTOR3(-inf, -inf, -inf);

    auto end = vertices_.end();
    for(auto it = vertices_.begin();
            it != end;++ it)
    {
        low.x = min(low.x, it->x);
        low.y = min(low.y, it->y);
        low.z = min(low.z, it->z);

        up.x = max(up.x, it->x);
        up.y = max(up.y, it->y);
        up.z = max(up.z, it->z);
    }
}


void TriangleMesh::get_vtx_neighborship(std::vector< std::set<int> >& tbl) const
{
    tbl.resize(vertices_.size());
    for(size_t i = 0;i < vertices_.size();++ i) tbl[i].clear();

    const std::vector<Tuple3ui>::const_iterator end = triangles_.end();
    for(std::vector<Tuple3ui>::const_iterator it = triangles_.begin();
            it != end;++ it)
    {
        tbl[it->x].insert(it->y); tbl[it->x].insert(it->z);
        tbl[it->y].insert(it->x); tbl[it->y].insert(it->z);
        tbl[it->z].insert(it->x); tbl[it->z].insert(it->y);
    }
}

/*
 * Return a list of triangles adjacent to each vertex
 */

void TriangleMesh::get_vtx_tgls(std::vector< std::set<int> >& vtx_ts) const
{
    vtx_ts.resize(vertices_.size());
    for(size_t i = 0;i < triangles_.size();++ i)
    {
        vtx_ts[ triangles_[i].x ].insert(i);
        vtx_ts[ triangles_[i].y ].insert(i);
        vtx_ts[ triangles_[i].z ].insert(i);
    }
}


#endif
