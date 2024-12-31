#include "ogldev_texture.h"
#include "particles.h"

#include <iostream>
using std::endl;
using std::cerr;

#include <vector>
using std::vector;

#include <glm/gtc/matrix_transform.hpp>
using glm::vec3;

#define PRIM_RESTART 0xffffff

Particles::Particles(): bh1(5,0,0,1), bh2(-5,0,0,1)
{
    m_numParticlesX = 100;
    m_numParticlesY = 100;
    m_numParticlesZ = 100;

    m_speed = 35.0f;
    m_angle = 0.0f;

    m_totalParticles = m_numParticlesX * m_numParticlesY * m_numParticlesZ;
}

void Particles::Init()
{
    m_colorTech.Init();    
    m_particlesTech.Init();

    InitBuffers();
}

void Particles::InitBuffers()
{
    vector<Vector4f> Positions(m_totalParticles);
    CalcPositions(Positions);

    GLuint PosBuf = 0;
    GLuint VelBuf = 0;

    glGenBuffers(1, &PosBuf);
    glGenBuffers(1, &VelBuf);

    GLuint BufSize = (int)Positions.size() * sizeof(Positions[0]);

    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, PosBuf);
    glBufferData(GL_SHADER_STORAGE_BUFFER, BufSize, Positions.data(), GL_DYNAMIC_DRAW);

    vector<Vector4f> Velocities(Positions.size(), Vector4f(0.0f));
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, VelBuf);
    glBufferData(GL_SHADER_STORAGE_BUFFER, BufSize, Velocities.data(), GL_DYNAMIC_COPY);

    glGenVertexArrays(1, &m_vao);
    glBindVertexArray(m_vao);

    glBindBuffer(GL_ARRAY_BUFFER, PosBuf);
    glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 0, 0);
    glEnableVertexAttribArray(0);

    glBindVertexArray(0);

    glGenBuffers(1, &bhBuf);
    glBindBuffer(GL_ARRAY_BUFFER, bhBuf);
    GLfloat data[] = { bh1.x, bh1.y, bh1.z, bh1.w, bh2.x, bh2.y, bh2.z, bh2.w };
    glBufferData(GL_ARRAY_BUFFER, 8 * sizeof(GLfloat), data, GL_DYNAMIC_DRAW);

    glGenVertexArrays(1, &bhVao);
    glBindVertexArray(bhVao);

    glBindBuffer(GL_ARRAY_BUFFER, bhBuf);
    glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 0, 0);
    glEnableVertexAttribArray(0);

    glBindVertexArray(0);
}


void Particles::CalcPositions(vector<Vector4f>& Positions)
{
    Vector4f p(0.0f, 0.0f, 0.0f, 1.0f);

    float dx = 2.0f / (m_numParticlesX - 1);
    float dy = 2.0f / (m_numParticlesY - 1);
    float dz = 2.0f / (m_numParticlesZ - 1);

    // We want to center the particles at (0,0,0)
    glm::mat4 transf = glm::translate(glm::mat4(1.0f), glm::vec3(-1, -1, -1));

    int ParticleIndex = 0;
    for (int x = 0; x < m_numParticlesX; x++) {
        for (int y = 0; y < m_numParticlesY; y++) {
            for (int z = 0; z < m_numParticlesZ; z++) {
                p.x = dx * x;
                p.y = dy * y;
                p.z = dz * z;
                p.w = 1.0f;
                Positions[ParticleIndex] = p;
                ParticleIndex++;
                //  p = transf * p;
            }
        }
    }
}



void Particles::Update(float dt)
{ 
    m_angle += m_speed * dt;
   // printf("%f\n", m_angle);
    if (m_angle > 360.0f) {
        m_angle -= 360.0f;
    }
}

void Particles::Render(const Matrix4f& VP)
{
    // Rotate the attractors ("black holes")
    glm::mat4 rot = glm::rotate(glm::mat4(1.0f), glm::radians(m_angle), glm::vec3(0, 0, 1));
    Vector3f BlackHolePos1(glm::vec3(rot * bh1));
    Vector3f BlackHolePos2(glm::vec3(rot * bh2));

    ExecuteComputeShader(BlackHolePos1, BlackHolePos2);

    RenderParticles(BlackHolePos1, BlackHolePos2, VP);
}

void Particles::RenderParticles(const Vector3f& BlackHolePos1, const Vector3f& BlackHolePos2, const Matrix4f& VP)
{
    // Draw the scene
    m_colorTech.Enable();
    m_colorTech.SetWVP(VP);

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // Draw the particles
    glPointSize(10.0f);
    m_colorTech.SetColor(Vector4f(0.0f, 0.0f, 0.0f, 0.2f));
    glBindVertexArray(m_vao);
    glDrawArrays(GL_POINTS,0, m_totalParticles);
    glBindVertexArray(0);

    // Draw the attractors
    glPointSize(5.0f);
    GLfloat data[] = { BlackHolePos1.x, BlackHolePos1.y, BlackHolePos1.z, 1.0f, BlackHolePos2.x, BlackHolePos2.y, BlackHolePos2.z, 1.0f };
    glBindBuffer(GL_ARRAY_BUFFER, bhBuf);
    glBufferSubData( GL_ARRAY_BUFFER, 0, 8 * sizeof(GLfloat), data );
    m_colorTech.SetColor(Vector4f(1,1,0,1.0f));
    glBindVertexArray(bhVao);
    glDrawArrays(GL_POINTS, 0, 2);
    glBindVertexArray(0);
}


void Particles::ExecuteComputeShader(const Vector3f& BlackHolePos1, const Vector3f& BlackHolePos2)
{
    m_particlesTech.Enable();
    m_particlesTech.SetBlackHoles(BlackHolePos1, BlackHolePos2);

    glDispatchCompute(m_totalParticles / 1000, 1, 1);
    glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
}
