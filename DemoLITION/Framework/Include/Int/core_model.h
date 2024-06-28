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

#pragma once

#include <map>
#include <vector>
#include <GL/glew.h>

#include <assimp/Importer.hpp>      // C++ importer interface
#include <assimp/scene.h>       // Output data structure
#include <assimp/postprocess.h> // Post processing flags

#include "ogldev_util.h"
#include "ogldev_math_3d.h"
#include "ogldev_texture.h"
#include "ogldev_material.h"
#include "ogldev_basic_glfw_camera.h"
#include "demolition_lights.h"
#include "demolition_model.h"

#define INVALID_MATERIAL 0xFFFFFFFF

// #define USE_MESH_OPTIMIZER

class DemolitionRenderCallbacks
{
public:
    virtual void DrawStart_CB(uint DrawIndex) = 0;

    virtual void ControlSpecularExponent_CB(bool IsEnabled) = 0;

    virtual void SetMaterial_CB(const Material& material) = 0;

    virtual void DisableDiffuseTexture_CB() = 0;

    virtual void SetWorldMatrix_CB(const Matrix4f& World) = 0;
};

class CoreRenderingSystem;

class CoreModel : public Model
{
public:
    CoreModel(CoreRenderingSystem* pCoreRenderingSystem) { m_pCoreRenderingSystem = pCoreRenderingSystem; }

    ~CoreModel();

    bool LoadAssimpModel(const std::string& Filename, int WindowWidth, int WindowHeight);

    void Render(DemolitionRenderCallbacks* pRenderCallbacks = NULL);

    void Render(uint DrawIndex, uint PrimID);

    void Render(uint NumInstances, const Matrix4f* WVPMats, const Matrix4f* WorldMats);

    PBRMaterial& GetPBRMaterial() { return m_Materials[0].PBRmaterial; };

    void GetLeadingVertex(uint DrawIndex, uint PrimID, Vector3f& Vertex);

    const std::vector<BasicCamera>& GetCameras() const { return m_cameras; }

    uint NumBones() const
    {
        return (uint)m_BoneNameToIndexMap.size();
    }

    // This is the main function to drive the animation. It receives the animation time
    // in seconds and a reference to a vector of transformation matrices (one matrix per bone).
    // It calculates the current transformation for each bone according to the current time
    // and updates the corresponding matrix in the vector. This must then be updated in the VS
    // to be accumulated for the final local position (see skinning.vs). The animation index
    // is an optional param which selects one of the animations.
    void GetBoneTransforms(float AnimationTimeSec, vector<Matrix4f>& Transforms, unsigned int AnimationIndex = 0);

    // Same as above but this one blends two animations together based on a blending factor
    void GetBoneTransformsBlended(float AnimationTimeSec,
                                  vector<Matrix4f>& Transforms,
                                  unsigned int StartAnimIndex,
                                  unsigned int EndAnimIndex,
                                  float BlendFactor);

    const std::vector<DirectionalLight>& GetDirLights() const { return m_dirLights; }
    const std::vector<SpotLight>& GetSpotLights() const { return m_spotLights; }
    const std::vector<PointLight>& GetPointLights() const { return m_pointLights; }

    void SetColorTexture(int TextureHandle);

    Texture* GetColorTexture() const { return m_pColorTexture; }

    void SetNormalMap(int TextureHandle);

    Texture* GetNormalMap() const { return m_pNormalMap; }

    void SetHeightMap(int TextureHandle);

    Texture* GetHeightMap() const { return m_pHeightMap; }

    void SetTextureScale(float Scale) { m_textureScale = Scale; }

    bool IsAnimated() const;

private:

    void Clear();

    void RenderMesh(int MeshIndex, DemolitionRenderCallbacks* pRenderCallbacks = NULL);

    template<typename VertexType>
    void ReserveSpace(std::vector<VertexType>& Vertices, uint NumVertices, uint NumIndices);

    template<typename VertexType>
    void InitSingleMesh(vector<VertexType>& Vertices, uint MeshIndex, const aiMesh* paiMesh);

    template<typename VertexType>
    void InitSingleMeshOpt(vector<VertexType>& Vertices, uint MeshIndex, const aiMesh* paiMesh);

    template<typename VertexType>
    void PopulateBuffers(vector<VertexType>& Vertices);

    template<typename VertexType>
    void PopulateBuffersNonDSA(vector<VertexType>& Vertices);

