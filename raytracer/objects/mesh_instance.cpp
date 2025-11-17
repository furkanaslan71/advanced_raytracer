#include "mesh_instance.h"

MeshInstance::MeshInstance(const MeshInstance_& mesh_instance,
	const BaseMesh& base_mesh,
	const std::vector<Translation_>& translations,
	const std::vector<Scaling_>& scalings,
	const std::vector<Rotation_>& rotations,
	glm::mat4 base_transformation
)
	: id(mesh_instance.id),
		material_id(mesh_instance.material_id),
		smooth_shading(mesh_instance.smooth_shading),
		reset_transform(mesh_instance.reset_transform),
	transformations(mesh_instance.transformations),
	base_mesh(base_mesh)
{
	if(material_id == -1)
		material_id = base_mesh.material_id;

	if(reset_transform)
	{
		auto res = calculateCompositeTransformationMatrix(translations, scalings, rotations);
		composite_transformation_matrix = res;
	}
	else
	{
		//composite_transformation_matrix = base_mesh.composite_transformation_matrix;
		composite_transformation_matrix = base_transformation;
		auto res = calculateCompositeTransformationMatrix(translations, scalings, rotations);
		composite_transformation_matrix = res * composite_transformation_matrix;
	}
}
MeshInstance::MeshInstance(BaseMesh& base_mesh)
	: base_mesh(base_mesh)
	, material_id(base_mesh.material_id)
	, smooth_shading(base_mesh.smooth_shading)
	, id(base_mesh.id)
	, reset_transform(false)
{
	composite_transformation_matrix = base_mesh.composite_transformation_matrix;
}


bool MeshInstance::hit(const Ray& ray, Interval ray_t, HitRecord& rec) const
{
	//inverse transform the ray
	if (getAABB().hit(ray, ray_t) == false)
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

	Interval local_ray_t(1e-8, INFINITY);

	if (!base_mesh.hit(local_ray, local_ray_t, rec))
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

	if (world_t < ray_t.min || world_t > ray_t.max)
		return false;


	rec.t = world_t;

	rec.set_front_face(ray);
	rec.material_id = material_id;
	return true;
}

AABB MeshInstance::getAABB() const
{
	AABB box = base_mesh.getAABB();
	box = box.transformBox(composite_transformation_matrix);
	return box;
}

glm::mat4 MeshInstance::calculateCompositeTransformationMatrix(
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
