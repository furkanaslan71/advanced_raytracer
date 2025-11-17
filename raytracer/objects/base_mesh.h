#ifndef BASE_MESH_H
#define BASE_MESH_H

#include "../include/hittable.h"
#include "../include/parser.hpp"
#include "../include/aabb.h"
#include "../include/bvh.h"
#include "../include/config.h"


class BaseMesh : public Hittable {
public:
	BaseMesh(const Mesh_& raw_mesh,
		const std::vector<Vec3f_>& vertex_data,
		std::vector<Translation_>& translations,
		std::vector<Scaling_>& scalings,
		std::vector<Rotation_>& rotations);
	~BaseMesh() = default;

	bool hit(const Ray& ray, Interval ray_t, HitRecord& rec) const override;
	AABB getAABB() const override;

	int id;
	int material_id;
	bool smooth_shading;
	glm::mat4 composite_transformation_matrix;

private:
	glm::mat4 calculateCompositeTransformationMatrix(
		std::vector<Translation_>& translations,
		std::vector<Scaling_>& scalings,
		std::vector<Rotation_>& rotations);

	
	std::vector<std::string> transformations;
	std::vector<std::shared_ptr<Hittable>> faces;
	BvhNode bvh_root;
};

#endif // !BASE_MESH_H