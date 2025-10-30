#ifndef BASE_RAY_TRACER_H
#define BASE_RAY_TRACER_H
#include "rendering_technique.h"
#include "../objects/plane.h"
#define OUT

class BaseRayTracer : public RenderingTechnique {
public:
	BaseRayTracer( Color& background_color,
		LightSources& light_sources,
		BvhNode& world,
		std::vector<Plane>& planes,
		MaterialManager& material_manager,
		RendererInfo& renderer_info);

	Color traceRay(const Ray& ray) const override;

	Color computeColor(const Ray& ray, int depth) const;

	Color applyShading(const Ray& ray, int depth, HitRecord& rec) const;

	bool hitPlanes(const Ray& ray, Interval ray_t, HitRecord& rec) const;

	Color& background_color;
	LightSources& light_sources;
	BvhNode& world;
	std::vector<Plane>& planes;
	MaterialManager& material_manager;
	RendererInfo& renderer_info;
};

#endif // BASE_RAY_TRACER_H