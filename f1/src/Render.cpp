#include <glm/ext.hpp>
#include "Render.hpp"
#include "Callbacks.hpp"
#include <vector>
#include <iostream>

extern bool wireframe, play, top_view, antialiasing;

// matrices que definen la camara
glm::mat4 projection_matrix, view_matrix;

// función para renderizar cada "parte" del auto
void renderPart(const Car &car, const std::vector<Model> &v_models, const glm::mat4 &matrix, Shader &shader) {
	// select a shader
	
	glm::mat4 desp ( 1.0f, 0.0f, 0.0f, 0.0f, // nuevo eje x
					0.0f, 1.0f, 0.0f, 0.0f, // nuevo eje y
					0.0f, 0.0f, 1.0f, 0.0f, // nuevo eje z
					car.x, 0.0f, car.y, 1.0f ); // desplazamiento
	
	glm::mat4 rot ( cos(-car.ang), 0.0f, -sin(-car.ang), 0.0f, // nuevo eje x
					0.0f, 1.0f, 0.0f, 0.0f, // nuevo eje y
					sin(-car.ang), 0.0f, cos(-car.ang), 0.0f, // nuevo eje z
					0.0f, 0.0f, 0.0f, 1.0f ); // desplazamiento
	
	for(const Model &model : v_models) {
		shader.use();
		
		// matrixes
		if (play) {
			/// @todo: modificar una de estas matrices para mover todo el auto (todas
			///        las partes) a la posición (y orientación) que le corresponde en la pista
			
			
			glm::mat4 new_matrix = view_matrix * desp * rot;
			// why view matrix --> cam
			
			shader.setMatrixes(matrix,new_matrix,projection_matrix);
			
		} else {
			glm::mat4 model_matrix = glm::rotate(glm::mat4(1.f),view_angle,glm::vec3{1.f,0.f,0.f}) *
						             glm::rotate(glm::mat4(1.f),model_angle,glm::vec3{0.f,1.f,0.f}) *
			                         matrix;
			shader.setMatrixes(model_matrix,view_matrix,projection_matrix);
		}
		
		// setup light and material
		shader.setLight(glm::vec4{20.f,40.f,20.f,0.f}, glm::vec3{1.f,1.f,1.f}, 0.35f);
		shader.setMaterial(model.material);
		
		// send geometry
		shader.setBuffers(model.buffers);
		glPolygonMode(GL_FRONT_AND_BACK,(wireframe and (not play))?GL_LINE:GL_FILL);
		model.buffers.draw();
	}
}

// función que actualiza las matrices que definen la cámara
void setViewAndProjectionMatrixes(const Car &car) {
	projection_matrix = glm::perspective( glm::radians(view_fov), float(win_width)/float(win_height), 0.1f, 100.f );
	if (play) {
		if (top_view) {
			/// @todo: modificar el look at para que en esta vista el auto siempre apunte hacia arriba
			glm::vec3 pos_auto = {car.x, 0.f, car.y};
//			glm::vec3 ang_cam = {-car.x, 0.f, -car.y}; //idk why
			glm::vec3 ang_cam = {cos(car.ang), 0.f, sin(car.ang)}; //smart
			
			view_matrix = glm::lookAt( pos_auto+glm::vec3{0.f,30.f,0.f}, pos_auto, ang_cam );
		} else {
			/// @todo: definir view_matrix de modo que la camara persiga al auto desde "atras"
			glm::vec3 pos_auto = {car.x, 0.f, car.y};
			glm::vec3 ang_cam = {cos(car.ang), 0.f, sin(car.ang)}; //smart
			
			
			view_matrix = glm::lookAt(pos_auto -4.f*ang_cam + glm::vec3(0.f, 5.75f, 0.f), pos_auto, glm::vec3(0.f, 1.f, 0.f) );
//			view_matrix = glm::lookAt(pos_auto -4.f*ang_cam + glm::vec3(0.f, 0.15f, 0.f), pos_auto, glm::vec3(0.f, 1.f, 0.f) );
			
		}
	} else {
		view_matrix = glm::lookAt( glm::vec3{0.f,0.f,3.f}, view_target, glm::vec3{0.f,1.f,0.f} );
	}
}

