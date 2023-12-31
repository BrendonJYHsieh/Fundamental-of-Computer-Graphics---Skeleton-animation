#include "SkeletalModel.h"


SkeletalModel::SkeletalModel(GLSLProgram* shaderProgIn)
{
	m_VAO = 0;

	pScene = NULL;

	m_NumBones = 0;

	m_pShaderProg = shaderProgIn;

}

SkeletalModel::~SkeletalModel()
{
	Clear();
}

void SkeletalModel::Clear()
{
	if (m_VAO != 0) {
		glDeleteVertexArrays(1, &m_VAO);
		m_VAO = 0;
	}
}

void SkeletalModel::LoadMesh(const std::string& Filename)
{
	Clear();

	glGenVertexArrays(1, &m_VAO);
	glBindVertexArray(m_VAO);

	glGenBuffers(1, &vbo);
	glGenBuffers(1, &ebo);
	glGenBuffers(1, &boneBo);
	glGenBuffers(1, &texcoordBo);
	//GL_GenBuffers(1, &indiceBo);

	pScene = Importer.ReadFile(Filename.c_str(), 
		aiProcess_JoinIdenticalVertices |
		aiProcess_SortByPType | 
		aiProcess_Triangulate | 
		aiProcess_GenSmoothNormals | 
		aiProcess_FlipUVs |
		aiProcess_LimitBoneWeights);


	if (pScene) {
		if (pScene->mNumAnimations == 0) {
			return;
		}

		m_GlobalInverseTransform = pScene->mRootNode->mTransformation;
		m_GlobalInverseTransform.Inverse();

		InitFromScene(pScene, Filename);
	}
	else {
		printf("Error parsing '%s': '%s'\n", Filename.c_str(), Importer.GetErrorString());
	}



	glBindVertexArray(0);
}

void SkeletalModel::InitFromScene(const aiScene* pScene, const std::string& Filename)
{	
	m_Entries.resize(pScene->mNumMeshes);
	m_Materials.resize(pScene->mNumMaterials);



	std::vector<VertexStruct> vertices; 
	std::vector<VertexBoneData> bones;
	std::vector<unsigned int> Indices;

	unsigned int NumVertices = 0;
	unsigned int NumIndices = 0;


	for (unsigned int i = 0; i < m_Entries.size(); i++) {
		m_Entries[i].MaterialIndex = pScene->mMeshes[i]->mMaterialIndex;

		m_Entries[i].NumIndices = pScene->mMeshes[i]->mNumFaces * 3;

 		m_Entries[i].BaseVertex = NumVertices;
		m_Entries[i].BaseIndex = NumIndices;
		NumVertices += pScene->mMeshes[i]->mNumVertices;
		NumIndices += m_Entries[i].NumIndices;
	}

	vertices.reserve(NumVertices); 
	bones.resize(NumVertices);
	Indices.reserve(NumIndices);
	m_TexCoords.reserve(NumVertices);
	m_Indices.reserve(NumIndices);

	// Initialize the meshes in the scene one by one
	for (unsigned int i = 0; i < m_Entries.size(); i++) {
		const aiMesh* paiMesh = pScene->mMeshes[i];
		InitMesh(i, paiMesh, vertices, Indices, bones);
	}

	if (!InitMaterials(pScene, Filename)) {
		std::cout << "InitMaterials failed" << std::endl;
		return;
	}

	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(VertexStruct), &vertices[0],
		GL_STATIC_DRAW);

	// Vertex positions 
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, FALSE, sizeof(VertexStruct), (GLvoid*)0);

	// Vertex Normals
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1, 3, GL_FLOAT, FALSE, sizeof(VertexStruct), (GLvoid*)offsetof(VertexStruct, normal));

	//// Vertex Texture Coords
	//GL_EnableVertexAttribArray(2);
	//GL_VertexAttribPointer(2, 2, GL_FLOAT, FALSE, sizeof(VertexStruct), (GLvoid*)offsetof(VertexStruct, uvs));

	glBindBuffer(GL_ARRAY_BUFFER, boneBo);
	glBufferData(GL_ARRAY_BUFFER, sizeof(bones[0]) * bones.size(), &bones[0], GL_STATIC_DRAW);

	glEnableVertexAttribArray(2);
	glVertexAttribIPointer(2, 4, GL_INT, sizeof(VertexBoneData), (const GLvoid*)0);

	glEnableVertexAttribArray(3);
	glVertexAttribPointer(3, 4, GL_FLOAT, FALSE, sizeof(VertexBoneData), (const GLvoid*)16);

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(Indices[0]) * Indices.size(), &Indices[0],
		GL_STATIC_DRAW);

	glBindBuffer(GL_ARRAY_BUFFER, texcoordBo);
	glBufferData(GL_ARRAY_BUFFER, sizeof(m_TexCoords[0]) * m_TexCoords.size(), &m_TexCoords[0], GL_STATIC_DRAW);

	glEnableVertexAttribArray(4);
	glVertexAttribPointer(4, 2, GL_FLOAT,FALSE, 0, 0);

	//GL_BindBuffer(GL_ELEMENT_ARRAY_BUFFER, indiceBo);
	//GL_BufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(m_Indices[0]) * m_Indices.size(), &m_Indices[0], GL_STATIC_DRAW);

	vertices.clear();
	Indices.clear();
	bones.clear();
}

