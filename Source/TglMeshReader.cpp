
#include "DXUT.h"
#include <fstream>
#include <vector>
#include "geometry/Tuple3.h"
#include "geometry/TriangleMesh.h"
#include "geometry/splooshstrings.h"
#include "TglMeshReader.h"

int MeshObjReader::read(const wchar_t* file, TriangleMesh& mesh, 
                bool centerize, bool reversetglrot)
{
    using namespace std;

    ifstream fin(file);
    if ( fin.fail() ) return E_FAIL;
    
    char text[1024];
    vector<D3DXVECTOR3>     vtx;    // temporary space to put the vertex
    vector<D3DXVECTOR3>    nml;
    vector<Tuple3ui>    tgl;
    vector<Tuple3ui>    tNml;

    int l = 1;
    fin.getline(text, 1024);
    while ( !fin.fail() )
    {
        vector<string> tokens = sploosh::tokenize(string(text));
        if ( tokens.empty() ) 
        {
            fin.getline(text, 1024);
            ++ l;
            continue;
        }

        if ( tokens[0] == "v" )
        {
            if ( tokens.size() != 4 )
            {
				printf("(A) Incorrect file format at Line %d.\n", l);
                exit(1);
            }
			vtx.push_back(D3DXVECTOR3(sploosh::Double(tokens[1]),
                                  -sploosh::Double(tokens[3]),
                                  sploosh::Double(tokens[2])));
        }
        else if ( tokens[0] == "vn" )
        {
            if ( tokens.size() != 4 )
            {
                printf("(B) Incorrect file format at Line %d.\n", l);
                exit(1);
            }
            nml.push_back(D3DXVECTOR3(sploosh::Double(tokens[1]),
                                   -sploosh::Double(tokens[3]),
                                   sploosh::Double(tokens[2])));
			D3DXVec3Normalize(&(nml.back()), &nml.back());
        }
        else if ( tokens[0] == "f" )
        {
            if ( tokens.size() != 4 )
            {
                printf("(C) Incorrect file format at Line %d (# of fields is larger than 3)\n", l);
                exit(1);
            }

            vector<string> ts0 = sploosh::split(tokens[1], '/');
            vector<string> ts1 = sploosh::split(tokens[2], '/');
            vector<string> ts2 = sploosh::split(tokens[3], '/');

            if ( ts0.size() != ts1.size() || ts1.size() != ts2.size() )
            {
                printf("(D) Incorrect file format at Line %d.\n", l);
                exit(1);
            }

            tgl.push_back(Tuple3ui(sploosh::Int(ts0[0])-1,
                                   sploosh::Int(ts1[0])-1,
                                   sploosh::Int(ts2[0])-1));
            if ( ts0.size() > 2 )
                tNml.push_back(Tuple3ui(sploosh::Int(ts0[2])-1,
                                        sploosh::Int(ts1[2])-1,
                                        sploosh::Int(ts2[2])-1));
        }

        fin.getline(text, 1024);
        ++ l;
    }

    fin.close();

    /* No triangles at all */
    if ( tgl.empty() ) printf("THERE IS NO TRIANGLE MESHS AT ALL!\n");

    /* Centerize the objects */
    if ( centerize )
    {
		D3DXVECTOR3 maxPt(
			-std::numeric_limits<float>::infinity(), 
			-std::numeric_limits<float>::infinity(), 
			-std::numeric_limits<float>::infinity()), 
                minPt(
				std::numeric_limits<float>::infinity(), 
				std::numeric_limits<float>::infinity(), 
				std::numeric_limits<float>::infinity());
        for(size_t i = 0;i < vtx.size();++ i)
        {
            maxPt.x = max(vtx[i].x, maxPt.x);
            maxPt.y = max(vtx[i].y, maxPt.y);
            maxPt.z = max(vtx[i].z, maxPt.z);

            minPt.x = min(vtx[i].x, minPt.x);
            minPt.y = min(vtx[i].y, minPt.y);
            minPt.z = min(vtx[i].z, minPt.z);
        }

		D3DXVECTOR3 center = (maxPt + minPt) * 0.5;
        for(size_t i = 0;i < vtx.size();++ i)
            vtx[i] -= center;
    }

    if ( nml.empty() )
    {
		nml.resize(vtx.size());
        for(size_t i = 0;i < tgl.size();++ i)
        {
			D3DXVECTOR3 v0 = vtx[tgl[i][0]];
			D3DXVECTOR3 v1 = vtx[tgl[i][1]];
			D3DXVECTOR3 v2 = vtx[tgl[i][2]];

			D3DXVECTOR3 e0 = v1 - v0;
			D3DXVECTOR3 e1 = v2 - v0;
			D3DXVECTOR3 norm;
			D3DXVec3Cross(&norm, &e0, &e1);
			nml[tgl[i][0]] += norm;
			nml[tgl[i][1]] += norm;
			nml[tgl[i][2]] += norm;
		}

        for(size_t i = 0;i < nml.size();++ i)
        {
			if(D3DXVec3LengthSq(&nml[i]) == 0) nml[i] = D3DXVECTOR3(0, 1, 0);
			D3DXVec3Normalize(&nml[i], &nml[i]);
		}

        for(size_t i = 0;i < vtx.size();++ i)
            mesh.add_vertex_normal(vtx[i], nml[i]);
    }
    else
    {
		vector<D3DXVECTOR3> nmlmap(vtx.size(), D3DXVECTOR3(0,0,0));

        for(size_t i = 0;i < tgl.size();++ i)
        {
			nmlmap[tgl[i][0]] += nml[tNml[i][0]];
			nmlmap[tgl[i][1]] += nml[tNml[i][1]];
			nmlmap[tgl[i][2]] += nml[tNml[i][2]];
        }

        for(size_t i = 0;i < vtx.size();++ i) {
			D3DXVec3Normalize(&nmlmap[i], &nmlmap[i]);
            mesh.add_vertex_normal(vtx[i], nmlmap[i]);
		}
    }

    if ( reversetglrot )
        for(size_t i = 0;i < tgl.size();++ i)
            mesh.add_triangle(tgl[i][0], tgl[i][2], tgl[i][1]);
    else
        for(size_t i = 0;i < tgl.size();++ i)
            mesh.add_triangle(tgl[i][0], tgl[i][1], tgl[i][2]);

    return 0;
}

