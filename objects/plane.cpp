#include "plane.h"

Plane::Plane()
		: id(-1), material_id(-1), point(Vec3(0, 0, 0)), normal(Vec3(0, 1, 0))
	{
}

Plane::Plane(const Plane_& _plane, const std::vector<Vec3f_>& _vertex_data)
	: id(_plane.id), material_id(_plane.material_id),
	point(Vec3(_vertex_data[_plane.point_vertex_id])),
	normal(Vec3(_plane.normal).normalize())
{
}
bool Plane::hit(const Ray& ray, const Interval& interval, HitRecord& rec) const
{
	double denom = normal.dot(ray.direction);
	if (std::abs(denom) > 1e-6)
	{
		Vec3 p0l0 = point - ray.origin;
		double t = p0l0.dot(normal) / denom;
		if (t >= interval.min && t <= interval.max)
		{
			rec.t = t;
			rec.point = ray.origin + ray.direction * t;
			rec.normal = normal;
			rec.material_id = material_id;
			rec.set_front_face(ray);
			return true;
		}
	}
	return false;
}
