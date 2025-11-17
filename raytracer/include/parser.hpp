#ifndef PARSER_H
#define PARSER_H

#include <string>
#include <vector>
#include <ostream>
#include <algorithm>
#include "config.h"

typedef struct Vec3f_ {
    float x, y, z;
} Vec3f_;

typedef struct Vec4f_ {
    float l, r, b, t;
} Vec4f_;

typedef struct Camera_ {
    int id;
    Vec3f_ position;
    Vec3f_ gaze;
    Vec3f_ up;
    Vec4f_ near_plane;
    float near_distance;
    int image_width;
    int image_height;
    std::string image_name;
    std::vector<std::string> transformations;
} Camera_;

typedef struct PointLight_ {
    int id;
    Vec3f_ position;
    Vec3f_ intensity;
		std::vector<std::string> transformations;
} PointLight_;

typedef struct Material_ {
    int id;
    std::string type;
    Vec3f_ ambient_reflectance;
    Vec3f_ diffuse_reflectance;
    Vec3f_ specular_reflectance;
    Vec3f_ mirror_reflectance;
    float phong_exponent;
    float refraction_index;
    Vec3f_ absorption_coefficient;
    float absorption_index;
} Material_;

typedef struct Triangle_ {
    int material_id;
    int v0_id, v1_id, v2_id;
    std::vector<std::string> transformations;
} Triangle_;

typedef struct Mesh_ {
    int id;
    int material_id;
		bool smooth_shading;
    std::vector<Triangle_> faces;
    std::vector<std::string> transformations;
} Mesh_;

typedef struct MeshInstance_ {
  int id;
  int base_mesh_id;
	int material_id;
	bool smooth_shading;
	bool reset_transform;
  std::vector<std::string> transformations;
}MeshInstance_;

typedef struct Sphere_ {
    int id;
    int material_id;
    int center_vertex_id;
    float radius;
    std::vector<std::string> transformations;
} Sphere_;

typedef struct Plane_ {
    int id;
    int material_id;
    int point_vertex_id;
    Vec3f_ normal;
    std::vector<std::string> transformations;
} Plane_;

typedef struct Translation_ {
   float tx, ty, tz;
}Translation_;

typedef struct Scaling_ {
  float sx, sy, sz;
}Scaling_;

typedef struct Rotation_ {
  float angle;
  float axis_x, axis_y, axis_z;
}Rotation_;
	

typedef struct Scene_ {
    Vec3f_ background_color;
    float shadow_ray_epsilon;
    float intersection_test_epsilon;
    int max_recursion_depth;         
    std::vector<Camera_> cameras;
    Vec3f_ ambient_light;
    std::vector<PointLight_> point_lights;
    std::vector<Material_> materials;
    std::vector<Vec3f_> vertex_data;
		std::vector<Translation_> translations;
		std::vector<Scaling_> scalings;
		std::vector<Rotation_> rotations;
    std::vector<Mesh_> meshes;
		std::vector<MeshInstance_> mesh_instances;
    std::vector<Triangle_> triangles;
    std::vector<Sphere_> spheres;
    std::vector<Plane_> planes;
} Scene_;

// --- Function Declaration ---

void parseScene(const std::string& filename, Scene_& scene);

inline std::ostream& operator<<(std::ostream& os, const Vec3f_& v) {
    os << "(" << v.x << ", " << v.y << ", " << v.z << ")";
    return os;
}

inline std::ostream& operator<<(std::ostream& os, const Vec4f_& v) {
    os << "(l:" << v.l << ", r:" << v.r << ", b:" << v.b << ", t:" << v.t << ")";
    return os;
}

void printSceneSummary(const Scene_& scene);
void printScene(const Scene_& scene);

#endif //PARSER_H
