
#include "scene.h"

#include <cstdio>
#include <cstdlib>
#include <iostream>
using std::cerr;
using std::endl;

#include "defines.h"

#include "objloader.hpp"

using glm::vec3;


#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/transform2.hpp>

#include <imgui/imgui.h>
#include <imgui/imgui_impl_glfw.h>
#include <imgui/imgui_impl_opengl3.h>

extern GLFWwindow* window;
extern Scene* scene;


bool menu_open = true;

bool in_UI = true;

float in_UI_change_time = 0.0f;

bool lbutton_down = false;

double mouse_click_xpos = 0, mouse_click_ypos = 0;

float model_roate_angle = 0.0f;



void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
	//process mouse cursor
	if (glfwGetKey(window, GLFW_KEY_U)) {
		if (glfwGetTime() - in_UI_change_time > 0.5f) {
			in_UI = !in_UI;
			set_using_UI(in_UI);
			if (in_UI) {
				glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
			}
			else {
				glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
				glfwSetCursorPos(window, 1024 / 2, 768 / 2);
			}
			in_UI_change_time = glfwGetTime();
		}
	}
}


void mouse_callback(GLFWwindow* window, int button, int action, int mods)
{
	if (button == GLFW_MOUSE_BUTTON_LEFT) {
		if (GLFW_PRESS == action) {
			lbutton_down = true;
			glfwGetCursorPos(window,&mouse_click_xpos,&mouse_click_ypos);
		}
		else if (GLFW_RELEASE == action) {
			lbutton_down = false;
		}
	}
}

void scroll_callback(GLFWwindow* window, double x, double y)
{
	scene->scale += y * 0.2f;
	if (scene->scale < 0.01f) scene->scale = 0.01f;
	else if(scene->scale > 10.0f) scene->scale = 10.0f;
}



Scene::Scene()
{
}

void Scene::get_all_animation_file(vector<string> scan_dirs) {
	animation_paths.clear();
	animation_names.clear();
	for (string dir : scan_dirs) {
		if (dir.back() != '/') dir.push_back('/');
		for (auto const& dir_entry : std::filesystem::directory_iterator(dir))
		{
			string path = dir_entry.path().string();
			string file_name;
			string file_extension;
			bool get_dot = false;
			for (int i = path.length() - 1; i >= 0; i--) {
				if (path[i] == '/') {
					break;
				}

				if (get_dot) {
					file_name = path[i] + file_name;
				}
				else {
					if (path[i] == '.') {
						get_dot = true;
					}
					file_extension = (char)std::tolower(path[i]) + file_extension;
				}
			}
			if (file_extension == ".fbx" || file_extension == ".dae") {
				animation_names.push_back(file_name);
				animation_paths.push_back(path);
				animation_select.push_back(false);
			}
		}
	}
}

void Scene::load_all_animations() {
	for (string path : animation_paths) {
		if (animation_db.find(path) == animation_db.end()) {
			animation_db.insert({ path, new SkeletalModel(prog) });
			animation_db[path]->LoadMesh(path);
		}
	}
}


void Scene::initScene()
{
	prog = new GLSLProgram();

	compileAndLinkShader();

	prog->initialiseBoneUniforms();

    glEnable(GL_DEPTH_TEST);

	setLightParams();

	get_all_animation_file(vector<string>{ "./", "./assets" });



	if (animation_paths.size() == 0) {
		exit(-1);
	}

	load_all_animations();


	//m_AnimatedModel = animation_db[animation_paths[0]];
	m_AnimatedModel = animation_db[animation_paths[0]];
	animation_select[0] = true;

	glfwSetInputMode(window, GLFW_STICKY_KEYS, GL_TRUE);
	glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);

	glfwPollEvents();
	glfwSetCursorPos(window, 1024 / 2, 768 / 2);

	glfwSetKeyCallback(window, key_callback);


	glfwSetMouseButtonCallback(window, mouse_callback);

	glfwSetScrollCallback(window,scroll_callback);

	set_using_UI(in_UI);
}


void Scene::update(float time_pass)
{
	std::vector<Matrix4f> Transforms;
	
	m_AnimatedModel->BoneTransform(pause ? 0.0f : time_pass * speed, Transforms);

	for (unsigned int i = 0; i < Transforms.size(); i++) {
		m_AnimatedModel->SetBoneTransform(i, Transforms[i]);
	}

}

void Scene::setLightParams()
{

	prog->setUniform("lightIntensity", glm::vec3(lightIntensity, lightIntensity, lightIntensity));
	prog->setUniform("lightPos", lightPos);
}


