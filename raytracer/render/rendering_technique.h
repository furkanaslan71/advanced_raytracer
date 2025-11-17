#ifndef RENDERING_TECHNIQUE_H
#define RENDERING_TECHNIQUE_H

#include "../include/color.h"
#include "../include/ray.h"
#include "../light/light.h"
#include "../material/material_manager.h"
#include "../include/bvh.h"

#define BACKFACE_CULLING 0
typedef struct RendererInfo {
	float shadow_ray_epsilon;
	float intersection_test_epsilon;
	int max_recursion_depth;;
}RendererInfo;


class RenderingTechnique {
public:
	RenderingTechnique() = default;
	virtual ~RenderingTechnique() = default;
	virtual Color traceRay(const Ray& ray) const = 0;
};

#endif // RENDERING_TECHNIQUE_H

