/*
        Copyright 2011 Etay Meiri

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "picking_technique.h"
#include "util.h"

static const char* pVS = "                                                          \n\
#version 330                                                                        \n\
                                                                                    \n\
layout (location = 0) in vec3 Position;                                             \n\
                                                                                    \n\
uniform mat4 gWVP;                                                                  \n\
                                                                                    \n\
void main()                                                                         \n\
{                                                                                   \n\
    gl_Position = gWVP * vec4(Position, 1.0);                                       \n\
}";


static const char* pFS = "                                                          \n\
#version 330                                                                        \n\
                                                                                    \n\
#extension GL_EXT_gpu_shader4 : enable                                              \n\
                                                                                    \n\
out uvec3 FragColor;                                                                \n\
                                                                                    \n\
uniform uint gDrawIndex;                                                            \n\
uniform uint gObjectIndex;                                                          \n\
                                                                                    \n\
void main()                                                                         \n\
{                                                                                   \n\
    FragColor = uvec3(gObjectIndex, gDrawIndex,gl_PrimitiveID + 1);                 \n\
}";



PickingTechnique::PickingTechnique()
{   
}

bool PickingTechnique::Init()
{
    if (!Technique::Init()) {
        return false;
    }

    if (!AddShader(GL_VERTEX_SHADER, pVS)) {
        return false;
    }

    if (!AddShader(GL_FRAGMENT_SHADER, pFS)) {
        return false;
    }
    
    if (!Finalize()) {
        return false;
    }

    m_WVPLocation = GetUniformLocation("gWVP");
    m_objectIndexLocation = GetUniformLocation("gObjectIndex");
    m_drawIndexLocation = GetUniformLocation("gDrawIndex");

    if (m_WVPLocation == INVALID_UNIFORM_LOCATION ||
        m_objectIndexLocation == INVALID_UNIFORM_LOCATION ||
        m_drawIndexLocation == INVALID_UNIFORM_LOCATION) {
        return false;
    }

    return true;
}


void PickingTechnique::SetWVP(const Matrix4f& WVP)
{
    glUniformMatrix4fv(m_WVPLocation, 1, GL_TRUE, (const GLfloat*)WVP.m);    
}


void PickingTechnique::DrawStartCB(unsigned int DrawIndex)
{
    glUniform1ui(m_drawIndexLocation, DrawIndex);
}


void PickingTechnique::SetObjectIndex(unsigned int ObjectIndex)
{
    glUniform1ui(m_objectIndexLocation, ObjectIndex);
}