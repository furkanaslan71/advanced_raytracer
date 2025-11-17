#ifndef PLANE_H
#define PLANE_H


#include "../include/parser.hpp"
#include "../include/vec3.h"
#include "../include/ray.h"
#include "../include/hittable.h"
#include "../include/interval.h"

class Plane {
public:
	Plane();
	Plane(
		const Plane_& _plane,
		const std::vector<Vec3f_>& _vertex_data,
		const std::vector<std::string>& transformations,
		const std::vector<Translation_>& translations,
		const std::vector<Scaling_>& scalings,
		const std::vector<Rotation_>& rotations
	);
	bool hit(const Ray& ray, const Interval& interval, HitRecord& rec) const;
	int id;
	int material_id;
	Vec3 point;
	Vec3 normal;
	glm::mat4 composite_transformation_matrix;
private:
	glm::mat4 calculateCompositeTransformationMatrix(
		const std::vector<Translation_>& translations,
		const std::vector<Scaling_>& scalings,
		const std::vector<Rotation_>& rotations) const;

	std::vector<std::string> transformations;
};
#endif // PLANE_H