// función que rendiriza todo el auto, parte por parte
void renderCar(const Car &car, const std::vector<Part> &parts, Shader &shader) {
	const Part &axis = parts[0], &body = parts[1], &wheel = parts[2],
	           &fwing = parts[3], &rwing = parts[4], &helmet = parts[antialiasing?5:6];

	/// @todo: armar la matriz de transformación de cada parte para construir el auto
	
	// chasis
	float fix = 0.15f;
	glm::mat4 m_body ( 1.f, 0.0f, 0.0f, 0.0f, // nuevo eje x
					  0.0f, 1.f, 0.0f, 0.0f, // nuevo eje y
					  0.0f, 0.0f, 1.f, 0.0f, // nuevo eje z
					  0.f, fix, 0.f, 1.0f ); // desplazamiento
	
	// wheels 
	
	float front_wheel_size = 0.15f;
	float rear_wheel_size = 0.15f;
	float wheel_distance = 0.3f;
	
	glm::mat4 frw( front_wheel_size, 0.0f, 0.0f, 0.0f, // nuevo eje x
				0.0f, front_wheel_size, 0.0f, 0.0f, // nuevo eje y
				0.0f, 0.0f, front_wheel_size, 0.0f, // nuevo eje z
				0.5f, fix, wheel_distance, 1.0f ); // desplazamiento
	
	glm::mat4 flw( front_wheel_size, 0.0f, 0.0f, 0.0f, 
				  0.0f, front_wheel_size, 0.0f, 0.0f, 
				  0.0f, 0.0f, front_wheel_size, 0.0f, 
				  0.5f, fix, -wheel_distance, 1.0f );
	
	glm::mat4 rrw( rear_wheel_size, 0.0f, 0.0f, 0.0f, 
				  0.0f, rear_wheel_size, 0.0f, 0.0f, 
				  0.0f, 0.0f, rear_wheel_size, 0.0f, 
				  -0.9f, fix, wheel_distance, 1.0f );
	
	glm::mat4 rlw( rear_wheel_size, 0.0f, 0.0f, 0.0f, 
				  0.0f, rear_wheel_size, 0.0f, 0.0f, 
				  0.0f, 0.0f, rear_wheel_size, 0.0f, 
				  -0.9f, fix, -wheel_distance, 1.0f );
	
	
	// front wing
	float alpha = -1.57079633; // -pi/2
	float front_wing_size = 0.420f;
	float front_wing_height = -0.05f;
	
	glm::mat4 fw ( front_wing_size*cos(alpha), 0.0f, front_wing_size*(-sin(alpha)), 0.0f, 
				  0.0f, front_wing_size, 0.0f, 0.0f, 
				  front_wing_size*sin(alpha), 0.0f, front_wing_size*cos(alpha), 0.0f, 
				  0.82f, front_wing_height + fix, 0.0f, 1.0f );
	
	//rear wing 
	float rear_wing_size = 0.25f;
	float rear_wing_height = 0.1f;
	
	glm::mat4 rw ( rear_wing_size*cos(alpha), 0.0f, rear_wing_size*(-sin(alpha)), 0.0f, 
				  0.0f, rear_wing_size, 0.0f, 0.0f, 
				  rear_wing_size*sin(alpha), 0.0f, rear_wing_size*cos(alpha), 0.0f, 
				  -1.1f, rear_wing_height + fix, 0.0f, 1.0f );
	
	//driver
	float driver_size = 0.08f;
	float driver_height = 0.1f;
	glm::mat4 d( driver_size*cos(alpha), 0.0f, driver_size*(-sin(alpha)), 0.0f, 
				  0.0f, driver_size, 0.0f, 0.0f, 
				  driver_size*sin(alpha), 0.0f, driver_size*cos(alpha), 0.0f, 
				  -0.05f, driver_height + fix , 0.0f, 1.0f );
//	
//	//halo gg
//	float beta = 3.14159265;
//	float halo_size = 0.25f;
//	float halo_height = 0.05f + fix;
//	
//	glm::mat4 h0 ( halo_size, 0.0f, 0.0f, 0.0f, 
//				  0.0f, halo_size, 0.0f, 0.0f, 
//				  0.0f, 0.0f, halo_size, 0.0f, 
//				  0.05f, halo_height, 0.0f, 1.0f );
//	
//	glm::mat4 h1 ( cos(beta+alpha), 0.0f, -sin(beta+alpha), 0.0f, 
//				  0.0f, 1.0f, 0.0f, 0.0f, 
//				  sin(beta+alpha), 0.0f, cos(beta+alpha), 0.0f, 
//				  0.05f, halo_height, 0.0f, 1.0f );
//	
//	glm::mat4 h2 ( cos(beta), -sin(beta), 0.0f, 0.0f, 
//				  sin(beta), cos(beta), 0.0f, 0.0f, 
//				  0.0f, 0.0f, 1.0f, 0.0f, 
//				  0.05f, halo_height, 0.0f, 1.0f );
//	
//
//	glm::mat4 h = h0*h1*h2;
//	
//	float h_size = 0.15f;
//	float hx_height = 0.17f + fix;
//	
//	glm::mat4 hx ( h_size*cos(alpha), 0.0f, h_size*(-sin(alpha)), 0.0f, 
//				  0.0f, h_size, 0.0f, 0.0f, 
//				  h_size*sin(alpha), 0.0f, h_size*cos(alpha), 0.0f, 
//				  0.05f, hx_height, 0.0f, 1.0f );

	
	//wheels movement
//	//steering
	glm::mat4 turn ( cos(-car.rang1), 0.0f, -sin(-car.rang1), 0.0f, 
				  0.0f, 1.0f, 0.0f, 0.0f, 
				  sin(-car.rang1), 0.0f, cos(-car.rang1), 0.0f, 
				  0.0f, 0.0f, 0.0f, 1.0f );
	frw = frw * turn;
	flw = flw * turn;
//	
//	frw = frw * rotate(glm::mat4(1.f),car.rang1, glm::vec3(0.f,-1.f,0.f));
//	flw = flw * rotate(glm::mat4(1.f),car.rang1, glm::vec3(0.f,-1.f,0.f));
//	
	//roll
	glm::mat4 roll ( cos(car.rang2), -sin(car.rang2),0.0f , 0.0f, 
					sin(car.rang2), cos(car.rang2), 0.0f, 0.0f, 
					0.0f, 0.0f, 1.0f, 0.0f, 
					0.0f, 0.0f, 0.0f, 1.0f );
	
	frw = frw * roll;
	flw = flw * roll;
	rrw = rrw * roll;
	rlw = rlw * roll;
	
	if (body.show or play) {
		renderPart(car,body.models, m_body,shader);
	}
	
	if (wheel.show or play) {
		renderPart(car,wheel.models,frw,shader); // front right
	}
	
	if (wheel.show or play) {
		renderPart(car,wheel.models,flw,shader); // front left
	}
	
	if (wheel.show or play) {
		renderPart(car,wheel.models,rrw,shader); // rear right
	}
	
	if (wheel.show or play) {
		renderPart(car,wheel.models,rlw,shader); // rear left
	}
	
	if (fwing.show or play) {
		renderPart(car,fwing.models, fw,shader); //front wing
	}
	
	if (rwing.show or play) {
		float scl = 0.30f;
		renderPart(car,rwing.models,rw,shader); //rear wing
	}
//	
//	if (rwing.show or play) {
//		float scl = 0.30f;
//		renderPart(car,rwing.models,h,shader); // halo
//	}
//	
//	if (fwing.show or play) {
//		renderPart(car,fwing.models, hx,shader); //halo
//	}
//	
	if (helmet.show or play) {
		renderPart(car,helmet.models, d,shader); //driver
	}
	
	if (axis.show and (not play)) renderPart(car,axis.models,glm::mat4(1.f),shader);
}

// función que renderiza la pista
void renderTrack() {
	static Model track = Model::loadSingle("models/track",Model::fDontFit);
	static Shader shader("shaders/texture");
	shader.use();
	shader.setMatrixes(glm::mat4(1.f),view_matrix,projection_matrix);
	shader.setMaterial(track.material);
	shader.setBuffers(track.buffers);
	track.texture.bind();
	static float aniso = -1.0f;
	if (aniso<0) glGetFloatv(GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, &aniso);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, aniso);
	glPolygonMode(GL_FRONT_AND_BACK,GL_FILL);
	track.buffers.draw();
}

void renderShadow(const Car &car, const std::vector<Part> &parts) {
	static Shader shader_shadow("shaders/shadow");
	glEnable(GL_STENCIL_TEST); glClear(GL_STENCIL_BUFFER_BIT);
	glStencilFunc(GL_EQUAL,0,~0); glStencilOp(GL_KEEP,GL_KEEP,GL_INCR);
	renderCar(car,parts,shader_shadow);
	glDisable(GL_STENCIL_TEST);
}
