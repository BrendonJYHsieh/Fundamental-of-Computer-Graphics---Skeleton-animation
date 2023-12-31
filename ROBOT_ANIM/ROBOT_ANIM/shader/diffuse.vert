#version 430

layout (location = 0) in vec3 VertexPosition; 
layout (location = 1) in vec3 VertexNormal;

layout (location=2) in ivec4 BoneIDs;
layout (location=3) in vec4 Weights;

layout (location = 4) in vec2 TexCoord;

out vec3 vertPos;
out vec3 N;

uniform mat3 NormalMatrix;
uniform mat4 M;
uniform mat4 V;
uniform mat4 P;

const int MAX_BONES = 70;
uniform mat4 gBones[MAX_BONES];

out vec2 TexCoord0;

void main()
{
   	mat4 BoneTransform = gBones[ BoneIDs[0] ] * Weights[0];
	BoneTransform += gBones[ BoneIDs[1] ] * Weights[1];
    BoneTransform += gBones[ BoneIDs[2] ] * Weights[2];
    BoneTransform += gBones[ BoneIDs[3] ] * Weights[3];

	vec4 tPos = BoneTransform * vec4(VertexPosition, 1.0);

	gl_Position = (P * V * M) * tPos;


	vec4 tNormal = BoneTransform * vec4(VertexNormal, 0.0);

	N = normalize( mat4(NormalMatrix) * tNormal).xyz;

    vec4 worldPos = M * tPos;

	vertPos = worldPos.xyz;     

	TexCoord0 = TexCoord;
}