    template<typename VertexType>
    void PopulateBuffersDSA(vector<VertexType>& Vertices);

    CoreRenderingSystem* m_pCoreRenderingSystem = NULL;

    struct BasicMeshEntry {
        BasicMeshEntry()
        {
            NumIndices = 0;
            BaseVertex = 0;
            BaseIndex = 0;
            MaterialIndex = INVALID_MATERIAL;
        }

        uint NumIndices;
        uint BaseVertex;
        uint BaseIndex;
        uint MaterialIndex;
        Matrix4f Transformation;		
    };

    std::vector<BasicMeshEntry> m_Meshes;

    const aiScene* m_pScene = NULL;

    Matrix4f m_GlobalInverseTransform;

    enum BUFFER_TYPE {
        INDEX_BUFFER = 0,
        VERTEX_BUFFER = 1,
        WVP_MAT_BUFFER = 2,  // required only for instancing
        WORLD_MAT_BUFFER = 3,  // required only for instancing
        NUM_BUFFERS = 4
    };

    GLuint m_VAO = 0;

    GLuint m_Buffers[NUM_BUFFERS] = { 0 };

    struct Vertex {
        Vector3f Position;
        Vector2f TexCoords;
        Vector3f Normal;
        Vector3f Tangent;
        Vector3f Bitangent;

        void Print()
        {
            Position.Print();
            TexCoords.Print();
            Normal.Print();
            Tangent.Print();
            Bitangent.Print();
        }
    };
	
    #define MAX_NUM_BONES_PER_VERTEX 4
	
    struct VertexBoneData
    {
        uint BoneIDs[MAX_NUM_BONES_PER_VERTEX] = { 0 };
        float Weights[MAX_NUM_BONES_PER_VERTEX] = { 0.0f };
        int index = 0;  // slot for the next update

        VertexBoneData()
        {
        }

        void AddBoneData(uint BoneID, float Weight)
        {
            for (int i = 0 ; i < index ; i++) {
                if (BoneIDs[i] == BoneID) {
                    //  printf("bone %d already found at index %d old weight %f new weight %f\n", BoneID, i, Weights[i], Weight);
                    return;
                }
            }

            // The iClone 7 Raptoid Mascot (https://sketchfab.com/3d-models/iclone-7-raptoid-mascot-free-download-56a3e10a73924843949ae7a9800c97c7)
            // has a problem of zero weights causing an overflow and the assertion below. This fixes it.
            if (Weight == 0.0f) {
                return;
            }

            // printf("Adding bone %d weight %f at index %i\n", BoneID, Weight, index);

            if (index == MAX_NUM_BONES_PER_VERTEX) {
                return;
                                assert(0);
            }

            BoneIDs[index] = BoneID;
            Weights[index] = Weight;

            index++;
        }
    };

    struct SkinnedVertex {
        Vector3f Position;
        Vector2f TexCoords;
        Vector3f Normal;
        Vector3f Tangent;
        Vector3f Bitangent;
        VertexBoneData Bones;
    };


    bool InitFromScene(const aiScene* pScene, const std::string& Filename, int WindowWidth, int WindowHeight);

    bool InitGeometry(const aiScene* pScene, const string& Filename);

    template<typename VertexType>
    void InitGeometryInternal(int NumVertices, int NumIndices);

    void InitLights(const aiScene* pScene);

    void InitSingleLight(const aiScene* pScene, const aiLight& light);

    void InitDirectionalLight(const aiScene* pScene, const aiLight& light);

    void InitPointLight(const aiScene* pScene, const aiLight& light);

    void InitSpotLight(const aiScene* pScene, const aiLight& light);

    void CountVerticesAndIndices(const aiScene* pScene, uint& NumVertices, uint& NumIndices);

    template<typename VertexType>
    void InitAllMeshes(const aiScene* pScene, std::vector<VertexType>& Vertices);

    template<typename VertexType>
    void OptimizeMesh(int MeshIndex, std::vector<uint>& Indices, std::vector<VertexType>& Vertices, std::vector<VertexType>& AllVertices);

    void CalculateMeshTransformations(const aiScene* pScene);
    void TraverseNodeHierarchy(Matrix4f ParentTransformation, aiNode* pNode);

    bool InitMaterials(const aiScene* pScene, const std::string& Filename);

