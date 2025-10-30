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
	Plane(const Plane_& _plane, const std::vector<Vec3f_>& _vertex_data);
	bool hit(const Ray& ray, const Interval& interval, HitRecord& rec) const;
	int id;
	int material_id;
	Vec3 point;
	Vec3 normal;
};
#endif // PLANE_H