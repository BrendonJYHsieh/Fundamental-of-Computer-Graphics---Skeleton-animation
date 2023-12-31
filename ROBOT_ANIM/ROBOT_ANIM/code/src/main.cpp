#include <iostream>

#include <imgui/imgui.h>
#include <imgui/imgui_impl_glfw.h>
#include <imgui/imgui_impl_opengl3.h>

#include <glad/glad.h>

#include <GLFW/glfw3.h>

#include "scene.h"


#include "controls.hpp"

#include "objloader.hpp"


#define WIN_WIDTH 1024
#define WIN_HEIGHT 768

#define MOVE_VELOCITY 0.01f
#define ROTATE_VELOCITY 0.001f

//The GLFW Window
GLFWwindow *window;

//The Scene
Scene* scene;


#define PARTSNUM 18
#define BODY 0
#define LEFTSHOUDER 1
#define ULEFTARM 2
#define DLEFTARM 3
#define LEFTHAND 4
#define DOR(angle) (angle*3.1415/180);
bool isFrame;

GLuint VAO;
GLuint VBO;
GLuint uVBO;
GLuint nVBO;
GLuint mVBO;
GLuint UBO;
GLuint VBOs[PARTSNUM];
GLuint uVBOs[PARTSNUM];
GLuint nVBOs[PARTSNUM];
GLuint program;
int pNo;

float angles[PARTSNUM];
float position = 0.0;
float angle = 0.0;
float eyeAngley = 0.0;
float eyedistance = 20.0;
float size = 1;
GLfloat movex, movey;
GLint MatricesIdx;
GLuint ModelID;

int vertices_size[PARTSNUM];
int uvs_size[PARTSNUM];
int normals_size[PARTSNUM];
int materialCount[PARTSNUM];

std::vector<std::string> mtls[PARTSNUM];//use material
std::vector<unsigned int> faces[PARTSNUM];//face count
map<string, vec3> KDs;//mtl-name&Kd
map<string, vec3> KSs;//mtl-name&Ks

mat4 Projection;
mat4 View;
mat4 Model;
mat4 Models[PARTSNUM];


void load2Buffer(const char* obj, int i) {
	std::vector<vec3> vertices;
	std::vector<vec2> uvs;
	std::vector<vec3> normals; // Won't be used at the moment.
	std::vector<unsigned int> materialIndices;

	bool res = loadOBJ(obj, vertices, uvs, normals, faces[i], mtls[i]);
	if (!res) printf("load failed\n");

	//glUseProgram(program);

	glGenBuffers(1, &VBOs[i]);
	glBindBuffer(GL_ARRAY_BUFFER, VBOs[i]);
	glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(vec3), &vertices[0], GL_STATIC_DRAW);
	vertices_size[i] = vertices.size();

	//(buffer type,data起始位置,data size,data first ptr)
	//vertices_size[i] = glm_model->numtriangles;

	//printf("vertices:%d\n",vertices_size[);

	glGenBuffers(1, &uVBOs[i]);
	glBindBuffer(GL_ARRAY_BUFFER, uVBOs[i]);
	glBufferData(GL_ARRAY_BUFFER, uvs.size() * sizeof(vec2), &uvs[0], GL_STATIC_DRAW);
	uvs_size[i] = uvs.size();

	glGenBuffers(1, &nVBOs[i]);
	glBindBuffer(GL_ARRAY_BUFFER, nVBOs[i]);
	glBufferData(GL_ARRAY_BUFFER, normals.size() * sizeof(vec3), &normals[0], GL_STATIC_DRAW);
	normals_size[i] = normals.size();
}

mat4 translate(float x, float y, float z) {
	vec4 t = vec4(x, y, z, 1);//w = 1 ,則x,y,z=0時也能translate
	vec4 c1 = vec4(1, 0, 0, 0);
	vec4 c2 = vec4(0, 1, 0, 0);
	vec4 c3 = vec4(0, 0, 1, 0);
	mat4 M = mat4(c1, c2, c3, t);
	return M;
}
mat4 scale(float x, float y, float z) {
	vec4 c1 = vec4(x, 0, 0, 0);
	vec4 c2 = vec4(0, y, 0, 0);
	vec4 c3 = vec4(0, 0, z, 0);
	vec4 c4 = vec4(0, 0, 0, 1);
	mat4 M = mat4(c1, c2, c3, c4);
	return M;
}