void SkeletalModel::CountVerticesAndIndices(const aiScene* pScene, unsigned int& NumVertices, unsigned int& NumIndices)
{
	for (unsigned int i = 0; i < m_Entries.size(); i++) {
		m_Entries[i].MaterialIndex = pScene->mMeshes[i]->mMaterialIndex;
		m_Entries[i].NumIndices = pScene->mMeshes[i]->mNumFaces * 3;
		m_Entries[i].BaseVertex = NumVertices;
		m_Entries[i].BaseIndex = NumIndices;

		NumVertices += pScene->mMeshes[i]->mNumVertices;
		NumIndices += m_Entries[i].NumIndices;
	}
}


void SkeletalModel::ReserveSpace(unsigned int NumVertices, unsigned int NumIndices)
{
	m_Positions.reserve(NumVertices);
	m_Normals.reserve(NumVertices);
	m_TexCoords.reserve(NumVertices);
	m_Indices.reserve(NumIndices);
}


string GetDirFromFilename(const string& Filename)
{
	// Extract the directory part from the file name
	string::size_type SlashIndex;

	SlashIndex = Filename.find_last_of("/");

	string Dir;

	if (SlashIndex == string::npos) {
		Dir = ".";
	}
	else if (SlashIndex == 0) {
		Dir = "/";
	}
	else {
		Dir = Filename.substr(0, SlashIndex);
	}

	return Dir;
}

bool SkeletalModel::InitMaterials(const aiScene* pScene, const string& Filename)
{
	string Dir = GetDirFromFilename(Filename);

	bool Ret = true;


	for (unsigned int i = 0; i < pScene->mNumMaterials; i++) {
		const aiMaterial* pMaterial = pScene->mMaterials[i];

		LoadTextures(Dir, pMaterial, i);

		LoadColors(pMaterial, i);
	}

	return Ret;
}


void SkeletalModel::LoadTextures(const string& Dir, const aiMaterial* pMaterial, int index)
{
	
	//LoadDiffuseTexture(Dir, pMaterial, index);
	//LoadSpecularTexture(Dir, pMaterial, index);
	
	LoadDiffuseTexture("./D.png", pMaterial, index);
	LoadSpecularTexture("./S.png", pMaterial, index);
}


