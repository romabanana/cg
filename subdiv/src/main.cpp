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
#include "SubDivMesh.hpp"
#include "SubDivMeshRenderer.hpp"

#define VERSION 20241025
#include <iostream>
//using namespace std;

// models and settings
std::vector<std::string> models_names = { "cubo", "icosahedron", "plano", "suzanne", "star" };
int current_model = 0;
bool fill = true, nodes = true, wireframe = true, smooth = false, 
	 reload_mesh = true, mesh_modified = false;

// extraa callbacks
void keyboardCallback(GLFWwindow* glfw_win, int key, int scancode, int action, int mods);

SubDivMesh mesh;
void subdivide(SubDivMesh &mesh);

int main() {
	
	// initialize window and setup callbacks
	Window window(win_width,win_height,"CG Demo");
	setCommonCallbacks(window);
	glfwSetKeyCallback(window, keyboardCallback);
	view_fov = 60.f;
	
	// setup OpenGL state and load shaders
	glEnable(GL_DEPTH_TEST); glDepthFunc(GL_LESS); 
	glEnable(GL_BLEND); glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
	glClearColor(0.8f,0.8f,0.9f,1.f);
	Shader shader_flat("shaders/flat"),
	       shader_smooth("shaders/smooth"),
		   shader_wireframe("shaders/wireframe");
	SubDivMeshRenderer renderer;
	
	// main loop
	Material material;
	material.ka = material.kd = glm::vec3{.8f,.4f,.4f};
	material.ks = glm::vec3{.5f,.5f,.5f};
	material.shininess = 50.f;
	
	FrameTimer timer;
	do {
		
		glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
		
		if (reload_mesh) {
			mesh = SubDivMesh("models/"+models_names[current_model]+".dat");
			reload_mesh = false; mesh_modified = true;
		}
		if (mesh_modified) {
			renderer = makeRenderer(mesh,false);
			mesh_modified = false;
		}
		
		if (nodes) {
			shader_wireframe.use();
			setMatrixes(shader_wireframe);
			renderer.drawPoints(shader_wireframe);
		}
		
		if (wireframe) {
			shader_wireframe.use();
			setMatrixes(shader_wireframe);
			renderer.drawLines(shader_wireframe);
		}
		
		if (fill) {
			Shader &shader = smooth ? shader_smooth : shader_flat;
			shader.use();
			setMatrixes(shader);
			shader.setLight(glm::vec4{2.f,1.f,5.f,0.f}, glm::vec3{1.f,1.f,1.f}, 0.25f);
			shader.setMaterial(material);
			renderer.drawTriangles(shader);
		}
		
		// settings sub-window
		window.ImGuiDialog("CG Example",[&](){
			if (ImGui::Combo(".dat (O)", &current_model,models_names)) reload_mesh = true;
			ImGui::Checkbox("Fill (F)",&fill);
			ImGui::Checkbox("Wireframe (W)",&wireframe);
			ImGui::Checkbox("Nodes (N)",&nodes);
			ImGui::Checkbox("Smooth Shading (S)",&smooth);
			if (ImGui::Button("Subdivide (D)")) { subdivide(mesh); mesh_modified = true; }
			if (ImGui::Button("Reset (R)")) reload_mesh = true;
			ImGui::Text("Nodes: %i, Elements: %i",mesh.n.size(),mesh.e.size());
		});
		
		// finish frame
		window.finishFrame();
		
	} while( glfwGetKey(window,GLFW_KEY_ESCAPE)!=GLFW_PRESS && !glfwWindowShouldClose(window) );
}

void keyboardCallback(GLFWwindow* glfw_win, int key, int scancode, int action, int mods) {
	if (action==GLFW_PRESS) {
		switch (key) {
		case 'D': subdivide(mesh); mesh_modified = true; break;
		case 'F': fill = !fill; break;
		case 'N': nodes = !nodes; break;
		case 'W': wireframe = !wireframe; break;
		case 'S': smooth = !smooth; break;
		case 'R': reload_mesh=true; break;
		case 'O': case 'M': current_model = (current_model+1)%models_names.size(); reload_mesh = true; break;
		}
	}
}

// La struct Arista guarda los dos indices de nodos de una arista
// Siempre pone primero el menor indice, para facilitar la búsqueda en lista ordenada;
//    es para usar con el Mapa de más abajo, para asociar un nodo nuevo a una arista vieja
struct Arista {
	int n[2];
	Arista(int n1, int n2) {
		n[0]=n1; n[1]=n2;
		if (n[0]>n[1]) std::swap(n[0],n[1]);
	}
	Arista(Elemento &e, int i) { // i-esima arista de un elemento
		n[0]=e[i]; n[1]=e[i+1];
		if (n[0]>n[1]) std::swap(n[0],n[1]); // pierde el orden del elemento
	}
	const bool operator<(const Arista &a) const {
		return (n[0]<a.n[0]||(n[0]==a.n[0]&&n[1]<a.n[1]));
	}
};