    void LoadTextures(const string& Dir, const aiMaterial* pMaterial, int index);

    void LoadDiffuseTexture(const string& Dir, const aiMaterial* pMaterial, int index);
    void LoadDiffuseTextureEmbedded(const aiTexture* paiTexture, int MaterialIndex);
    void LoadDiffuseTextureFromFile(const string& dir, const aiString& Path, int MaterialIndex);

    void LoadSpecularTexture(const string& Dir, const aiMaterial* pMaterial, int index);
    void LoadSpecularTextureEmbedded(const aiTexture* paiTexture, int MaterialIndex);
    void LoadSpecularTextureFromFile(const string& dir, const aiString& Path, int MaterialIndex);

    void LoadColors(const aiMaterial* pMaterial, int index);

    void InitCameras(const aiScene* pScene, int WindowWidth, int WindowHeight);

    void InitSingleCamera(int Index, const aiScene* pScene, int WindowWidth, int WindowHeight);

    std::vector<Material> m_Materials;
    Texture* m_pColorTexture = NULL;
    Texture* m_pNormalMap = NULL;
    Texture* m_pHeightMap = NULL;
	
    // Temporary space for vertex stuff before we load them into the GPU
    vector<uint> m_Indices;

    Assimp::Importer m_Importer;

    std::vector<BasicCamera> m_cameras;
    std::vector<DirectionalLight> m_dirLights;
    std::vector<PointLight> m_pointLights;
    std::vector<SpotLight> m_spotLights;
    float m_textureScale = 1.0f;
	
    /////////////////////////////////////
	// Skeletal animation stuff
    /////////////////////////////////////

    void LoadMeshBones(vector<SkinnedVertex>& SkinnedVertices, uint MeshIndex, const aiMesh* paiMesh);
    void LoadSingleBone(vector<SkinnedVertex>& SkinnedVertices, uint MeshIndex, const aiBone* pBone);
    int GetBoneId(const aiBone* pBone);
    void CalcInterpolatedScaling(aiVector3D& Out, float AnimationTime, const aiNodeAnim* pNodeAnim);
    void CalcInterpolatedRotation(aiQuaternion& Out, float AnimationTime, const aiNodeAnim* pNodeAnim);
    void CalcInterpolatedPosition(aiVector3D& Out, float AnimationTime, const aiNodeAnim* pNodeAnim);
    uint FindScaling(float AnimationTime, const aiNodeAnim* pNodeAnim);
    uint FindRotation(float AnimationTime, const aiNodeAnim* pNodeAnim);
    uint FindPosition(float AnimationTime, const aiNodeAnim* pNodeAnim);
    const aiNodeAnim* FindNodeAnim(const aiAnimation& Animation, const string& NodeName);
    void ReadNodeHierarchy(float AnimationTime, const aiNode* pNode, const Matrix4f& ParentTransform, const aiAnimation& Animation);
    void ReadNodeHierarchyBlended(float StartAnimationTimeTicksm, float EndAnimationTimeTicks, const aiNode* pNode, const Matrix4f& ParentTransform,
                                  const aiAnimation& StartAnimation, const aiAnimation& EndAnimation, float BlendFactor);
    void MarkRequiredNodesForBone(const aiBone* pBone);
    void InitializeRequiredNodeMap(const aiNode* pNode);
    float CalcAnimationTimeTicks(float TimeInSeconds, unsigned int AnimationIndex);

    struct LocalTransform {
        aiVector3D Scaling;
        aiQuaternion Rotation;
        aiVector3D Translation;
    };

    void CalcLocalTransform(LocalTransform& Transform, float AnimationTimeTicks, const aiNodeAnim* pNodeAnim);

    GLuint m_boneBuffer = 0;

    map<string,uint> m_BoneNameToIndexMap;

    struct BoneInfo
    {
        Matrix4f OffsetMatrix;
        Matrix4f FinalTransformation;

        BoneInfo(const Matrix4f& Offset)
        {
            OffsetMatrix = Offset;
            FinalTransformation.SetZero();
        }
    };

    vector<BoneInfo> m_BoneInfo;

    struct NodeInfo {

        NodeInfo() {}

        NodeInfo(const aiNode* n) { pNode = n;}

        const aiNode* pNode = NULL;
        bool isRequired = false;
    };

    map<string,NodeInfo> m_requiredNodeMap;
};