mat4 rotate(float angle, float x, float y, float z) {
	float r = DOR(angle);
	mat4 M = mat4(1);

	vec4 c1 = vec4(cos(r) + (1 - cos(r)) * x * x, (1 - cos(r)) * y * x + sin(r) * z, (1 - cos(r)) * z * x - sin(r) * y, 0);
	vec4 c2 = vec4((1 - cos(r)) * y * x - sin(r) * z, cos(r) + (1 - cos(r)) * y * y, (1 - cos(r)) * z * y + sin(r) * x, 0);
	vec4 c3 = vec4((1 - cos(r)) * z * x + sin(r) * y, (1 - cos(r)) * z * y - sin(r) * x, cos(r) + (1 - cos(r)) * z * z, 0);
	vec4 c4 = vec4(0, 0, 0, 1);
	M = mat4(c1, c2, c3, c4);
	return M;
}

using namespace glm;
void Obj2Buffer() {
	std::vector<vec3> Kds;
	std::vector<vec3> Kas;
	std::vector<vec3> Kss;
	std::vector<std::string> Materials;//mtl-name
	std::string texture;
	loadMTL("Obj/gundam.mtl", Kds, Kas, Kss, Materials, texture);
	//printf("%d\n",texture);
	for (int i = 0; i < Materials.size(); i++) {
		string mtlname = Materials[i];
		//  name            vec3
		KDs[mtlname] = Kds[i];
	}


	load2Buffer("Obj/body.obj", 0);

	load2Buffer("Obj/ulefthand.obj", 1);
	load2Buffer("Obj/dlefthand.obj", 2);
	load2Buffer("Obj/lefthand.obj", 3);
	load2Buffer("Obj/lshouder.obj", 4);

	load2Buffer("Obj/head.obj", 5);

	load2Buffer("Obj/urighthand.obj", 6);
	load2Buffer("Obj/drighthand.obj", 7);
	load2Buffer("Obj/righthand.obj", 8);
	load2Buffer("Obj/rshouder.obj", 9);

	load2Buffer("Obj/dbody.obj", 11);
	load2Buffer("Obj/back2.obj", 10);

	load2Buffer("Obj/uleftleg.obj", 12);
	load2Buffer("Obj/dleftleg.obj", 13);
	load2Buffer("Obj/leftfoot.obj", 14);

	load2Buffer("Obj/urightleg.obj", 15);
	load2Buffer("Obj/drightleg.obj", 16);
	load2Buffer("Obj/rightfoot.obj", 17);

	GLuint totalSize[3] = { 0,0,0 };
	GLuint offset[3] = { 0,0,0 };
	for (int i = 0; i < PARTSNUM; i++) {
		totalSize[0] += vertices_size[i] * sizeof(vec3);
		totalSize[1] += uvs_size[i] * sizeof(vec2);
		totalSize[2] += normals_size[i] * sizeof(vec3);
	}
	//generate vbo
	glGenBuffers(1, &VBO);
	glGenBuffers(1, &uVBO);
	glGenBuffers(1, &nVBO);
	//bind vbo ,第一次bind也同等於 create vbo 
	glBindBuffer(GL_ARRAY_BUFFER, VBO);//VBO的target是GL_ARRAY_BUFFER
	glBufferData(GL_ARRAY_BUFFER, totalSize[0], NULL, GL_STATIC_DRAW);

	glBindBuffer(GL_ARRAY_BUFFER, uVBO);//VBO的target是GL_ARRAY_BUFFER
	glBufferData(GL_ARRAY_BUFFER, totalSize[1], NULL, GL_STATIC_DRAW);

	glBindBuffer(GL_ARRAY_BUFFER, nVBO);//VBO的target是GL_ARRAY_BUFFER
	glBufferData(GL_ARRAY_BUFFER, totalSize[2], NULL, GL_STATIC_DRAW);


	for (int i = 0; i < PARTSNUM; i++) {
		glBindBuffer(GL_COPY_WRITE_BUFFER, VBO);
		glBindBuffer(GL_COPY_READ_BUFFER, VBOs[i]);
		glCopyBufferSubData(GL_COPY_READ_BUFFER, GL_COPY_WRITE_BUFFER, 0, offset[0], vertices_size[i] * sizeof(vec3));
		offset[0] += vertices_size[i] * sizeof(vec3);
		glInvalidateBufferData(VBOs[i]);//free vbo
		glBindBuffer(GL_COPY_WRITE_BUFFER, 0);

		glBindBuffer(GL_COPY_WRITE_BUFFER, uVBO);
		glBindBuffer(GL_COPY_READ_BUFFER, uVBOs[i]);
		glCopyBufferSubData(GL_COPY_READ_BUFFER, GL_COPY_WRITE_BUFFER, 0, offset[1], uvs_size[i] * sizeof(vec2));
		offset[1] += uvs_size[i] * sizeof(vec2);
		glInvalidateBufferData(uVBOs[i]);//free vbo
		glBindBuffer(GL_COPY_WRITE_BUFFER, 0);

		glBindBuffer(GL_COPY_WRITE_BUFFER, nVBO);
		glBindBuffer(GL_COPY_READ_BUFFER, nVBOs[i]);
		glCopyBufferSubData(GL_COPY_READ_BUFFER, GL_COPY_WRITE_BUFFER, 0, offset[2], normals_size[i] * sizeof(vec3));
		offset[2] += normals_size[i] * sizeof(vec3);
		glInvalidateBufferData(uVBOs[i]);//free vbo
		glBindBuffer(GL_COPY_WRITE_BUFFER, 0);
	}
	glBindBuffer(GL_COPY_WRITE_BUFFER, 0);


}

