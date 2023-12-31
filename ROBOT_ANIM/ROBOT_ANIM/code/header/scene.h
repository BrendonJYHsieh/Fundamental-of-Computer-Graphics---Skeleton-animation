#ifndef Scene_H
#define Scene_H
#include <string>
#include <vector>
#include <fstream>
#include <cctype>
#include <filesystem>
#include <map>
#include <glad/glad.h>

#include <GLFW/glfw3.h>
#include <glm/glm.hpp>

#include <assimp\Importer.hpp>
#include <assimp/scene.h>           
#include <assimp/postprocess.h>     

#include "glslprogram.h"
#include "SkeletalModel.h"
#include "controls.hpp"

using glm::mat4;
using std::string;
using std::vector;
using std::map;
namespace fs = std::filesystem;
class Scene
{
private:
    GLSLProgram* prog; 

    int width, height;

	mat4 model = mat4(1.0f); 

    map<string,SkeletalModel*> animation_db;

	SkeletalModel* m_AnimatedModel; 

    void setMatrices(); 

    void compileAndLinkShader(); 

    vector<string> animation_paths;
    vector<string> animation_names;
    vector<bool> animation_select;

    int animation_choose = 0;

    void get_all_animation_file(vector<string> scan_dirs);
    void load_all_animations();

    bool pause = false;

    float speed = 1.0f;

    vec3 lightPos = vec3(10.0f, 10.0f, 10.0f);
    float lightIntensity = 0.7f;

    vec3 ambient = vec3(0.7, 0.7, 0.7);
    vec3 diffuse = vec3(1, 1, 1);
    vec3 specular = vec3(1, 1, 1);
    float specularShininess = 1.0f;

public:
	Scene(); 

    float scale = 5.0f;

    void setLightParams(); 

    void setMaterial(); 

    void initScene(); 

    void update( float time_pass); 

    void render();	

    void resize(int, int); 

    void render_gui();

    void rotate_model();

};

#endif
