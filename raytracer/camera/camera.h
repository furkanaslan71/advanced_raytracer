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
	Camera(
		const Camera_& cam,
		const std::vector<Translation_>& translations,
		const std::vector<Scaling_>& scalings,
		const std::vector<Rotation_>& rotations
	);
	~Camera();
	void render(IN const BaseRayTracer& rendering_technique,		
							OUT std::vector<std::vector<Color>>& image) const;

	glm::mat4 composite_transformation_matrix;

private:	
	Vec3 position, gaze, up;
	double near_plane[4]; // l, r, b, t
	double near_distance;
	int image_width, image_height;
	Vec3 w, u, v, q, su, sv, m;
	glm::mat4 calculateCompositeTransformationMatrix(
		const std::vector<Translation_>& translations,
		const std::vector<Scaling_>& scalings,
		const std::vector<Rotation_>& rotations);

	std::vector<std::string> transformations;

};

#endif //CAMERA_H