void updateModels() {
	mat4 Rotatation[PARTSNUM];
	mat4 Translation[PARTSNUM];
	for (int i = 0; i < PARTSNUM; i++) {
		Models[i] = mat4(1.0f);
		Rotatation[i] = mat4(1.0f);
		Translation[i] = mat4(1.0f);
	}
	float r, pitch, yaw, roll;
	float alpha, beta, gamma;

	//Body
	beta = angle;
	Rotatation[0] = rotate(beta, 0, 1, 0);
	Translation[0] = translate(0, 2.9 + position, 0);
	Models[0] = Translation[0] * Rotatation[0];
	//左手=======================================================
	//左上手臂
	yaw = DOR(beta); r = 3.7;
	alpha = angles[1];
	gamma = 10;
	Rotatation[1] = rotate(alpha, 1, 0, 0) * rotate(gamma, 0, 0, 1);//向前旋轉*向右旋轉
	Translation[1] = translate(3.7, 1, -0.5);

	Models[1] = Models[0] * Translation[1] * Rotatation[1];

	//左肩膀
	Rotatation[4] = rotate(alpha, 1, 0, 0) * rotate(gamma, 0, 0, 1);//向前旋轉*向右旋轉
	Translation[4] = translate(3.7, 1, -0.5);//位移到左上手臂處
	Models[4] = Models[0] * Translation[1] * Rotatation[1];

	//左下手臂
	pitch = DOR(alpha); r = 3;
	roll = DOR(gamma);
	static int i = 0;
	i += 5;
	alpha = angles[2] - 20;
	//上手臂+下手臂向前旋轉*向右旋轉
	Rotatation[2] = rotate(alpha, 1, 0, 0);
	//延x軸位移以上手臂為半徑的圓周長:translate(0,r*cos,r*sin)
	//延z軸位移以上手臂為半徑角度:translate(r*sin,-rcos,0)
	Translation[2] = translate(0, -3, 0);

	Models[2] = Models[1] * Translation[2] * Rotatation[2];


	pitch = DOR(alpha);
	//b = DOR(angles[2]);
	roll = DOR(gamma);
	//手掌角度與下手臂相同
	//Rotatation[3] = Rotatation[2];
	//延x軸位移以上手臂為半徑的圓周長:translate(0,r*cos,r*sin) ,角度為上手臂+下手臂
	Translation[3] = translate(0, -4.8, 0);
	Models[3] = Models[2] * Translation[3] * Rotatation[3];
	//============================================================
	//頭==========================================================
	Translation[5] = translate(0, 3.9, -0.5);
	Models[5] = Models[0] * Translation[5] * Rotatation[5];
	//============================================================
	//右手=========================================================
	gamma = -10; alpha = angles[6] = -angles[1];
	Rotatation[6] = rotate(alpha, 1, 0, 0) * rotate(gamma, 0, 0, 1);
	Translation[6] = translate(-3.9, 1.7, -0.2);
	Models[6] = Models[0] * Translation[6] * Rotatation[6];

	Rotatation[9] = rotate(alpha, 1, 0, 0) * rotate(gamma, 0, 0, 1);
	Translation[9] = translate(-3.9, 1.1, -0.2);
	Models[9] = Models[0] * Translation[9] * Rotatation[9];

	angles[7] = angles[2];
	pitch = DOR(alpha); r = -3;
	roll = DOR(gamma);
	alpha = angles[7] - 20;
	Rotatation[7] = rotate(alpha, 1, 0, 0);
	Translation[7] = translate(0, -3, 0);
	Models[7] = Models[6] * Translation[7] * Rotatation[7];

	pitch = DOR(alpha);
	//b = DOR(angles[7]);
	roll = DOR(gamma);
	Translation[8] = translate(0, -6, 0);
	Models[8] = Models[7] * Translation[8] * Rotatation[8];
	//=============================================================
	//back&DBody===================================================
	Translation[10] = translate(0, 2, -4.5);
	Models[10] = Models[0] * Translation[10] * Rotatation[10];

	Translation[11] = translate(0, -5.3, 0);
	Models[11] = Models[0] * Translation[11] * Rotatation[11];
	//=============================================================
	//左腳
	alpha = angles[12]; gamma = 10;
	Rotatation[12] = rotate(alpha, 1, 0, 0) * rotate(gamma, 0, 0, 1);
	Translation[12] = translate(1.8, -4.5, 0);
	Models[12] = Translation[12] * Rotatation[12] * Models[12];

	pitch = DOR(alpha); r = -7;
	roll = DOR(gamma);
	alpha = angles[13] + angles[12];
	Translation[13] = translate(-r * sin(roll), r * cos(pitch), r * sin(pitch)) * Translation[12];
	Rotatation[13] = rotate(alpha, 1, 0, 0);
	Models[13] = Translation[13] * Rotatation[13] * Models[13];

	pitch = DOR(alpha); r = -5;
	//b = DOR(angles[13]);
	roll = DOR(gamma);
	Translation[14] = translate(-(r + 2) * sin(roll), r * cos(pitch), r * sin(pitch) - 1) * Translation[13];
	Rotatation[14] = rotate(alpha, 1, 0, 0);
	Models[14] = Translation[14] * Rotatation[14] * Models[14];
	//=============================================================
	//右腳
	alpha = angles[15] = -angles[12];
	gamma = -10;
	Rotatation[15] = rotate(alpha, 1, 0, 0) * rotate(gamma, 0, 0, 1);
	Translation[15] = translate(-1.8, -4.5, 0);
	Models[15] = Translation[15] * Rotatation[15] * Models[15];

	angles[16] = angles[13];
	pitch = DOR(alpha); r = -7;
	roll = DOR(gamma);
	alpha = angles[16] + angles[15];
	Rotatation[16] = rotate(alpha, 1, 0, 0);
	Translation[16] = translate(-r * sin(roll), r * cos(pitch), r * sin(pitch)) * Translation[15];
	Models[16] = Translation[16] * Rotatation[16] * Models[16];

	pitch = DOR(alpha); r = -5;
	//b = DOR(angles[16]);
	roll = DOR(gamma);
	alpha = angles[15] + angles[16];
	Translation[17] = translate(-(r + 2) * sin(roll), r * cos(pitch), r * sin(pitch) - 0.5) * Translation[16];
	Rotatation[17] = rotate(alpha, 1, 0, 0);
	Models[17] = Translation[17] * Rotatation[17] * Models[17];
	//=============================================================
}

