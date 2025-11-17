#ifndef MESH_INSTANCE_H
#define MESH_INSTANCE_H

#include "../include/hittable.h"
#include "../include/parser.hpp"
#include "../objects/base_mesh.h"


class MeshInstance : public Hittable {
public:
	MeshInstance(const MeshInstance_& mesh_instance,
		const BaseMesh& base_mesh,
		const std::vector<Translation_>& translations,
		const std::vector<Scaling_>& scalings,
		const std::vector<Rotation_>& rotations,
		glm::mat4 base_transformation);
	MeshInstance(BaseMesh& base_mesh);
	~MeshInstance() = default;

	bool hit(const Ray& ray, Interval ray_t, HitRecord& rec) const override;
	AABB getAABB() const override;

	glm::mat4 composite_transformation_matrix;

private:	
	glm::mat4 calculateCompositeTransformationMatrix(
		const std::vector<Translation_>& translations,
		const std::vector<Scaling_>& scalings,
		const std::vector<Rotation_>& rotations);

	const BaseMesh& base_mesh;
	int id;
	int material_id;
	bool smooth_shading;
	bool reset_transform;
	std::vector<std::string> transformations;
};
#endif // !MESH_INSTANCE_H