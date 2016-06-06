#ifndef TGL_MESH_READER
#define TGL_MESH_READER

#include <vector>

class TriangleMesh;

class MeshObjReader
{
    public:
        /*!
         * Read the obj file for which the normals, if specified, should be indicated
         * at each vertex. In other words, the same vertex at different faces should
         * be specified with the same normal.
         */
		static int read(const wchar_t* file, TriangleMesh& mesh, 
                bool centerize = false, bool reversetglrot = false);

};

class MeshObj
{
	ID3D11Buffer* m_pVertexBuffer;
	ID3D11Buffer* m_pIndexBuffer;

	int numVertices;
	int numIndices;
	D3DXVECTOR3 m_bbox_center;
	D3DXVECTOR3 m_bbox_extent;
public:
	struct VERTEX {
		D3DXVECTOR3 pos;
		D3DXVECTOR3 norm;
	};
protected:
	std::vector<VERTEX> m_vertices;
public:
	MeshObj() : numVertices(0), numIndices(0), m_pVertexBuffer(NULL), m_pIndexBuffer(NULL) {};
	MeshObj(const MeshObj& o);
	MeshObj(WCHAR* szfn, ID3D11Device* pd3dDevice, ID3D11DeviceContext* pd3dContext);
	void Release();
	~MeshObj();

	int Create(WCHAR* szfn, ID3D11Device* pd3dDevice, ID3D11DeviceContext* pd3dContext);
	const std::vector<VERTEX>& GetStoredVertices() const;
	void Render(ID3D11DeviceContext* pd3dContext, D3D11_PRIMITIVE_TOPOLOGY topology = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST, unsigned instancing = 1);
	D3DXVECTOR3 GetMeshBBoxExtents();
	D3DXVECTOR3 GetMeshBBoxCenter();
};

#endif