void load2Buffer(char* obj, int i) {
	std::vector<vec3> vertices;
	std::vector<vec2> uvs;
	std::vector<vec3> normals; // Won't be used at the moment.
	std::vector<unsigned int> materialIndices;

	bool res = loadOBJ(obj, vertices, uvs, normals, faces[i], mtls[i]);
	if (!res) printf("load failed\n");

	//glUseProgram(program);

	glGenBuffers(1, &VBOs[i]);
	glBindBuffer(GL_ARRAY_BUFFER, VBOs[i]);
	glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(vec3), &vertices[0], GL_STATIC_DRAW);
	vertices_size[i] = vertices.size();

	//(buffer type,data起始位置,data size,data first ptr)
	//vertices_size[i] = glm_model->numtriangles;

	//printf("vertices:%d\n",vertices_size[);

	glGenBuffers(1, &uVBOs[i]);
	glBindBuffer(GL_ARRAY_BUFFER, uVBOs[i]);
	glBufferData(GL_ARRAY_BUFFER, uvs.size() * sizeof(vec2), &uvs[0], GL_STATIC_DRAW);
	uvs_size[i] = uvs.size();

	glGenBuffers(1, &nVBOs[i]);
	glBindBuffer(GL_ARRAY_BUFFER, nVBOs[i]);
	glBufferData(GL_ARRAY_BUFFER, normals.size() * sizeof(vec3), &normals[0], GL_STATIC_DRAW);
	normals_size[i] = normals.size();
}

