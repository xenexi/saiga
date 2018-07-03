/**
 * Copyright (c) 2017 Darius Rückert 
 * Licensed under the MIT License.
 * See LICENSE file for more information.
 */

#include "saiga/assets/VertexColoredModel.h"
#include "saiga/animation/objLoader2.h"

#if defined(SAIGA_VULKAN_INCLUDED) || defined(SAIGA_OPENGL_INCLUDED)
//#error This module must be independent of any graphics API.
#endif

namespace Saiga {

void VertexColoredModel::createCheckerBoard(glm::ivec2 size, float quadSize, vec4 color1, vec4 color2)
{
    vec4 n(0,1,0,0);
    for(int i = -size.x; i < size.x; ++i)
    {
        for(int j =-size.y; j < size.y; ++j)
        {
            vec4 c = (j+i%2)%2 == 0 ? color1 : color2;
            VertexNC verts[4] = {
                {{i,0,j,1},n,c},
                {{i,0,j+1,1},n,c},
                {{i+1,0,j+1,1},n,c},
                {{i+1,0,j,1},n,c},
            };

            for(int i = 0; i < 4; ++i)
            {
                verts[i].position.x *= quadSize;
                verts[i].position.z *= quadSize;
            }

            mesh.addQuad(verts);
        }
    }
}

void VertexColoredModel::loadObj(const std::string &file)
{
    Saiga::ObjLoader2 loader(file);
    loader.computeVertexColorAndData();
    loader.toTriangleMesh(mesh);
}


}






