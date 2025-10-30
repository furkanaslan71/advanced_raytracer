#include <iostream>
#include "../include/parser.hpp"
#include "../render/render_manager.h"
#include "../render/base_ray_tracer.h"
#include "../material/material_manager.h"
#include "../scene/scene.h"
#include "../objects/sphere.h"
#include "../objects/triangle.h"
#include "../objects/plane.h"

double getAreaTriangle(Vec3 v1, Vec3 v2, Vec3 v3)
{
  Vec3 edge1 = v2 - v1;
  Vec3 edge2 = v3 - v1;
  Vec3 cross_product = edge1.cross(edge2);
  double area = 0.5 * cross_product.length();
  return area;
}

constexpr auto BACKFACE_CULLING = false;

int main(int argc, char* argv[])
{
  // Expect exactly one argument after the executable name
  if (argc != 2)
  {
    std::cerr << "Usage: " << argv[0] << " <scene_file.json>" << std::endl;
    return 1;
  }

  std::string scene_filename = argv[1];

  //std::string scene_filename = "D:/Furkan/repos/raytracer/HelixNebula/inputs/other_dragon.json";

  Scene_ raw_scene;
  
  std::cout << "Parsing scene file: " << scene_filename << std::endl;
  parseScene(scene_filename, raw_scene);

  //printSceneSummary(scene);
  //printScene(raw_scene);

	std::vector<Plane> planes;

	std::vector<std::shared_ptr<Hittable>> world_objects;
  for (const Sphere_& raw_sphere : raw_scene.spheres)
  {
    Vec3 center = Vec3(raw_scene.vertex_data[raw_sphere.center_vertex_id]);
    double radius = static_cast<double>(raw_sphere.radius);
    int material_id = raw_sphere.material_id;
		world_objects.push_back(
      std::make_shared<Sphere>(center, radius, material_id));
  }

  for(const Triangle_ & raw_triangle : raw_scene.triangles)
  {
    Vec3 indices[3] = { raw_scene.vertex_data[raw_triangle.v0_id], 
      raw_scene.vertex_data[raw_triangle.v1_id], 
      raw_scene.vertex_data[raw_triangle.v2_id]};

    world_objects.push_back(
      std::make_shared<Triangle>(indices, raw_triangle.material_id));
	}


  for(const Mesh_& raw_mesh : raw_scene.meshes)
  {
    if (raw_mesh.smooth_shading)
    {
			std::vector<std::vector<std::pair<Vec3, double>>> per_vertex_triangles; // pair<triangle_normal, area> for each vertex
      per_vertex_triangles.resize(raw_scene.vertex_data.size());
      for(const Triangle_& raw_triangle : raw_mesh.faces)
      {
        Vec3 v0 = Vec3(raw_scene.vertex_data[raw_triangle.v0_id]);
        Vec3 v1 = Vec3(raw_scene.vertex_data[raw_triangle.v1_id]);
        Vec3 v2 = Vec3(raw_scene.vertex_data[raw_triangle.v2_id]);
        double area = getAreaTriangle(v0, v1, v2);
        Vec3 edge1 = v1 - v0;
        Vec3 edge2 = v2 - v0;
        Vec3 face_normal = edge1.cross(edge2).normalize();
        per_vertex_triangles[raw_triangle.v0_id].push_back(std::make_pair(face_normal, area));
        per_vertex_triangles[raw_triangle.v1_id].push_back(std::make_pair(face_normal, area));
        per_vertex_triangles[raw_triangle.v2_id].push_back(std::make_pair(face_normal, area));
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
        Vec3 indices[3] = { raw_scene.vertex_data[raw_triangle.v0_id],
          raw_scene.vertex_data[raw_triangle.v1_id],
          raw_scene.vertex_data[raw_triangle.v2_id] };

				Vec3 per_vertex_normals[3] = {
          vertex_normals[raw_triangle.v0_id],
          vertex_normals[raw_triangle.v1_id],
					vertex_normals[raw_triangle.v2_id] };

        world_objects.push_back(
          std::make_shared<Triangle>(indices, raw_triangle.material_id, per_vertex_normals));
      }
    }
    else
    {
      for (const Triangle_& raw_triangle : raw_mesh.faces)
      {
        Vec3 indices[3] = { raw_scene.vertex_data[raw_triangle.v0_id],
          raw_scene.vertex_data[raw_triangle.v1_id],
          raw_scene.vertex_data[raw_triangle.v2_id] };

        world_objects.push_back(
          std::make_shared<Triangle>(indices, raw_triangle.material_id));
      }
    }
	}

  for(const Plane_& raw_plane : raw_scene.planes)
  {
    planes.push_back(Plane(raw_plane, raw_scene.vertex_data));
	}


	MaterialManager material_manager(raw_scene.materials);

  Scene scene(raw_scene, world_objects);

  RendererInfo renderer_info(raw_scene.shadow_ray_epsilon, 
    raw_scene.intersection_test_epsilon, 
    raw_scene.max_recursion_depth,
    BACKFACE_CULLING);

  BaseRayTracer ray_tracer(scene.background_color, scene.light_sources, 
    scene.world, planes, material_manager, renderer_info);

  RenderManager renderer(scene, material_manager, renderer_info, ray_tracer);

  std::cout << "Rendering will start here in the future." << std::endl;
  
  renderer.render();

  return 0;
}