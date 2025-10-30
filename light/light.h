#ifndef LIGHT_H
#define LIGHT_H

#include <vector>
#include "../include/vec3.h"
#include "../include/color.h"

typedef struct PointLight {
	int id;
	Vec3 position;
	Color intensity;
}PointLight;

typedef struct LightSources {
	std::vector<PointLight> point_lights;
	Color ambient_light;
}LightSources;

#endif // LIGHT_H