void initializeGL() {

    glClearColor(0.5f,0.5f,0.5f,1.0f);
	scene = new Scene();
    scene->initScene();

}


void mainLoop() {

	long long f_startTime = glfwGetTime();

	while( ! glfwWindowShouldClose(window) && !glfwGetKey(window, GLFW_KEY_ESCAPE) ) {
		//GLUtils::checkForOpenGLError(__FILE__,__LINE__);
		
		static double fCurrentTime;

		float time_pass = glfwGetTime() - fCurrentTime;

		fCurrentTime = glfwGetTime();

		float fInterval = (fCurrentTime - (double)f_startTime);

		scene->update(time_pass);

		scene->render();

		glfwSwapBuffers(window);
		glfwPollEvents();

		f_startTime = fCurrentTime;

	}
}

void resizeGL(int w, int h ) {
    scene->resize(w,h);
}


int main(int argc, char* argv[])
{
	// Initialize GLFW
	if( !glfwInit() ) exit( EXIT_FAILURE );

	glfwWindowHint( GLFW_CONTEXT_VERSION_MAJOR, 4 );
	glfwWindowHint( GLFW_CONTEXT_VERSION_MINOR, 3 );
	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, TRUE);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	glfwWindowHint(GLFW_RESIZABLE, FALSE);
	glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, TRUE);

	string title = "Project 1 robot animation";
	window = glfwCreateWindow( WIN_WIDTH, WIN_HEIGHT, title.c_str(), NULL, NULL );
	if( ! window ) {
		glfwTerminate();
		exit( EXIT_FAILURE );
	}
	glfwMakeContextCurrent(window);

	bool err = gladLoadGL() == 0;
	if (err)
	{
		fprintf(stderr, "Failed to initialize OpenGL loader!\n");
		return 1;
	}



	// Setup Dear ImGui context
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO(); (void)io;
	ImGui::StyleColorsDark();

	// Setup Platform/Renderer bindings
	ImGui_ImplGlfw_InitForOpenGL(window, true);
	ImGui_ImplOpenGL3_Init("#version 430"); //glsl_version可以使用字符串"#version 150"替代
	ImGuiStyle& style = ImGui::GetStyle();


	ImGui::SetNextWindowPos(ImVec2(0, 0));
	ImGui::SetNextWindowSize(ImVec2(300, 150));

	// Initialization
	initializeGL();

	resizeGL(WIN_WIDTH,WIN_HEIGHT);


	// Enter the main loop
	mainLoop();

	// Close window and terminate GLFW
	glfwTerminate();

	delete scene;

	// Exit program
	exit( EXIT_SUCCESS );
}