void Scene::setMaterial()
{
	prog->setUniform("Ka", ambient);
	prog->setUniform("Kd", diffuse);
	prog->setUniform("Ks", specular);
	prog->setUniform("specularShininess", specularShininess);
	prog->setUniform("gSampler", 0);
}

void Scene::render_gui()
{
	// Start the Dear ImGui frame
	ImGui_ImplOpenGL3_NewFrame();
	ImGui_ImplGlfw_NewFrame();
	ImGui::NewFrame();
	if (menu_open) {
		ImGui::Begin("Menu", &menu_open, ImGuiWindowFlags_MenuBar);
		if (ImGui::Button("Refresh dir")) {
			get_all_animation_file(vector<string>{ "./", "./assets" });
			if (animation_paths.size() == 0) {
				exit(-1);
			}
			load_all_animations();
		}
		if (pause) {
			if (ImGui::Button("Play")) {
				pause = false;
			}
		}
		else {
			if (ImGui::Button("Pause")) {
				pause = true;
			}
		}

		ImGui::SliderFloat("Speed", &speed, 0.f, 5.0f);
		ImGui::SliderFloat("Scale", &scale, 0.01f, 10.0f);

		ImGui::Text("Choose Animation");
		
		if (ImGui::ListBoxHeader("",ImVec2(150,200))) {
			for (int i = 0; i < animation_names.size(); i++) {
				if (ImGui::Selectable(animation_names[i].c_str(), animation_select[i]))
				{
					m_AnimatedModel = animation_db[animation_paths[i]];
					m_AnimatedModel->re_start();
					animation_select[i] = true;
					for (int j = 0; j < animation_select.size(); j++) {
						if (i == j) continue;
						animation_select[j] = false;
					}
				}
			}
			ImGui::ListBoxFooter();
		}
		ImGui::SliderFloat3("lightPos", &lightPos[0], 0.0f, 30.0f);
		ImGui::SliderFloat("lightIntensity", &lightIntensity, 0.0f,3.0f);
		ImGui::SliderFloat3("ambient", &ambient[0], 0.0f, 1.0f);
		ImGui::SliderFloat3("diffuse", &diffuse[0], 0.0f, 1.0f);
		ImGui::SliderFloat3("specular", &specular[0], 0.0f, 1.0f);
		ImGui::SliderFloat("specularShininess", &specularShininess, 0.01f, 5.0f);
		ImGui::End();
	}

	// Rendering
	ImGui::Render();
	ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

void Scene::rotate_model() {
	ImGuiIO& io = ImGui::GetIO();
	if (!io.WantCaptureMouse && in_UI && lbutton_down) {
		double xpos = 0, ypos = 0;
		glfwGetCursorPos(window, &xpos, &ypos);
		model_roate_angle -= (mouse_click_xpos - xpos);
		mouse_click_xpos = xpos;
		if (model_roate_angle >= 360.0f) model_roate_angle -= 360.0f;
		else if (model_roate_angle < 0.0f) model_roate_angle += 360.0f;
	}

	model = glm::rotate(model, glm::radians(model_roate_angle), glm::vec3(0,1,0));
}


void Scene::render()
{


    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	setMatrices();
	

	setLightParams();
	setMaterial();

	m_AnimatedModel->render();
	render_gui();
	
}

void Scene::setMatrices()
{
	model = glm::mat4(1.0f);
	rotate_model();
	model = glm::scale(model, glm::vec3(scale));
	

	computeMatricesFromInputs();
	glm::mat4 ProjectionMatrix = getProjectionMatrix();
	glm::mat4 ViewMatrix = getViewMatrix();

	

	mat3 normMat = glm::transpose(glm::inverse(mat3(model)));

	prog->setUniform("M", model);
	prog->setUniform("NormalMatrix", normMat);
	prog->setUniform("V", ViewMatrix);
	prog->setUniform("P", ProjectionMatrix);
}


void Scene::resize(int w, int h)
{
    glViewport(0,0,w,h);
    width = w;
    height = h;
	//camera.setAspectRatio((float)width/height);

}

void Scene::compileAndLinkShader()
{
   
	try {
    	prog->compileShader("shader/diffuse.vert");
    	prog->compileShader("shader/diffuse.frag");
    	prog->link();
    	prog->validate();
    	prog->use();
    } catch(GLSLProgramException & e) {
 		cerr << e.what() << endl;
 		exit( EXIT_FAILURE );
    }
}

