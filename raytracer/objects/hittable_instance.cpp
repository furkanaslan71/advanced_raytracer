#include "hittable_instance.h"

HittableInstance::HittableInstance(const Sphere& sphere)
	: base_hittable(sphere)
{
	composite_transformation_matrix = sphere.composite_transformation_matrix;
}

HittableInstance::HittableInstance(const Triangle& triangle)
	: base_hittable(triangle)
{
	composite_transformation_matrix = triangle.composite_transformation_matrix;
}

bool HittableInstance::hit(const Ray& ray, Interval ray_t, HitRecord& rec) const
{
	//inverse transform the ray
	if(getAABB().hit(ray, ray_t) == false)
		return false;

	const Vec3 world_origin = ray.origin;
	const Vec3 world_direction = ray.direction;
	glm::mat4 inv_transform = glm::inverse(composite_transformation_matrix);
	glm::vec4 local_origin = inv_transform * glm::vec4(world_origin.x, world_origin.y, world_origin.z, 1.0f);
	glm::vec4 local_direction = inv_transform * glm::vec4(world_direction.x, world_direction.y, world_direction.z, 0.0f);

	Ray local_ray(
		Vec3::glmVec3toVec3(glm::vec3(local_origin)),
		Vec3::glmVec3toVec3(glm::vec3(local_direction)).normalize()
	);

	Interval local_ray_t(ray_t.min, INFINITY);

	if (!base_hittable.hit(local_ray, local_ray_t, rec))
	{
		return false; // No hit, we're done.
	}

	glm::vec4 local_hit_point = glm::vec4(rec.point.x, rec.point.y, rec.point.z, 1.0f);
	glm::vec4 world_hit_point = composite_transformation_matrix * local_hit_point;
	rec.point = Vec3::glmVec3toVec3(glm::vec3(world_hit_point));
	// Transform normal
	glm::vec4 local_normal = glm::vec4(rec.normal.x, rec.normal.y, rec.normal.z, 0.0f);
	glm::mat4 inv_transpose = glm::transpose(inv_transform);
	glm::vec4 world_normal = inv_transpose * local_normal;
	rec.normal = Vec3::glmVec3toVec3(glm::vec3(world_normal)).normalize();

	double local_t = rec.t;
	double world_t = world_origin.distance(Vec3::glmVec3toVec3(world_hit_point));

	if(world_t < ray_t.min || world_t > ray_t.max)
		return false;

	rec.t = world_t;

	rec.set_front_face(ray);
	return true;
}

AABB HittableInstance::getAABB() const
{
	AABB box = base_hittable.getAABB();
	box = box.transformBox(composite_transformation_matrix);
	return box;
}