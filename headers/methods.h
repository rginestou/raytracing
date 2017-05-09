#ifndef METHODS_H
#define METHODS_H

#include "scene.h"
#include <glm/glm.hpp>

typedef glm::vec4 color_t;

// > tracer
//		Main methods that calls all the raytracing functions sequencially
//		Return the color of the given ray's pixel
color_t tracer (Scene& scene, glm::vec3 ray, glm::vec3 origin);

// > intersectionFinder
//		Perform test over all elements in the scene to determin
//		if the ray passes through a face
void intersectionFinder (Scene& scene, glm::vec3 ray, glm::vec3 origin, int& faceId, size_t& elementId, glm::vec3& intersection, glm::vec3& normal);

// > shadowMapping
//		Perform test over all elements in the scene to determin
//		if a given point is shadowed by a face
bool shadowMapping (Scene& scene, int faceId, size_t elementId, glm::vec3 point);

color_t reflectionMapping (Scene& scene, int faceId, size_t elementId, glm::vec3 point, glm::vec3 incidentRay);

#endif