const std::vector<MeshObj::VERTEX>& MeshObj::GetStoredVertices() const
{
	return m_vertices;
}

int MeshObj::Create(WCHAR* szfn, ID3D11Device* pd3dDevice, ID3D11DeviceContext* pd3dContext)
{
	Release();

	TriangleMesh mesh;
	MeshObjReader::read(szfn, mesh);

	int N = mesh.num_vertices();
	m_vertices.resize(N);
	for(int i = 0; i < N; ++i)
	{
		m_vertices[i].pos = mesh.vertices_[i];
		m_vertices[i].norm = mesh.normals_[i];
	}
	
	D3D11_BUFFER_DESC bdesc;
	bdesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	bdesc.ByteWidth = mesh.num_vertices() * sizeof(D3DXVECTOR3) * 2;
	bdesc.CPUAccessFlags = 0;
	bdesc.MiscFlags = 0;
	bdesc.StructureByteStride = sizeof(D3DXVECTOR3) * 2;
	bdesc.Usage = D3D11_USAGE_DEFAULT;

	D3D11_SUBRESOURCE_DATA srd;
	srd.pSysMem = &m_vertices[0];
	srd.SysMemPitch = 0;
	srd.SysMemSlicePitch = 0;

	pd3dDevice->CreateBuffer(&bdesc, &srd, &m_pVertexBuffer);

	bdesc.BindFlags = D3D11_BIND_INDEX_BUFFER;
	bdesc.ByteWidth = mesh.num_triangles() * sizeof(Tuple3ui);
	bdesc.CPUAccessFlags = 0;
	bdesc.MiscFlags = 0;
	bdesc.StructureByteStride = sizeof(unsigned);
	bdesc.Usage = D3D11_USAGE_DEFAULT;

	srd.pSysMem = &mesh.triangles_[0];
	srd.SysMemPitch = 0;
	srd.SysMemSlicePitch = 0;

	pd3dDevice->CreateBuffer(&bdesc, &srd, &m_pIndexBuffer);

	numVertices = mesh.num_vertices();
	numIndices = mesh.num_triangles() * 3;

	D3DXVECTOR3 bblow, bbhigh;
	mesh.bounding_box(bblow, bbhigh);
	m_bbox_center = (bblow + bbhigh) * 0.5f;
	m_bbox_extent = (bbhigh - bblow) * 0.5f;
	
	return 0;
}

void MeshObj::Render(ID3D11DeviceContext* pd3dContext,  D3D11_PRIMITIVE_TOPOLOGY topology, unsigned instancing )
{
	UINT stride = sizeof(D3DXVECTOR3) * 2;
	UINT offset = 0;
	if(topology == D3D11_PRIMITIVE_TOPOLOGY_UNDEFINED) topology = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
	pd3dContext->IASetPrimitiveTopology(topology);
	pd3dContext->IASetVertexBuffers(0, 1, &m_pVertexBuffer, &stride, &offset);
	pd3dContext->IASetIndexBuffer(m_pIndexBuffer, DXGI_FORMAT_R32_UINT, 0);
	if(instancing == 1) pd3dContext->DrawIndexed(numIndices, 0, 0);
	else pd3dContext->DrawIndexedInstanced(numIndices, instancing, 0, 0, 0);
}

MeshObj::MeshObj(const MeshObj& o)
{
	m_pIndexBuffer = o.m_pIndexBuffer;
	m_pVertexBuffer = o.m_pVertexBuffer;
	numIndices = o.numIndices;
	numVertices = o.numVertices;
}

MeshObj::MeshObj(WCHAR* szfn, ID3D11Device* pd3dDevice, ID3D11DeviceContext* pd3dContext)
{
	Create(szfn, pd3dDevice, pd3dContext);
}

MeshObj::~MeshObj()
{

}

void MeshObj::Release()
{
	SAFE_RELEASE(m_pIndexBuffer);
	SAFE_RELEASE(m_pVertexBuffer);
}

D3DXVECTOR3 MeshObj::GetMeshBBoxExtents()
{
	return m_bbox_extent;
}

D3DXVECTOR3 MeshObj::GetMeshBBoxCenter()
{
	return m_bbox_center;
}