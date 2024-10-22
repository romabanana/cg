#include <algorithm>
#include <stdexcept>
#include <vector>
#include <string>
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/ext.hpp>
#include "Model.hpp"
#include "Window.hpp"
#include "Callbacks.hpp"
#include "Debug.hpp"
#include "Shaders.hpp"
#include "DrawBuffers.hpp"

#define VERSION 20241007

Window window;
DrawBuffers draw_buffers;

// models and settings
std::vector<std::string> models_names = { "chookity", "teapot", "suzanne" };
int current_model = 0;
Shader shader_texture, shader_phong;
Model mobject, mfloor, mlight;
glm::vec4 lpos;
float angle_object = 0.f, angle_light = 0.f;
bool rotate_object = true, rotate_light = true;

// extra callbacks
void keyboardCallback(GLFWwindow* glfw_win, int key, int scancode, int action, int mods);

void drawObject(const glm::mat4 &m1);
void drawFloor(bool light_on);
void drawLight();

glm::mat4 getReflectionMatrix();
glm::mat4 getShadowMatrix();

int main() {
	
	// initialize window and setup callbacks
	window = Window(win_width,win_height,"CG Demo",Window::fDefaults|Window::fBlend);
	setCommonCallbacks(window);
	glfwSetKeyCallback(window, keyboardCallback);
	
	// setup OpenGL state and load shaders
	glClearColor(0.8f,0.8f,0.7f,1.f);
	shader_texture = Shader ("shaders/texture");
	shader_phong = Shader("shaders/phong");
	
	// main loop
	mfloor = Model::loadSingle("models/floor",Model::fDontFit);
	mlight = Model::loadSingle("models/light",Model::fDontFit);
	int loaded_model = -1;
	FrameTimer ftime;
	view_target.y = .75f;
	view_pos.z *= 2;
	do {
		
		glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT|GL_STENCIL_BUFFER_BIT);
		
		// reload model if necessary
		if (loaded_model!=current_model) { 
			mobject = Model::loadSingle("models/"+models_names[current_model]);
			loaded_model = current_model;
		}
		
		view_angle = std::min(std::max(view_angle,0.01f), 1.72f);
		
		// auto-rotate
		double dt = ftime.newFrame();
		if (rotate_object) angle_object += dt;
		if (rotate_light) angle_light += dt;
		
		// update ligth position
		lpos.x = 2.0f*std::sin(angle_light);
		lpos.y = 3.0f;
		lpos.z = 2.0f*std::cos(angle_light);
		lpos.w = 1.0f;
		
		// get matrixes for drawObject
		glm::mat4 identity(1.f);
		glm::mat4 shadow = getShadowMatrix();
		glm::mat4 reflection = getReflectionMatrix();
		
		// prepare stencil
		/// @todo: generar valores diferentes en fondo, piso iluminado, sombra
		glEnable(GL_STENCIL_TEST);
		glStencilOp(GL_KEEP,GL_KEEP,GL_KEEP);
		glStencilFunc(GL_ALWAYS, 0,~0);
		
		// draw objects
		/// @todo: seleccionar la mascara y el valor de referencia adecuado para cada objecto
		
		glStencilOp(GL_KEEP,GL_KEEP,GL_KEEP);
		glStencilFunc(GL_ALWAYS, 1,~0);
		glColorMask(0,0,0,0);
		drawFloor(true);
		glColorMask(1,1,1,1);
		
		
		
		///Reflejo no anda
		
		glStencilOp(GL_KEEP,GL_KEEP, GL_KEEP);
		glStencilFunc(GL_EQUAL, 1,~0);
		
//		glDepthFunc(GL_GREATER);
		drawObject(reflection);
//		glDepthFunc(GL_LESS);		

		///Piso
		
		glEnable(GL_POLYGON_OFFSET_FILL);
		glPolygonOffset(2.0,2.0);
		glStencilOp(GL_KEEP,GL_KEEP,GL_REPLACE);
		glStencilFunc(GL_ALWAYS, 1,~0);
		drawFloor(true);
		glDisable(GL_POLYGON_OFFSET_FILL);

//		glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA); 
		
		
		///Sombra (un poco)
		
		///Pinto el stencil
		glEnable(GL_POLYGON_OFFSET_FILL);
		glPolygonOffset(1.0,1.0);
		
		glStencilOp(GL_KEEP,GL_ZERO, GL_INCR);
		glStencilFunc(GL_EQUAL, 1,~0);
		glColorMask(0,0,0,0);
		drawObject(shadow);
		glColorMask(1,1,1,1);
		glDisable(GL_POLYGON_OFFSET_FILL);
		
		glStencilOp(GL_KEEP,GL_KEEP, GL_KEEP);
		glStencilFunc(GL_EQUAL, 2,~0);
		drawFloor(true);
		drawFloor(false);
		
		
		glStencilOp(GL_KEEP,GL_KEEP,GL_KEEP);
		glStencilFunc(GL_ALWAYS, 0,~0);
		
		
		drawObject(identity);
		drawLight();	
		
		
	
		
		
		draw_buffers.draw(win_width,win_height);
		
		// settings sub-window
		window.ImGuiDialog("CG Example",[&](){
			ImGui::Combo(".obj (O)", &current_model,models_names);		
			ImGui::Checkbox("Rotate Object (R)",&rotate_object);
			ImGui::Checkbox("Rotate Light (L)",&rotate_light);
			draw_buffers.addImGuiSettings(window);
		});
		
		// finish frame
		window.finishFrame();
		
	} while( glfwGetKey(window,GLFW_KEY_ESCAPE)!=GLFW_PRESS && !glfwWindowShouldClose(window) );
}

