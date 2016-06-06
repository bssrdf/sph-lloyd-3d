/******************************************************************************
 *  File: Triangle.hpp
 *  Triangles in 3D space
 *  Copyright (c) 2008 by Changxi Zheng
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 ******************************************************************************/
#ifndef GEOMETRY_TRIANGLE_HPP
#   define GEOMETRY_TRIANGLE_HPP


class Triangle
{
    public:
        typedef D3DXVECTOR3   TVtx;

        /* =========== constructors ========== */
        Triangle(TVtx* v0, TVtx* v1, TVtx* v2)
        {
            vertices_[0] = v0;
            vertices_[1] = v1;
            vertices_[2] = v2;
        }

        Triangle(TVtx* v[])
        {
            vertices_[0] = v[0];
            vertices_[1] = v[1];
            vertices_[2] = v[2];
        }

        Triangle() 
        {
            vertices_[0] = vertices_[1] = vertices_[2] = NULL;
        }

        /* =========== Retrival methods ============ */
        TVtx** vertices() { return vertices_; }

        inline void init(TVtx* v0, TVtx* v1, TVtx* v2)
        {
            vertices_[0] = v0;
            vertices_[1] = v1;
            vertices_[2] = v2;
        }

        TVtx* operator [] (int n)
        {
            assert(n >= 0 && n < 3);
            return vertices_[n];
        }

        /* Using right-hand system */
        D3DXVECTOR3 weighted_normal() const // $$TESTED
        {
			D3DXVECTOR3 v1v0 = *vertices_[1] - *vertices_[0];
			D3DXVECTOR3 v2v0 = *vertices_[2] - *vertices_[0];
			D3DXVec3Cross(&v1v0, &v1v0, &v2v0);
            return (float)0.5 * v1v0;
        }

        float area() const  //$$TESTED
        {
			D3DXVECTOR3 v1v0 = *vertices_[1] - *vertices_[0];
			D3DXVECTOR3 v2v0 = *vertices_[2] - *vertices_[0];
			D3DXVec3Cross(&v1v0, &v1v0, &v2v0);
			return (float)0.5 * D3DXVec3Length(&v1v0);
        }

        static inline D3DXVECTOR3 weighted_normal(const D3DXVECTOR3& v0, 
                    const D3DXVECTOR3& v1, const D3DXVECTOR3& v2)
        {  			
			D3DXVECTOR3 v1v0 = v1 - v0;
			D3DXVECTOR3 v2v0 = v2 - v0;
			D3DXVec3Cross(&v1v0, &v1v0, &v2v0);
            return (float)0.5 * v1v0;
		}

        static inline D3DXVECTOR3 normal(const D3DXVECTOR3& v0, 
                    const D3DXVECTOR3& v1, const D3DXVECTOR3& v2)
        {  			
			D3DXVECTOR3 v1v0 = v1 - v0;
			D3DXVECTOR3 v2v0 = v2 - v0;
			D3DXVec3Cross(&v1v0, &v1v0, &v2v0);
            return v1v0;
		}

        static inline float area(const D3DXVECTOR3& v0, 
                    const D3DXVECTOR3& v1, const D3DXVECTOR3& v2)
        {  
			D3DXVECTOR3 v1v0 = v1 - v0;
			D3DXVECTOR3 v2v0 = v2 - v0;
			D3DXVec3Cross(&v1v0, &v1v0, &v2v0);
			return 0.5*D3DXVec3Length(&v1v0); 
		}

        /*
         * compute the angle formed by v0-v1-v2
         */
        static inline float angle(const D3DXVECTOR3& v0, 
                const D3DXVECTOR3& v1, const D3DXVECTOR3& v2)
        {
			D3DXVECTOR3 v1v0 = v1 - v0;
			D3DXVECTOR3 v2v1 = v2 - v1;
			float a2 = D3DXVec3LengthSq(&v1v0);
            float a  = sqrt(a2);
            float b2 = D3DXVec3LengthSq(&v2v1);
            float b  = sqrt(b2);

            float s = 2. * a * b;
            if ( fabs(s) < 1E-12 ) fprintf(stderr, "ERROR: zero-length edge encountered\n");
			D3DXVECTOR3 v0v2 = v0 - v2;
			return acos((a2 + b2 - D3DXVec3LengthSq(&v0v2)) / s);
        }

    private:
        TVtx*   vertices_[3];
};

#endif
