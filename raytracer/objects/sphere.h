#ifndef SPHERE_H
#define SPHERE_H

#include "../include/hittable.h"
#include "../include/parser.hpp"
#include "../include/aabb.h"


class Sphere : public Hittable {
public:
	Vec3 center;
	double radius;
	int material_id;

	Sphere(
		Vec3 _center, double _radius,
		int _material_id,
		const std::vector<std::string>& transformations,
		const std::vector<Translation_>& translations,
		const std::vector<Scaling_>& scalings,
		const std::vector<Rotation_>& rotations
		)
		: 
		center(_center), 
		radius(_radius), 
		material_id(_material_id),
		transformations(transformations)
	{
		bounding_box = AABB(center - Vec3(radius, radius, radius), 
														center + Vec3(radius, radius, radius));
		auto identity = glm::mat4(1.0f);
		composite_transformation_matrix = calculateCompositeTransformationMatrix(
			translations,
			scalings,
			rotations) * identity;
	}

	bool hit(const Ray& ray, Interval ray_t, HitRecord& rec) const override
	{

		Vec3 oc = ray.origin - center;
		double a = ray.direction.dot(ray.direction);
		double b = oc.dot(ray.direction);
		double c = oc.dot(oc) - radius * radius;
		double discriminant = b * b - a * c;

		// Intersection occurs
		if (discriminant >= 0)
		{
			double t1 = (-b - sqrt(discriminant)) / a;
			double t2 = (-b + sqrt(discriminant)) / a;

			if (t1 >= 0 || t2 >= 0)
			{
				double t = (t1 >= 0) ? t1 : t2;

				Vec3 hitPoint = ray.origin + ray.direction * t;
				Vec3 normal = (hitPoint - center).normalize();

				rec.t = t;
				rec.point = hitPoint;
				rec.normal = normal;
				rec.material_id = material_id;
				rec.set_front_face(ray);
				return true;
			}
		}
		return false;

	};
	AABB getAABB() const override
	{
		return bounding_box;
	}
	glm::mat4 composite_transformation_matrix;

private:
	glm::mat4 calculateCompositeTransformationMatrix(
		const std::vector<Translation_>& translations,
		const std::vector<Scaling_>& scalings,
		const std::vector<Rotation_>& rotations)
	{
		auto res = glm::mat4(1.0f);
		for (const auto& transform : transformations)
		{
			auto identity = glm::mat4(1.0f);
			switch (transform[0])
			{
			case 't': // translation
			{
				int index = std::stoi(transform.substr(1));
				auto& t = translations[index];
				glm::vec3 translation = glm::vec3(t.tx, t.ty, t.tz);
				res = glm::translate(identity, translation) * res;
				break;
			}
			case 's': // scaling
			{
				int index = std::stoi(transform.substr(1));
				auto& s = scalings[index];
				glm::vec3 scaling = glm::vec3(s.sx, s.sy, s.sz);
				res = glm::scale(identity, scaling) * res;
				break;
			}
			case 'r': // rotation
			{
				int index = std::stoi(transform.substr(1));
				auto& r = rotations[index];
				glm::vec3 axis = glm::vec3(r.axis_x, r.axis_y, r.axis_z);
				res = glm::rotate(identity, glm::radians(r.angle), axis) * res;
				break;
			}
			default:
				break;
			}
		}
		return res;
	}
	std::vector<std::string> transformations;
	AABB bounding_box;
};

#endif // SPHERE_H