void SkeletalModel::LoadDiffuseTexture(const string& Dir, const aiMaterial* pMaterial, int index)
{
	m_Materials[index].pDiffuse = NULL;

	if (pMaterial->GetTextureCount(aiTextureType_DIFFUSE) > 0) {
		aiString Path;

		if (pMaterial->GetTexture(aiTextureType_DIFFUSE, 0, &Path, NULL, NULL, NULL, NULL, NULL) == AI_SUCCESS) {
			string p(Path.data);
			//std::cout << "p: "<< p << std::endl;
			for (int i = 0; i < p.length(); i++) {
				if (p[i] == '\\') {
					p[i] = '/';
				}
			}

			if (p.substr(0, 2) == ".\\") {
				p = p.substr(2, p.size() - 2);
			}

			string FullPath = Dir + "/" + p;

			FullPath = "M-COC_iOS_HERO_Tony_Stark_Iron_Man_Mark_VI.png";
			//FullPath = "M-COC_iOS_HERO_Tony_Stark_Iron_Man_Mark_VII_Body_D.png";
			//std::cout << "FullPath" << FullPath << std::endl;

			m_Materials[index].pDiffuse = new Texture(GL_TEXTURE_2D, FullPath.c_str());

			if (!m_Materials[index].pDiffuse->Load()) {
				exit(0);
			}
		}
	}
}


void SkeletalModel::LoadSpecularTexture(const string& Dir, const aiMaterial* pMaterial, int index)
{
	m_Materials[index].pSpecularExponent = NULL;

	//std::cout << pMaterial->GetTextureCount(aiTextureType_SHININESS) << std::endl;
	if (pMaterial->GetTextureCount(aiTextureType_SHININESS) > 0) {
		aiString Path;

		if (pMaterial->GetTexture(aiTextureType_SHININESS, 0, &Path, NULL, NULL, NULL, NULL, NULL) == AI_SUCCESS) {
			string p(Path.data);

			if (p.substr(0, 2) == ".\\") {
				p = p.substr(2, p.size() - 2);
			}

			string FullPath = Dir + "/" + p;
			//std::cout << "FullPath" << FullPath << std::endl;

			m_Materials[index].pSpecularExponent = new Texture(GL_TEXTURE_2D, FullPath.c_str());

			if (!m_Materials[index].pSpecularExponent->Load()) {
				exit(0);
			}
		}
	}
}

void SkeletalModel::LoadColors(const aiMaterial* pMaterial, int index)
{
	aiColor3D AmbientColor(0.0f, 0.0f, 0.0f);

	if (pMaterial->Get(AI_MATKEY_COLOR_AMBIENT, AmbientColor) == AI_SUCCESS) {
		//printf("Loaded ambient color [%f %f %f]\n", AmbientColor.r, AmbientColor.g, AmbientColor.b);
		m_Materials[index].AmbientColor.r = AmbientColor.r;
		m_Materials[index].AmbientColor.g = AmbientColor.g;
		m_Materials[index].AmbientColor.b = AmbientColor.b;
	}

	aiColor3D DiffuseColor(0.0f, 0.0f, 0.0f);

	if (pMaterial->Get(AI_MATKEY_COLOR_DIFFUSE, DiffuseColor) == AI_SUCCESS) {
		//printf("Loaded diffuse color [%f %f %f]\n", DiffuseColor.r, DiffuseColor.g, DiffuseColor.b);
		m_Materials[index].DiffuseColor.r = DiffuseColor.r;
		m_Materials[index].DiffuseColor.g = DiffuseColor.g;
		m_Materials[index].DiffuseColor.b = DiffuseColor.b;
	}

	aiColor3D SpecularColor(0.0f, 0.0f, 0.0f);

	if (pMaterial->Get(AI_MATKEY_COLOR_SPECULAR, SpecularColor) == AI_SUCCESS) {
		//printf("Loaded specular color [%f %f %f]\n", SpecularColor.r, SpecularColor.g, SpecularColor.b);
		m_Materials[index].SpecularColor.r = SpecularColor.r;
		m_Materials[index].SpecularColor.g = SpecularColor.g;
		m_Materials[index].SpecularColor.b = SpecularColor.b;
	}
}

