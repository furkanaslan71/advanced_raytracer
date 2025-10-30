#include "scene.h"

#include "../include/ray.h"


Scene::Scene() {
		// Constructor implementation (if needed)
}

Scene::Scene(const Scene_& raw_scene, std::vector<std::shared_ptr<Hittable>>& objects)
	: background_color(raw_scene.background_color.x, raw_scene.background_color.y, raw_scene.background_color.z)
{
	for (const auto& raw_camera : raw_scene.cameras) {
		cameras.emplace_back(raw_camera);
	}
	for (const auto& raw_light : raw_scene.point_lights) {
		light_sources.point_lights.push_back(PointLight(raw_light.id,
			Vec3(raw_light.position),
			Color(raw_light.intensity.x, raw_light.intensity.y, raw_light.intensity.z)));
	}
	
	light_sources.ambient_light = Color(raw_scene.ambient_light.x,
		raw_scene.ambient_light.y,
		raw_scene.ambient_light.z);
		
	world = BvhNode(objects, 0, static_cast<int>(objects.size() - 1));
}

Scene::~Scene() {
		// Destructor implementation (if needed)
}

