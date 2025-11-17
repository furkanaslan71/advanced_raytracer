#ifndef HITTABLE_INSTANCE_H
#define HITTABLE_INSTANCE_H

#include "../include/hittable.h"
#include "../include/parser.hpp"
#include "../include/aabb.h"
#include "sphere.h"
#include "triangle.h"

class HittableInstance : public Hittable {
	public:
		HittableInstance(const Sphere& sphere);
		HittableInstance(const Triangle& triangle);
		~HittableInstance() = default;
		bool hit(const Ray& ray, Interval ray_t, HitRecord& rec) const override;
		AABB getAABB() const override;
		glm::mat4 composite_transformation_matrix;
		const Hittable& base_hittable;
};


#endif // !HITTABLE_INSTANCE_H