void SkeletalModel::InitMesh(unsigned int index, const aiMesh* paiMesh, std::vector<VertexStruct>& Vertices, std::vector<GLuint>& Indices, std::vector<VertexBoneData>& Bones)
{
	const aiVector3D Zero3D(0.0f, 0.0f, 0.0f);

	for (unsigned int i = 0; i < paiMesh->mNumVertices; i++) {
		const aiVector3D* pPos = &(paiMesh->mVertices[i]);
		const aiVector3D* pNormal = &(paiMesh->mNormals[i]);
		const aiVector3D* pTexCoord = paiMesh->HasTextureCoords(0) ?
			&(paiMesh->mTextureCoords[0][i]) : &Zero3D;

		glm::vec3 glmTempPos = glm::vec3(pPos->x, pPos->y, pPos->z);
		glm::vec3 glmTempNormal = glm::vec3(pNormal->x, pNormal->y, pNormal->z);
		glm::vec2 glmTempUV = glm::vec2(pTexCoord->x, pTexCoord->y);


		VertexStruct v;
		v.position = glmTempPos;
		v.normal = glmTempNormal;
		v.uvs = glmTempUV;

		Vertices.push_back(v);


		m_TexCoords.push_back(Vector2f(pTexCoord->x, pTexCoord->y));
	}
	
	if (paiMesh->HasBones()){
		LoadBones(index, paiMesh, Bones);

	}

	for (unsigned int i = 0; i < paiMesh->mNumFaces; i++) {
		const aiFace& Face = paiMesh->mFaces[i];
		assert(Face.mNumIndices == 3);
		Indices.push_back(Face.mIndices[0]);
		Indices.push_back(Face.mIndices[1]);
		Indices.push_back(Face.mIndices[2]);

		m_Indices.push_back(Face.mIndices[0]);
		m_Indices.push_back(Face.mIndices[1]);
		m_Indices.push_back(Face.mIndices[2]);
	}
	

}

void SkeletalModel::LoadBones(unsigned int MeshIndex, const aiMesh* pMesh, std::vector<VertexBoneData>& Bones)
{

	for (unsigned int i = 0; i < pMesh->mNumBones; i++) {

		unsigned int BoneIndex = 0;

		std::string BoneName(pMesh->mBones[i]->mName.data);

		if (m_BoneMapping.find(BoneName) == m_BoneMapping.end()) {

			BoneIndex = m_NumBones;

			m_NumBones++;

			BoneInfo bi;
			m_BoneInfo.push_back(bi);
		}
		else {
			BoneIndex = m_BoneMapping[BoneName];
		}

		m_BoneMapping[BoneName] = BoneIndex;

		m_BoneInfo[BoneIndex].BoneOffset = pMesh->mBones[i]->mOffsetMatrix;


		for (unsigned int j = 0; j < pMesh->mBones[i]->mNumWeights; j++) {
			unsigned int VertexID = m_Entries[MeshIndex].BaseVertex + pMesh->mBones[i]->mWeights[j].mVertexId;
			float Weight = pMesh->mBones[i]->mWeights[j].mWeight;
			Bones[VertexID].AddBoneData(BoneIndex, Weight);
		}
	}
}

void SkeletalModel::BoneTransform(float time_pass, std::vector<Matrix4f>& Transforms)
{
	if (!pScene) return;
	if (pScene->mNumAnimations == 0) {
		return;
	}
	Matrix4f Identity;
	Identity.InitIdentity();

	time += time_pass;
	

	float TicksPerSecond = pScene->mAnimations[0]->mTicksPerSecond;
	float TimeInTicks = time * TicksPerSecond;
	float AnimationTime = fmod(TimeInTicks, pScene->mAnimations[0]->mDuration);

	ReadNodeHierarchy(AnimationTime, pScene->mRootNode, Identity);

	Transforms.resize(m_NumBones);

	for (unsigned int i = 0; i < m_NumBones; i++) {
		Transforms[i] = m_BoneInfo[i].FinalTransformation;
	}

}

unsigned int SkeletalModel::FindRotation(float AnimationTime, const aiNodeAnim* pNodeAnim)
{
	assert(pNodeAnim->mNumRotationKeys > 0);

	for (unsigned int i = 0; i < pNodeAnim->mNumRotationKeys - 1; i++) {
		if (AnimationTime < (float)pNodeAnim->mRotationKeys[i + 1].mTime) {
			return i;
		}
	}
	assert(0);

	return 0;
}

