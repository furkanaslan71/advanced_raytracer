#include "base_mesh.h"
#include "../objects/triangle.h"
#include "mesh_instance.h"

#if INSTANCING
double getAreaTriangle(Vec3 v1, Vec3 v2, Vec3 v3)
{
	Vec3 edge1 = v2 - v1;
	Vec3 edge2 = v3 - v1;
	Vec3 cross_product = edge1.cross(edge2);
	double area = 0.5 * cross_product.length();
	return area;
}
#endif

BaseMesh::BaseMesh(const Mesh_& raw_mesh,
	const std::vector<Vec3f_>& vertex_data,
	std::vector<Translation_>& translations,
	std::vector<Scaling_>& scalings,
	std::vector<Rotation_>& rotations)
	: id(raw_mesh.id),
		material_id(raw_mesh.material_id),
		smooth_shading(raw_mesh.smooth_shading),
		transformations(raw_mesh.transformations)
{
	auto res = calculateCompositeTransformationMatrix(translations, 
		scalings, rotations);
	composite_transformation_matrix = res;
	// Create Triangle faces
	if (raw_mesh.smooth_shading)
	{
		std::vector<std::vector<std::pair<Vec3, double>>> per_vertex_triangles; // pair<triangle_normal, area> for each vertex
		per_vertex_triangles.resize(vertex_data.size());
		for (const Triangle_& raw_triangle : raw_mesh.faces)
		{
			Vec3 v0 = Vec3(vertex_data[raw_triangle.v0_id]);
			Vec3 v1 = Vec3(vertex_data[raw_triangle.v1_id]);
			Vec3 v2 = Vec3(vertex_data[raw_triangle.v2_id]);
			double area = getAreaTriangle(v0, v1, v2);
			Vec3 edge1 = v1 - v0;
			Vec3 edge2 = v2 - v0;
			Vec3 face_normal = edge1.cross(edge2).normalize();
			per_vertex_triangles[raw_triangle.v0_id].push_back(
				std::make_pair(face_normal, area)
			);
			per_vertex_triangles[raw_triangle.v1_id].push_back(
				std::make_pair(face_normal, area)
			);
			per_vertex_triangles[raw_triangle.v2_id].push_back(
				std::make_pair(face_normal, area)
			);
		}
		std::vector<Vec3> vertex_normals;
		for (const auto& v : per_vertex_triangles)
		{
			Vec3 normal(0.0, 0.0, 0.0);
			double total_area = 0.0;
			for (const auto& pair : v)
			{
				normal = normal + pair.first * pair.second;
				total_area += pair.second;
			}
			if (total_area > 0.0)
			{
				normal = normal / total_area;
				normal.normalize();
			}
			vertex_normals.push_back(normal);
		}
		for (const Triangle_& raw_triangle : raw_mesh.faces)
		{
			Vec3 indices[3] = { vertex_data[raw_triangle.v0_id],
				vertex_data[raw_triangle.v1_id],
				vertex_data[raw_triangle.v2_id] };

			Vec3 per_vertex_normals[3] = {
				vertex_normals[raw_triangle.v0_id],
				vertex_normals[raw_triangle.v1_id],
				vertex_normals[raw_triangle.v2_id] };

			faces.push_back(std::make_shared<Triangle>(
					indices, 
					raw_triangle.material_id, 
					per_vertex_normals,
					raw_triangle.transformations,
					translations,
					scalings,
					rotations
				)
			);
		}

	}
	else
	{
		for (const Triangle_& raw_triangle : raw_mesh.faces)
		{
			Vec3 indices[3] = { vertex_data[raw_triangle.v0_id],
				vertex_data[raw_triangle.v1_id],
				vertex_data[raw_triangle.v2_id] };

			faces.push_back(std::make_shared<Triangle>(
					indices, 
					raw_triangle.material_id,
					raw_triangle.transformations,
					translations,
					scalings,
					rotations
				)
			);
		}
	}
	bvh_root = BvhNode(faces, 0, static_cast<int>(faces.size() - 1));
}


bool BaseMesh::hit(const Ray& ray, Interval ray_t, HitRecord& rec) const
{
	return bvh_root.hit(ray, ray_t, rec);
}

AABB BaseMesh::getAABB() const
{
	return bvh_root.getAABB();
}

glm::mat4 BaseMesh::calculateCompositeTransformationMatrix(
	std::vector<Translation_>& translations,
	std::vector<Scaling_>& scalings,
	std::vector<Rotation_>& rotations)
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