// Mapa sirve para guardar una asociación entre una arista y un indice de nodo (que no es de la arista)
using Mapa = std::map<Arista,int>;

void subdivide(SubDivMesh &mesh) {
	
	/// @@@@@: Implementar Catmull-Clark... lineamientos:
	//  Los nodos originales estan en las posiciones 0 a #n-1 de m.n,
	//  Los elementos orignales estan en las posiciones 0 a #e-1 de m.e
	//  1) Por cada elemento, agregar el centroide (nuevos nodos: #n a #n+#e-1)
//	std::vector<Elemento> element_vector = mesh.e;
//	std::vector<Nodo> node_vector = mesh.n;
//	int original_size = node_vector.size();

	int size_original = mesh.n.size();
	
	glm::vec3 p = glm::vec3 (0,0,0);
	Nodo nuevo_nodo = Nodo(p);
	for(Elemento elem : mesh.e){
		p = glm::vec3 (0,0,0);
		int cant = elem.nv;
		for(size_t i = 0; i < cant; i++){
			p += mesh.n[elem.n[i]].p;
		}
		p = p/float(cant);
		nuevo_nodo = Nodo(p);
		mesh.n.push_back(nuevo_nodo);
	}
	
	//  2) Por cada arista de cada cara, agregar un pto en el medio que es
	//      promedio de los vertices de la arista y los centroides de las caras 
	//      adyacentes. Aca hay que usar los elementos vecinos.
	//      En los bordes, cuando no hay vecinos, es simplemente el promedio de los 
	//      vertices de la arista
	//      Hay que evitar procesar dos veces la misma arista (como?)
	//      Mas adelante vamos a necesitar determinar cual punto agregamos en cada
	//      arista, y ya que no se pueden relacionar los indices con una formula simple
	//      se sugiere usar Mapa como estructura auxiliar
	
	Mapa mapita;
	p = glm::vec3 (0,0,0);
	Nodo nodo_arista = Nodo(p);
	int i_aristas = 0;
	int i_elemento = 0;
	for(Elemento elem: mesh.e){
		int nodos = elem.nv;
		for(size_t i = 0; i < nodos; i++){
//			std::cout<<i<<std::endl;
			///chequear que nos se haya recorrido la arista
			glm::vec3 p = glm::vec3 (0,0,0);
			int n1 = elem.n[i];
			int n2 = elem.n[(i+1)%elem.nv];
//			if(n2 == -1) n2 = 0; ///why n2 = -1?
			Arista a(n1,n2);
//			std::cout<<"here: "<<n1<<" "<<n2<<std::endl;
			
			if(mapita.find(a)== mapita.end()){
//				if(n1 == 3) std::cout<<"here";
				
				///Agregar arista al mapa
				mapita[a]=i_aristas; 
//				mapita[a]=n2; 
				i_aristas++; 
//				std::cout<<n1<<" ,"<<n2<<" indice: "<<i_aristas<<std::endl;
				
				p = mesh.n[n1].p + mesh.n[n2].p;
				p = p*0.5f;
				
				///verificar frontera;
				if(elem.v[i] != -1){ // no frontera
					glm::vec3 centroide = glm::vec3 (0,0,0);
					glm::vec3 centroide_vecino = glm::vec3 (0,0,0);
					
					centroide = mesh.n[size_original+i_elemento].p;
					centroide = centroide/float(nodos);
					
					
					centroide_vecino = mesh.n[size_original+elem.v[i]].p;
					centroide_vecino = centroide_vecino/float(mesh.e[elem.v[i]].nv);
					
					p += centroide + centroide_vecino;
					p = p*0.5f;
					
					}
				
				nuevo_nodo = Nodo(p);
				mesh.n.push_back(nuevo_nodo);
				}
			}
			i_elemento++;
		}
	
//	for (const auto& entry : mapita) {
//		std::cout << "Key: (" << entry.first.n[0] << ", " << entry.first.n[1] << ")" << std::endl;
//	}

	
	//  3) Armar los elementos nuevos
	//      Los quads se dividen en 4, (uno reemplaza al original, los otros 3 se agregan)
	//      Los triangulos se dividen en 3, (uno reemplaza al original, los otros 2 se agregan)
	//      Para encontrar los nodos de las aristas usar el mapa que armaron en el paso 2
	//      Ordenar los nodos de todos los elementos nuevos con un mismo criterio (por ej, 
	//      siempre poner primero al centroide del elemento), para simplificar el paso 4.

	
	int elementos = mesh.e.size();
	
	int k = 1;
	for(size_t i = 0; i<elementos; i++){
		// un elemente se construye(n1,n2,n3,n4);
		int nodos = mesh.e[i].nv;
		int n0 = size_original+i; //centroide
		
		for(size_t j = 0; j<nodos; j++){
			
			int n2 = mesh.e[i].n[j]; // Nodo original
			int next = mesh.e[i].n[(j+1)%nodos]; // Siguiente en el elemento
			int prev = mesh.e[i].n[(j-1+nodos)%nodos]; // previo en el elemento
			
			int i1 = mapita[Arista(n2, prev)]; // indice asociado a arista previa
			int i3 = mapita[Arista(n2, next)]; // indice asociado a arista siguiente
			
			int n1 = size_original+elementos+i1; // indice de nodo1
			int n3 = size_original+elementos+i3; // indice de nodo3
			
			if(j==nodos-1){
				mesh.reemplazarElemento(i,n0,n1,n2,n3);
			}else{
				mesh.agregarElemento(n0,n1,n2,n3);
				//n0 : centroide
				//n1 : n. arista ste.
				//n2 : n. original.
				//n3 : n. arista pre.
			}
			
		}
		
	}
	mesh.makeVecinos();
	
	//  4) Calcular las nuevas posiciones de los nodos originales
	//      Para nodos interiores: (4r-f+(n-3)p)/n
	//         f=promedio de nodos interiores de las caras (los agregados en el paso 1)
	//         r=promedio de los pts medios de las aristas (los agregados en el paso 2)
	//         p=posicion del nodo original
	//         n=cantidad de elementos para ese nodo
	//      Para nodos del borde: (r+p)/2
	//         r=promedio de los dos pts medios de las aristas
	//         p=posicion del nodo original
	//      Ojo: en el paso 3 cambio toda la SubDivMesh, analizar donde quedan en los nuevos 
	//      elementos (¿de que tipo son?) los nodos de las caras y los de las aristas 
	//      que se agregaron antes.
	
	std::vector<int> visitados;
	for(size_t i = 0; i<size_original; i++){
		std::cout<<"idx :"<<i+1<<"/"<<size_original<<std::endl;
	
		//f:promedio de centroides
		//f:promedio de pts asociados a ristas
		glm::vec3 f = glm::vec3(0,0,0);
		glm::vec3 r = glm::vec3(0,0,0);
		glm::vec3 node = glm::vec3(0,0,0);
		glm::vec3 nodo_original = mesh.n[i].p;
		
		int cant_asociados = mesh.n[i].e.size();//n
//		std::cout<<cant_asociados<<std::endl;
		for(size_t j = 0; j<cant_asociados; j++){
			int idx_e = mesh.n[i].e[j]; //indice del elem
			int idx_c = mesh.e[idx_e].n[0]; //indice centroide
			f += mesh.n[idx_c].p; //centroide
		}
		f = f/float(cant_asociados);
		
		int cant_aristas = 0;
		
		for(size_t j = 0; j<cant_asociados; j++){
			int idx_e = mesh.n[i].e[j]; //indice del elem
			
			int n1 = mesh.e[idx_e].n[1];
			int n3 = mesh.e[idx_e].n[3];
			std::cout<<"n1: "<<n1<<", n3:"<<n3<<std::endl;
			if(std::find(visitados.begin(),visitados.end(),n1)==visitados.end()){
					visitados.push_back(n1);
					r += mesh.n[n1].p;
					cant_aristas++;
			}
						
			if(std::find(visitados.begin(),visitados.end(),n3)==visitados.end()){
					visitados.push_back(n3);
					r += mesh.n[n3].p;
					cant_aristas++;
			}
		}
		std::cout<< cant_aristas<<std::endl;
		r = r/float(cant_aristas);
		visitados.clear();
		std::cout << "r: " << r.x << ", " << r.y << ", " << r.z << std::endl;
		std::cout << "nodo_original: " << nodo_original.x << ", " << nodo_original.y << ", " << nodo_original.z << std::endl;
		
		if(mesh.n[i].es_frontera){
			mesh.n[i].p = (r+nodo_original)/2.f;
		}else{
			mesh.n[i].p = ((4.f*r) - f + float((cant_asociados-3))*nodo_original)/float(cant_asociados);
		}
//		mesh.n[i] = node;
		
	}
		
	
	// tips:
	//   no es necesario cambiar ni agregar nada fuera de este método, (con Mapa como 
	//     estructura auxiliar alcanza)
	//   sugerencia: probar primero usando el cubo (es cerrado y solo tiene quads)
	//               despues usando la piramide (tambien cerrada, y solo triangulos)
	//               despues el ejemplo plano (para ver que pasa en los bordes)
	//               finalmente el mono (tiene mezcla y elementos sin vecinos)
	//   repaso de como usar un mapa:
	//     para asociar un indice (i) de nodo a una arista (n1-n2): elmapa[Arista(n1,n2)]=i;
	//     para saber si hay un indice asociado a una arista:  ¿elmapa.find(Arista(n1,n2))!=elmapa.end()?
	//     para recuperar el indice (en j) asociado a una arista: int j=elmapa[Arista(n1,n2)];
	
	
	// Esta llamada valida si la estructura de datos quedó consistente (si todos los
	// índices están dentro del rango válido, y si son correctas las relaciones
	// entre los .n de los elementos y los .e de los nodos). Mantener al final de
	// esta función para ver que la subdivisión implementada no rompa esos invariantes.
	mesh.verificarIntegridad();
}
