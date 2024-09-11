#define GLM_ENABLE_EXPERIMENTAL
#include <glm/glm.hpp>
#include <glm/ext.hpp>
#include <glm/gtx/string_cast.hpp>
#include "utils.hpp"
#include "Debug.hpp"

BoundingBox::BoundingBox(glm::vec3 &p1, glm::vec3 &p2) 
	: pmin({std::min(p1.x,p2.x),std::min(p1.y,p2.y),std::min(p1.z,p2.z)}),
      pmax({std::max(p1.x,p2.x),std::max(p1.y,p2.y),std::max(p1.z,p2.z)}) 
{
	
}
	
bool BoundingBox::contiene(glm::vec3 &p) const {
	return p.x>=pmin.x && p.x<=pmax.x &&
		p.y>=pmin.y && p.y<=pmax.y &&
		p.z>=pmin.z && p.z<=pmax.z;
}

Pesos calcularPesos(glm::vec3 x0, glm::vec3 x1, glm::vec3 x2, glm::vec3 x) {
	/// @todo: implementar
	
	//Area total.
	glm::vec3 at = cross((x1 - x0),(x2 - x0));
	float at2 = dot(at,at); //at�
	
	//Calculo a0
	glm::vec3 a0 = cross((x1 - x),(x2 - x));
	float alpha0 = dot(a0,at)/at2;
	
	//Calculo a1
	glm::vec3 a1 = cross((x2 - x),(x0 - x));
	float alpha1 = dot(a1,at)/at2;
	
	//Calculo a2
	glm::vec3 a2 = cross((x0 - x),(x1 - x));
	float alpha2 = dot(a2,at)/at2;
	
	//debug
//	std::cout<<dot(a1,at)<<"/"<<at2<<std::endl;
//	std::cout<<glm::to_string(a1)<< " "<<glm::to_string(at)<<std::endl;
	
//	cg_error("debe implementar la funcion calcularPesos (utils.cpp)");
	return {alpha0,alpha1,alpha2};
}