unsigned int SkeletalModel::FindScale(float AnimationTime, const aiNodeAnim* pNodeAnim)
{
	assert(pNodeAnim->mNumScalingKeys > 0);

	for (unsigned int i = 0; i < pNodeAnim->mNumScalingKeys - 1; i++) {
		if (AnimationTime < (float)pNodeAnim->mScalingKeys[i + 1].mTime) {
			return i;
		}
	}
	assert(0);

	return 0;
}

unsigned int SkeletalModel::FindTranslation(float AnimationTime, const aiNodeAnim* pNodeAnim)
{
	assert(pNodeAnim->mNumPositionKeys > 0);

	for (unsigned int i = 0; i < pNodeAnim->mNumPositionKeys - 1; i++) {
		if (AnimationTime < (float)pNodeAnim->mPositionKeys[i + 1].mTime) {
			return i;
		}
	}
	assert(0);

	return 0;
}


void SkeletalModel::CalcInterpolatedRotation(aiQuaternion& Out, float AnimationTime, const aiNodeAnim* pNodeAnim)
{
	if (pNodeAnim->mNumRotationKeys == 1) {
		Out = pNodeAnim->mRotationKeys[0].mValue;
		return;
	}
	unsigned int RotationIndex = FindRotation(AnimationTime, pNodeAnim);

	unsigned int NextRotationIndex = (RotationIndex + 1);
	assert(NextRotationIndex < pNodeAnim->mNumRotationKeys);

	float DeltaTime = pNodeAnim->mRotationKeys[NextRotationIndex].mTime - pNodeAnim->mRotationKeys[RotationIndex].mTime;

	float Factor = (AnimationTime - (float)pNodeAnim->mRotationKeys[RotationIndex].mTime) / DeltaTime;

	const aiQuaternion& StartRotationQ = pNodeAnim->mRotationKeys[RotationIndex].mValue;
	const aiQuaternion& EndRotationQ = pNodeAnim->mRotationKeys[NextRotationIndex].mValue;

	aiQuaternion::Interpolate(Out, StartRotationQ, EndRotationQ, Factor);

	Out = Out.Normalize();
}

void SkeletalModel::CalcInterpolatedScaling(aiVector3D& Out, float AnimationTime, const aiNodeAnim* pNodeAnim)
{
	if (pNodeAnim->mNumScalingKeys == 1) {
		Out = pNodeAnim->mScalingKeys[0].mValue;
		return;
	}

	unsigned int ScalingIndex = FindScale(AnimationTime, pNodeAnim);
	unsigned int NextScalingIndex = (ScalingIndex + 1);
	assert(NextScalingIndex < pNodeAnim->mNumScalingKeys);
	float DeltaTime = pNodeAnim->mScalingKeys[NextScalingIndex].mTime - pNodeAnim->mScalingKeys[ScalingIndex].mTime;
	float Factor = (AnimationTime - (float)pNodeAnim->mScalingKeys[ScalingIndex].mTime) / DeltaTime;
	const aiVector3D& Start = pNodeAnim->mScalingKeys[ScalingIndex].mValue;
	const aiVector3D& End= pNodeAnim->mScalingKeys[NextScalingIndex].mValue;

	aiVector3D Delta = End - Start;
	Out = Start + Factor * Delta;
}

