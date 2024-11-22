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
#include <iostream>
#include <utility>
#define VERSION 20241025

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
	const bool operator==(const Arista &a) const {
		return (n[0]==a.n[0]&&n[1]<a.n[1]);
	}
};

// Mapa sirve para guardar una asociación entre una arista y un indice de nodo (que no es de la arista)
using Mapa = std::map<Arista,int>;

void subdivide(SubDivMesh &mesh) {
	
	/// @@@@@: Implementar Catmull-Clark... lineamientos:
	//  Los nodos originales estan en las posiciones 0 a #n-1 de m.n,
	//  Los elementos orignales estan en las posiciones 0 a #e-1 de m.e
	
	int cant_nodos_originales = mesh.n.size();	///Cantidad nodos inicial
	
	//  1) Por cada elemento, agregar el centroide (nuevos nodos: #n a #n+#e-1)
	
	///Calculo los centroides de un elemento sumando los nodos de dicho Elemento 
	///y lo agrego al vector de nodos; el Elemento[i] tendra un nodo(centroide)[i+n] asociado 
	for(int i=0;i<mesh.e.size();i++){
		
		int cant_nodos = mesh.e[i].nv;
		
		glm::vec3 sum_nodos(0.f,0.f,0.f);
		for(int j=0;j<cant_nodos;j++){
			int ind_nodo = mesh.e[i][j];
			sum_nodos += mesh.n[ind_nodo].p;
		}
		
		glm::vec3 centroide_i = sum_nodos/(cant_nodos*1.f);
		mesh.n.push_back(Nodo(centroide_i));
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
	
	///Arista = dos indices de los nodos que la forman, ordenados de menor a mayor
	///Con el mapa se le asocia el nodo de arista correspondiente
	std::map<Arista,int> aristas;
	
	///Guardo cada arista (su par de nodos) y el nodo correspondiente a esta
	for(int i=0;i<mesh.e.size();i++){
		int cant_nodos_por_elemento = mesh.e[i].nv;
		for(int j=0;j<cant_nodos_por_elemento;j++){
			///Como los nodos de un elemento estan ordenados en sentido contrareloj, el nodo[j] del elemento[i] y el siguiente nodo[(j+1)%nv] del elemento[i] forman una arista
			int n1 = mesh.e[i].n[j];
			int n2 = mesh.e[i].n[(j+1)%cant_nodos_por_elemento];
			
			Arista arista_i(n1,n2);
			
			if(aristas.find(arista_i)==aristas.end()){	///Si nunca tuve en cuenta esta arista entonces calculo el nodo de arista correspondiente y agrego ambos a mi mapa de aristas
				
				glm::vec3 punto_medio = (mesh.n[n1].p + mesh.n[n2].p)/2.f;	///Si la arista es frontera, el nodo de arista es el promedio de los nodos que forman dicha arista (es decir el punto medio de la arista)
				
				if( mesh.e[i].v[j]!=-1){	///Pero si la arista no es frontera (si tiene elementos vecinos) es el promedio de los nodos que la forman y los centroides de los elementos que comparten la arista
												///La arista de los nodos n[j] y n[j+1] del elemento e[i] tiene un vecino e[i].v[j]
					
					glm::vec3 f1 = mesh.n[i+cant_nodos_originales].p;
					int ind_vecino = (mesh.e[i].v[j]);
					glm::vec3 f2 = mesh.n[ind_vecino+cant_nodos_originales].p;
					punto_medio = (punto_medio + (f1 + f2)/2.f)/2.f;
				}
				
				aristas.insert({arista_i,mesh.n.size()});
				mesh.n.push_back(Nodo(punto_medio));
				
			}
		}
	}
	
	//  3) Armar los elementos nuevos
	//      Los quads se dividen en 4, (uno reemplaza al original, los otros 3 se agregan)
	//      Los triangulos se dividen en 3, (uno reemplaza al original, los otros 2 se agregan)
	//      Para encontrar los nodos de las aristas usar el mapa que armaron en el paso 2
	//      Ordenar los nodos de todos los elementos nuevos con un mismo criterio (por ej, 
	//      siempre poner primero al centroide del elemento), para simplificar el paso 4.
	int cant_elementos = mesh.e.size();	///Cantidad elementos inicial
	
	///Armo los nuevos elementos (uno por cada nodo[j] del elemento[i] (excepto uno para modificar el original)) 	///Se recorre el vector de nodos del elemento y no el vector de nodos del mesh
	///Cada elemento guarda sus nodos de la manera (notar que mantiene el sentido de recorrido contrareloj): 
	///	n[0] = centroide del elemento[i] original
	///	n[1] = nodo de arista entre n[j] del elemento[i] y n[(j-1+nv)%nv] del elemento[i] (es decir del nodo j del elemento original y el anterior) 
	///	n[2] = nodo e[i].n[j]
	///	n[3] = nodo de arista entre n[j] del elemento[i ] y n[(j+1)%nv] del elemento[i] (es decir del nodo j del elemento original y el posterior)
	
	for(int i=0; i<cant_elementos;i++){
		
		int cant_nodos_por_elemento = mesh.e[i].nv;
		int ind_centroide = cant_nodos_originales+i;
		
		for(int j=0;j<cant_nodos_por_elemento-1;j++){
			
			int j_nodo = mesh.e[i].n[j];
			
			int n_a1 = mesh.e[i].n[ (j+1) % cant_nodos_por_elemento];
			int n_a2 = mesh.e[i].n[ (j-1+cant_nodos_por_elemento)  % cant_nodos_por_elemento];
			
			Arista a1(j_nodo,n_a1);
			
			Arista a2(j_nodo,n_a2);
			
			int c1 = aristas[a1];
			int c2 = aristas[a2];
			
			mesh.agregarElemento(ind_centroide,c2,j_nodo,c1);///Ademas de armar los elementos se encarga de actualizar el vector e de cada nodo n
			
		}
		
		int j_ult = mesh.e[i].n[cant_nodos_por_elemento-1];
		int n_a1 = mesh.e[i].n[0];
		int n_a2 = mesh.e[i].n[cant_nodos_por_elemento-2];
		
		Arista a1(j_ult,n_a1);
		Arista a2(j_ult,n_a2);
		
		int c1 = aristas[a1];
		int c2 = aristas[a2];
		
		mesh.reemplazarElemento(i,ind_centroide,c2,j_ult,c1);///Ademas de modificar el elemento se encarga de actualizar el vector e de cada nodo n
	}
	
	mesh.makeVecinos();///Actualiza el vector de vecinos de los elementos (vacio en los nuevos y erroneo en el actualizado) y el bool es_frontera de cada nodo
	
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
	// tips:
	//   no es necesario cambiar ni agregar nada fuera de este método, (con Mapa como 
	//     estructura auxiliar alcanza)

	///Calculo las posiciones de los nodos[0:n-1] del mesh
	for(int j=0;j<cant_nodos_originales;j++){
		
		int cant_elementos_por_nodo = mesh.n[j].e.size();
		
		///Calculo el promedio de los centroides de los elementos asociados al nodo (primer nodo (n[0]) del elemento)
		glm::vec3 f(0.f,0.f,0.f);
		for(int i=0;i<cant_elementos_por_nodo;i++){
			int ind_elemento = mesh.n[j].e[i];
			int ind_centroide = mesh.e[ind_elemento].n[0];
			f+= mesh.n[ind_centroide].p;
		}
		f=f/(cant_elementos_por_nodo*1.f);
		
		///Calculo el promedio de los nodos arista asociados al nodo original
		glm::vec3 r(0.f,0.f,0.f);
		std::vector<int> nodos_arista;
		
		///Busqueda
		for(int i=0;i<cant_elementos_por_nodo;i++){ 
			int ind_elemento = mesh.n[j].e[i];
			
			///Si el nodo es frontera: tengo en cuenta solo las aristas frontera que son las que tienen ambos nodos frontera (es decir si el nodo con el que forma la arista tambien es frontera)
			///Si no es frontera: tengo en cuenta todas las aristas asociadas al nodo
			
			int ind_arista_1 = mesh.e[ind_elemento].n[1]; 
			if(find(nodos_arista.begin(),nodos_arista.end(),ind_arista_1)==nodos_arista.end() and (mesh.n[ind_arista_1].es_frontera or !mesh.n[j].es_frontera))	nodos_arista.push_back(ind_arista_1);
			
			int ind_arista_2 = mesh.e[ind_elemento].n[3]; 
			if(find(nodos_arista.begin(),nodos_arista.end(),ind_arista_2)==nodos_arista.end() and (mesh.n[ind_arista_2].es_frontera or !mesh.n[j].es_frontera))	nodos_arista.push_back(ind_arista_2);
		}	
		
		///Calculo promedio nodos arista
		for(int i=0;i<(nodos_arista.size());i++){
			r+= mesh.n[nodos_arista[i]].p;
		};
		r=r/(nodos_arista.size()*1.f);
		
		if(mesh.n[j].es_frontera){
			mesh.n[j].p = (r+mesh.n[j].p)/2.f; ///Si es frontera calculo el promedio entre nodos arista y el nodo original
		}else {
			mesh.n[j].p = (4.f*r-f+(cant_elementos_por_nodo-3.f)*mesh.n[j].p)/(cant_elementos_por_nodo*1.f);///Si no es frontera calculo el promedio ponderado afin (si sumo los pesos y los dividido por la cant_elementos del nodo estos suman 1)
		}
	}
	
	
	// Esta llamada valida si la estructura de datos quedó consistente (si todos los
	// índices están dentro del rango válido, y si son correctas las relaciones
	// entre los .n de los elementos y los .e de los nodos). Mantener al final de
	// esta función para ver que la subdivisión implementada no rompa esos invariantes.
	
	mesh.verificarIntegridad();
	/// ve para cada nodo de un elemento, que el nodo tenga la ref al elemento en su e	
	/// y ve que cada elemento al que un nodo dice pertenecer, efectivamente lo contenga

}
