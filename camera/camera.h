#ifndef CAMERA_H
#define CAMERA_H

#define OUT
#define IN


#include <vector>
#include "color.h"
#include "ray.h"
#include "vec3.h"
#include "bvh.h"
#include "parser.hpp"
#include "../render/rendering_technique.h"
#include <thread>
#include "../render/base_ray_tracer.h"


class Scene;
class RenderingTechnique;

class Camera {
public:
	int id;
	std::string image_name;
	Camera();
	Camera(const Camera_& cam);
	~Camera();
	void render(IN const BaseRayTracer& rendering_technique,		
							OUT std::vector<std::vector<Color>>& image) const;

private:	
	Vec3 position;
	Vec3 gaze;
	Vec3 up;
	double near_plane[4]; // l, r, b, t
	double near_distance;
	int image_width;
	int image_height;
	Vec3 w;
	Vec3 u;
	Vec3 v;
	Vec3 q;
	Vec3 su;
	Vec3 sv;
	Vec3 m;
};

#endif //CAMERA_H