void SkeletalModel::CalcInterpolatedTranslation(aiVector3D& Out, float AnimationTime, const aiNodeAnim* pNodeAnim)
{
	if (pNodeAnim->mNumPositionKeys == 1) {
		Out = pNodeAnim->mPositionKeys[0].mValue;
		return;
	}


	unsigned int PositionIndex = FindTranslation(AnimationTime, pNodeAnim);
	unsigned int NextPositionIndex = (PositionIndex + 1);
	assert(NextPositionIndex < pNodeAnim->mNumPositionKeys);
	float DeltaTime = pNodeAnim->mPositionKeys[NextPositionIndex].mTime - pNodeAnim->mPositionKeys[PositionIndex].mTime;
	float Factor = (AnimationTime - (float)pNodeAnim->mPositionKeys[PositionIndex].mTime) / DeltaTime;
	const aiVector3D& Start = pNodeAnim->mPositionKeys[PositionIndex].mValue;
	const aiVector3D& End = pNodeAnim->mPositionKeys[NextPositionIndex].mValue;

	aiVector3D Delta = End - Start;
	Out = Start + Factor * Delta;
}

void SkeletalModel::SetBoneTransform(unsigned int Index, const Matrix4f& Transform )
{
	if (!pScene) return;
	if (pScene->mNumAnimations == 0) {
		return;
	}
	assert(Index < 100);

	m_pShaderProg->setUniformIndex(Index, Transform);
}

void SkeletalModel::render()
{
	if (!pScene) return;
	if (pScene->mNumAnimations == 0) {
		return;
	}
	glBindVertexArray(m_VAO);

	// Render all the model's meshes.
	for (unsigned int i = 0; i < m_Entries.size(); i++) {

		unsigned int MaterialIndex = m_Entries[i].MaterialIndex;
		assert(MaterialIndex < m_Materials.size());

		if (m_Materials[MaterialIndex].pDiffuse) {
			m_Materials[MaterialIndex].pDiffuse->Bind(GL_TEXTURE0);
		}

		if (m_Materials[MaterialIndex].pSpecularExponent) {
			m_Materials[MaterialIndex].pSpecularExponent->Bind(GL_TEXTURE6);
		}

		glDrawElementsBaseVertex(GL_TRIANGLES,
			m_Entries[i].NumIndices,
			GL_UNSIGNED_INT,
			(void*)(sizeof(unsigned int) * m_Entries[i].BaseIndex),
			m_Entries[i].BaseVertex);
	}
	glBindVertexArray(0);
}

void SkeletalModel::ReadNodeHierarchy(float AnimationTime, const aiNode* pNode, const Matrix4f& ParentTransform)
{
	Matrix4f IdentityTest;
	IdentityTest.InitIdentity();

	std::string NodeName(pNode->mName.data);

	const aiAnimation* pAnimation = pScene->mAnimations[0];

	Matrix4f NodeTransformation(pNode->mTransformation);

	const aiNodeAnim* pNodeAnim = NULL;

	for (unsigned i = 0; i < pAnimation->mNumChannels; i++) {
		const aiNodeAnim* pNodeAnimIndex = pAnimation->mChannels[i];

		if (std::string(pNodeAnimIndex->mNodeName.data) == NodeName) {
			pNodeAnim = pNodeAnimIndex;
		} 
	}

	if (pNodeAnim) {


		aiQuaternion RotationQ;
		CalcInterpolatedRotation(RotationQ, AnimationTime, pNodeAnim);
		Matrix4f RotationM = Matrix4f(RotationQ.GetMatrix());

		aiVector3D Translation;
		CalcInterpolatedTranslation(Translation, AnimationTime, pNodeAnim);
		Matrix4f TranslationM;
		TranslationM.InitTranslationTransform(Translation.x, Translation.y, Translation.z);

		NodeTransformation = TranslationM * RotationM;
	}
	
	Matrix4f GlobalTransformation = ParentTransform * NodeTransformation;
	
	if (m_BoneMapping.find(NodeName) != m_BoneMapping.end()) {
		unsigned int BoneIndex = m_BoneMapping[NodeName];
		m_BoneInfo[BoneIndex].FinalTransformation = m_GlobalInverseTransform  * GlobalTransformation *
			m_BoneInfo[BoneIndex].BoneOffset;
	}

	for (unsigned i = 0; i < pNode->mNumChildren; i++) {
		ReadNodeHierarchy(AnimationTime, pNode->mChildren[i], GlobalTransformation);
	}
}

void SkeletalModel::re_start() {
	time = 0.0f;
}