void keyboardCallback(GLFWwindow* glfw_win, int key, int scancode, int action, int mods) {
	if (action==GLFW_PRESS) {
		switch (key) {
		case 'O': case 'M': current_model = (current_model+1)%models_names.size(); break;
		case 'R': rotate_object = !rotate_object; break;
		case 'L': rotate_light = !rotate_light; break;
		case 'B': draw_buffers.setNextBuffer(); break;
		}
	}
}

void drawModel(const Model &model, const glm::mat4 &m = glm::mat4(1.f)) {
	// select a shader
	Shader &shader = [&]()->Shader&{
		if (model.texture.isOk()) {
			model.texture.bind();
			return shader_texture;
		}
		return shader_phong;
	}();
	shader.use();
	
	auto mats = common_callbacks::getMatrixes();
	shader.setMatrixes(mats[0]*m,mats[1],mats[2]);
	
	// setup light and material
	shader.setLight(lpos, glm::vec3{1.f,1.f,1.f}, 0.4f);
	shader.setMaterial(model.material);
	
	// send geometry
	shader.setBuffers(model.buffers);
	model.buffers.draw();
}

void drawObject(const glm::mat4 &m1) {
	auto m2 = glm::translate( m1, glm::vec3(0.f,1.f,0.f) );
	auto m3 = glm::rotate( m2, 0.5f*angle_object, glm::vec3(std::sin(2.f*angle_object)/5.f,1.f,std::cos(2.f*angle_object)/5.f) );
	drawModel(mobject,m3);
}

void drawFloor(bool light_on) {
	static Material mat_orig = mfloor.material;
	if (light_on) {
		mfloor.material.kd = mat_orig.kd;
		mfloor.material.ks = mat_orig.ks;
	} else {
		mfloor.material.kd = mfloor.material.ks = glm::vec3(0.f,0.f,0.f);
	}
	drawModel(mfloor);
}

void drawLight() {
	auto m1 = glm::translate( glm::mat4(1.f), glm::vec3(lpos) );
	auto m2 = glm::scale( m1, glm::vec3( 0.05f, 0.05f, 0.05f ) );
	auto m3 = glm::rotate( m2, angle_light, glm::vec3(0.f,1.f,0.f) );
	drawModel(mlight,m3);
}

glm::mat4 getReflectionMatrix() {
	glm::mat4 matrix(1.f);
	matrix[1][1]=-1;
	return matrix;
}
glm::mat4 getShadowMatrix() {
	glm::mat4 matrix;
	
	matrix[0][0] = lpos[1];
	matrix[1][0] = -lpos[0];
	matrix[2][0] = 0.f;
	matrix[3][0] = 0.f;
	
	matrix[0][1] = 0.f;
	matrix[1][1] = 0.f;
	matrix[2][1] = 0.f;
	matrix[3][1] = 0.f;
	
	matrix[0][2] = 0.f;
	matrix[1][2] = -lpos[2];
	matrix[2][2] = lpos[1];
	matrix[3][2] = 0.f;
	
	matrix[0][3] = 0.f;
	matrix[1][3] = -lpos[3];
	matrix[2][3] = 0.f;
	matrix[3][3] = lpos[1];
	
	return matrix;
}
