#ifndef SKELETALMODEL_H
#define SKELETALMODEL_H

#include <glad/glad.h>
#include "glm\glm.hpp"
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <vector>
#include <map>
#include "Math3D.h"
#include "glslprogram.h"
#include "Texture.h"

struct VertexStruct 
{
	glm::vec3 position;
	glm::vec3 normal;
	glm::vec2 uvs;
};

struct VertexBoneData
{
	unsigned int IDs[4];
	float Weights[4];

	VertexBoneData()
	{
		Reset();
	}

	void Reset()
	{
		memset(IDs, 0, 4 * sizeof(IDs[0]));
		memset(Weights, 0, 4 * sizeof(Weights[0]));
	}

	void AddBoneData(unsigned int BoneID, float Weight)
	{
		for (unsigned int i = 0; i < 4; i++) {

			if (Weights[i] == 0.0) {
				IDs[i] = BoneID;

				Weights[i] = Weight;
				return;
			}

		}
		assert(0);
	}
};


struct BoneInfo
{
	Matrix4f FinalTransformation;
	Matrix4f BoneOffset;

	BoneInfo()
	{
		BoneOffset.SetZero();
		FinalTransformation.SetZero();
	}
};

#define INVALID_MATERIAL 0xFFFFFFFF
struct MeshEntry {


	unsigned int BaseVertex;
	unsigned int BaseIndex;
	unsigned int NumIndices;
	unsigned int MaterialIndex; 

	MeshEntry()
	{

		NumIndices = 0; 
		BaseVertex = 0;  
		BaseIndex = 0; 
		MaterialIndex = INVALID_MATERIAL;
	}

	~MeshEntry() {}
};




class Material {

public:
	Vector3f AmbientColor = Vector3f(0.0f, 0.0f, 0.0f);
	Vector3f DiffuseColor = Vector3f(0.0f, 0.0f, 0.0f);
	Vector3f SpecularColor = Vector3f(0.0f, 0.0f, 0.0f);


	Texture* pDiffuse = NULL;
	Texture* pSpecularExponent = NULL;
};

class SkeletalModel
{
public:

	SkeletalModel(GLSLProgram* shaderProgIn);

	~SkeletalModel();

	void LoadMesh(const std::string& Filename);

	void BoneTransform(float TimeInSeconds, std::vector<Matrix4f>& Transforms);

	void SetBoneTransform(unsigned int Index, const Matrix4f& Transform);

	void render();

	void re_start();


private:
	
	void LoadBones(unsigned int MeshIndex, const aiMesh* pMesh, std::vector<VertexBoneData>& Bones); //!< Loads the bone data from a given mesh. 
	void CalcInterpolatedRotation(aiQuaternion& Out, float AnimationTime, const aiNodeAnim* pNodeAnim); //!< Calculates the interpolated quaternion between two keyframes. 
	void CalcInterpolatedScaling(aiVector3D& Out, float AnimationTime, const aiNodeAnim* pNodeAnim); //!< Calculates the interpolated scaling vector between two keyframes. 
	void CalcInterpolatedTranslation(aiVector3D& Out, float AnimationTime, const aiNodeAnim* pNodeAnim); //!< Calculates the interpolated translation vector between two keyframes. 

	unsigned int FindRotation(float AnimationTime, const aiNodeAnim* pNodeAnim); //!< Finds a rotation key given the current animation time. 
	unsigned int FindScale(float AnimationTime, const aiNodeAnim* pNodeAnim); // Finds a scaling key given the current animation time. 
	unsigned int FindTranslation(float AnimationTime, const aiNodeAnim* pNodeAnim); // Finds a translation key given the current animation time. 

	void ReadNodeHierarchy(float AnimationTime, const aiNode* pNode, const Matrix4f& ParentTransform);


	void InitFromScene(const aiScene* pScene, const std::string& Filename);
	void InitMesh(unsigned int index, const aiMesh* paiMesh, std::vector<VertexStruct>& Vertices, 
		std::vector<GLuint>& Indices, std::vector<VertexBoneData>& Bones);

	void Clear();

	bool InitMaterials(const aiScene* pScene, const std::string& Filename);

	void LoadTextures(const string& Dir, const aiMaterial* pMaterial, int index);

	void LoadDiffuseTexture(const string& Dir, const aiMaterial* pMaterial, int index);

	void LoadSpecularTexture(const string& Dir, const aiMaterial* pMaterial, int index);

	void LoadColors(const aiMaterial* pMaterial, int index);

	void ReserveSpace(unsigned int NumVertices, unsigned int NumIndices);

	void CountVerticesAndIndices(const aiScene* pScene, unsigned int& NumVertices, unsigned int& NumIndices);

	float time = 0.0f;

	GLSLProgram* m_pShaderProg;

	GLuint m_VAO;
	GLuint vbo;
	GLuint ebo;
	GLuint boneBo; 
	GLuint texcoordBo;
	GLuint indiceBo; 

	const aiScene* pScene; 

	Assimp::Importer Importer; 

	unsigned int m_NumBones; 

	std::map<std::string, unsigned int> m_BoneMapping; 

	std::vector<BoneInfo> m_BoneInfo; 
	
	Matrix4f GlobalTransformation; 
	Matrix4f m_GlobalInverseTransform; 

	std::vector<MeshEntry> m_Entries; 
	std::vector<Material> m_Materials;


	std::vector<Vector3f> m_Positions;
	std::vector<Vector3f> m_Normals;
	std::vector<Vector2f> m_TexCoords;
	std::vector<unsigned int> m_Indices;
};


#endif