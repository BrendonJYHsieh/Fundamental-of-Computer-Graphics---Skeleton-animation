#ifndef TEXTURE_H
#define TEXTURE_H
#include <iostream>
#include <string>
#include <glad/glad.h>

class Texture
{
public:
	Texture(GLenum TextureTarget, const std::string& FileName);

	// Should be called once to load the texture
	bool Load();

	// Must be called at least once for the specific texture unit
	void Bind(GLenum TextureUnit);

private:
	std::string m_fileName;
	GLenum m_textureTarget;
	GLuint m_textureObj;
};

#endif