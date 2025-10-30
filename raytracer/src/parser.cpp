﻿#include "../include/parser.hpp"
#include "../external/json.hpp"
#include <fstream>
#include <iostream>
#include <sstream>
#define M_PI 3.14159265358979323846

// For convenience
using json = nlohmann::json;

// Helper functions
Vec3f_ parseVec3f(const std::string& str) {
    Vec3f_ vec;
    std::stringstream ss(str);
    ss >> vec.x >> vec.y >> vec.z;
    return vec;
}

Vec4f_ parseVec4f(const std::string& str) {
    Vec4f_ vec;
    std::stringstream ss(str);
    ss >> vec.l >> vec.r >> vec.b >> vec.t;
    return vec;
}

static float readFloatLE(std::ifstream& f)
{
  float val;
  f.read(reinterpret_cast<char*>(&val), sizeof(float));
  return val;
}

static int32_t readInt32LE(std::ifstream& f)
{
  int32_t val;
  f.read(reinterpret_cast<char*>(&val), sizeof(int32_t));
  return val;
}

static uint8_t readUInt8(std::ifstream& f)
{
  uint8_t val;
  f.read(reinterpret_cast<char*>(&val), sizeof(uint8_t));
  return val;
}

bool perpendicular(Vec3f_ a, Vec3f_ b)
{
  double dot_product = a.x * b.x + a.y * b.y + a.z * b.z;
  return fabs(dot_product) < 1e-8;
}

Vec3f_ crossProduct(Vec3f_ a, Vec3f_ b)
{
  Vec3f_ result;
  result.x = a.y * b.z - a.z * b.y;
  result.y = a.z * b.x - a.x * b.z;
  result.z = a.x * b.y - a.y * b.x;
  return result;
}

// --- UNIVERSAL, ROBUST PLY PARSER (ASCII + basic binary) ---
namespace PlyHelpers
{
  // --- Endianness Check ---
  bool is_system_little_endian()
  {
    uint32_t i = 1;
    char* c = (char*)&i;
    return (*c);
  }

  // --- Byte Swap ---
  template <typename T>
  T byte_swap(T val)
  {
    T swapped_val;
    char* src = reinterpret_cast<char*>(&val);
    char* dst = reinterpret_cast<char*>(&swapped_val);
    for (size_t i = 0; i < sizeof(T); ++i)
    {
      dst[i] = src[sizeof(T) - 1 - i];
    }
    return swapped_val;
  }

  // --- Data Extraction from Buffer ---
  template <typename T>
  T extract_from_buffer(const char* buffer, bool file_is_little_endian)
  {
    T val;
    std::memcpy(&val, buffer, sizeof(T)); // Use memcpy to avoid alignment issues
    if (file_is_little_endian != is_system_little_endian())
    {
      val = byte_swap(val);
    }
    return val;
  }

  // --- Get Type Size ---
  size_t get_ply_type_size(const std::string& type)
  {
    if (type == "char" || type == "uchar" || type == "int8" || type == "uint8") return 1;
    if (type == "short" || type == "ushort" || type == "int16" || type == "uint16") return 2;
    if (type == "int" || type == "uint" || type == "float" || type == "int32" || type == "uint32" || type == "float32") return 4;
    if (type == "double" || type == "int64" || type == "uint64" || type == "float64") return 8;
    std::cerr << "Warning: Unknown PLY type '" << type << "', assuming 0 size." << std::endl;
    return 0;
  }

  // --- Extract Integer (any size) ---
  int extract_int_from_buffer(const char* buffer, const std::string& type, bool file_is_little_endian)
  {
    if (type == "char" || type == "int8") return static_cast<int>(extract_from_buffer<int8_t>(buffer, file_is_little_endian));
    if (type == "uchar" || type == "uint8") return static_cast<int>(extract_from_buffer<uint8_t>(buffer, file_is_little_endian));
    if (type == "short" || type == "int16") return static_cast<int>(extract_from_buffer<int16_t>(buffer, file_is_little_endian));
    if (type == "ushort" || type == "uint16") return static_cast<int>(extract_from_buffer<uint16_t>(buffer, file_is_little_endian));
    if (type == "int" || type == "int32") return static_cast<int>(extract_from_buffer<int32_t>(buffer, file_is_little_endian));
    if (type == "uint" || type == "uint32") return static_cast<int>(extract_from_buffer<uint32_t>(buffer, file_is_little_endian));
    return 0;
  }

  // --- Extract Float/Double ---
  float extract_float_from_buffer(const char* buffer, const std::string& type, bool file_is_little_endian)
  {
    if (type == "float" || type == "float32") return extract_from_buffer<float>(buffer, file_is_little_endian);
    if (type == "double" || type == "float64") return static_cast<float>(extract_from_buffer<double>(buffer, file_is_little_endian));
    return 0.0f;
  }

  // --- PLY Property Structs ---
  struct PlyProperty {
    std::string name;
    std::string type;
    size_t size = 0;
    size_t offset = 0;
  };

  struct PlyFaceProperty {
    std::string count_type = "uchar";
    std::string index_type = "int";
    size_t count_size = 1;
    size_t index_size = 4;
  };

  enum PlyFormat { UNKNOWN, ASCII, BINARY_LITTLE_ENDIAN, BINARY_BIG_ENDIAN };

} // namespace PlyHelpers


// --- UNIVERSAL, ROBUST PLY PARSER (ASCII + BINARY) ---
void parsePlyFile(const std::string& ply_filename, Mesh_& mesh, Scene_& scene)
{
  using namespace PlyHelpers;

  std::ifstream file(ply_filename, std::ios::binary);
  if (!file.is_open())
  {
    std::cerr << "Error: Could not open PLY file: " << ply_filename << std::endl;
    return;
  }

  // --- Header Parsing ---
  std::string line;
  PlyFormat format = UNKNOWN;
  long num_vertices = 0;
  long num_faces = 0;

  std::vector<PlyProperty> vertex_props;
  PlyFaceProperty face_prop;
  size_t vertex_record_size = 0;
  int x_prop_idx = -1, y_prop_idx = -1, z_prop_idx = -1;

  bool in_vertex_element = false;
  bool in_face_element = false;

  while (std::getline(file, line))
  {
    // Trim trailing CR/LF
    line.erase(line.find_last_not_of(" \t\n\r") + 1);

    std::stringstream ss(line);
    std::string token;
    ss >> token;

    if (token == "ply") continue;
    if (token == "comment") continue;

    if (token == "format")
    {
      ss >> token;
      if (token == "ascii") format = ASCII;
      else if (token == "binary_little_endian") format = BINARY_LITTLE_ENDIAN;
      else if (token == "binary_big_endian") format = BINARY_BIG_ENDIAN;
    }
    else if (token == "element")
    {
      ss >> token;
      if (token == "vertex")
      {
        ss >> num_vertices;
        in_vertex_element = true;
        in_face_element = false;
        vertex_record_size = 0;
        vertex_props.clear();
      }
      else if (token == "face")
      {
        ss >> num_faces;
        in_vertex_element = false;
        in_face_element = true;
      }
      else
      {
        in_vertex_element = false;
        in_face_element = false;
      }
    }
    else if (token == "property")
    {
      if (in_vertex_element)
      {
        PlyProperty p;
        ss >> p.type >> p.name;
        p.size = get_ply_type_size(p.type);
        p.offset = vertex_record_size;
        vertex_record_size += p.size;

        if (p.name == "x") x_prop_idx = vertex_props.size();
        else if (p.name == "y") y_prop_idx = vertex_props.size();
        else if (p.name == "z") z_prop_idx = vertex_props.size();

        vertex_props.push_back(p);
      }
      else if (in_face_element)
      {
        std::string list_token, count_type, index_type, prop_name;
        ss >> list_token >> count_type >> index_type >> prop_name;
        if (list_token == "list" && prop_name == "vertex_index")
        {
          face_prop.count_type = count_type;
          face_prop.index_type = index_type;
          face_prop.count_size = get_ply_type_size(count_type);
          face_prop.index_size = get_ply_type_size(index_type);
        }
      }
    }
    else if (token == "end_header")
    {
      break; // End of header, data follows
    }
  }

  // --- Header Validation ---
  if (format == UNKNOWN)
  {
    std::cerr << "Error: Unknown PLY format in " << ply_filename << std::endl;
    return;
  }
  if (num_vertices == 0)
  {
    std::cerr << "Error: PLY file has 0 vertices: " << ply_filename << std::endl;
    return;
  }
  if (x_prop_idx == -1 || y_prop_idx == -1 || z_prop_idx == -1)
  {
    std::cerr << "Error: PLY file missing x, y, or z vertex property: " << ply_filename << std::endl;
    return;
  }

  // --- Data Parsing ---

  // This is the critical step for combining meshes:
  // All indices from this file will be offset by the current vertex count.
  int vertex_base_index = scene.vertex_data.size();
  bool file_is_little_endian = (format == BINARY_LITTLE_ENDIAN);

  std::cout << "  Parsing " << num_vertices << " vertices from " << ply_filename << "..." << std::endl;

  // 1. Read Vertices
  std::vector<char> binary_buffer(vertex_record_size);
  for (long i = 0; i < num_vertices; ++i)
  {
    Vec3f_ v;

    if (format == ASCII)
    {
      if (!std::getline(file, line))
      {
        std::cerr << "Error: Unexpected end of file reading vertices from " << ply_filename << std::endl;
        return;
      }
      std::stringstream ss(line);
      float val;
      for (size_t j = 0; j < vertex_props.size(); ++j)
      {
        ss >> val; // Read value
        // Assign to correct component if it's x, y, or z
        if (j == x_prop_idx) v.x = val;
        else if (j == y_prop_idx) v.y = val;
        else if (j == z_prop_idx) v.z = val;
      }
    }
    else
    { // Binary formats
      file.read(binary_buffer.data(), vertex_record_size);
      if (!file)
      {
        std::cerr << "Error: Unexpected end of file reading binary vertices from " << ply_filename << std::endl;
        return;
      }

      const PlyProperty& p_x = vertex_props[x_prop_idx];
      v.x = extract_float_from_buffer(binary_buffer.data() + p_x.offset, p_x.type, file_is_little_endian);

      const PlyProperty& p_y = vertex_props[y_prop_idx];
      v.y = extract_float_from_buffer(binary_buffer.data() + p_y.offset, p_y.type, file_is_little_endian);

      const PlyProperty& p_z = vertex_props[z_prop_idx];
      v.z = extract_float_from_buffer(binary_buffer.data() + p_z.offset, p_z.type, file_is_little_endian);
    }

    // Add the new vertex to the GLOBAL scene vertex list
    scene.vertex_data.push_back(v);
  }

  std::cout << "  Parsing " << num_faces << " faces from " << ply_filename << "..." << std::endl;

  // 2. Read Faces
  std::vector<char> count_buffer(face_prop.count_size);
  std::vector<char> index_buffer(face_prop.index_size);

  for (long i = 0; i < num_faces; ++i)
  {
    std::vector<int> local_indices;
    int num_verts_in_face = 0;

    if (format == ASCII)
    {
      if (!std::getline(file, line))
      {
        std::cerr << "Error: Unexpected end of file reading faces from " << ply_filename << std::endl;
        return;
      }
      std::stringstream ss(line);
      ss >> num_verts_in_face;
      int index;
      for (int j = 0; j < num_verts_in_face; ++j)
      {
        ss >> index;
        local_indices.push_back(index);
      }
    }
    else
    { // Binary formats
      file.read(count_buffer.data(), face_prop.count_size);
      if (!file)
      {
        std::cerr << "Error: Unexpected end of file reading face count from " << ply_filename << std::endl;
        return;
      }
      num_verts_in_face = extract_int_from_buffer(count_buffer.data(), face_prop.count_type, file_is_little_endian);

      for (int j = 0; j < num_verts_in_face; ++j)
      {
        file.read(index_buffer.data(), face_prop.index_size);
        if (!file)
        {
          std::cerr << "Error: Unexpected end of file reading face index from " << ply_filename << std::endl;
          return;
        }
        local_indices.push_back(extract_int_from_buffer(index_buffer.data(), face_prop.index_type, file_is_little_endian));
      }
    }

    // --- Triangulation (same logic for all formats) ---
    if (num_verts_in_face < 3)
    {
      std::cerr << "Warning: Skipping face " << i << " (less than 3 vertices) in " << ply_filename << std::endl;
      continue;
    }

    // Triangulate the polygon (N-gon) using a triangle fan
    // Pivot vertex is v0
    int v0_global = local_indices[0] + vertex_base_index;

    for (size_t j = 1; j < local_indices.size() - 1; ++j)
    {
      int v1_global = local_indices[j] + vertex_base_index;
      int v2_global = local_indices[j + 1] + vertex_base_index;

      // Add the new face (triangle) to the MESH's face list
      // It uses the mesh's material_id and the GLOBAL vertex indices
      mesh.faces.push_back({ mesh.material_id, v0_global, v1_global, v2_global });
    }
  }

  std::cout << "  Finished parsing PLY file. Total vertices in scene: " << scene.vertex_data.size() << std::endl;
  file.close();
}


// Function implementation
void parseScene(const std::string& filename, Scene_& scene) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        std::cerr << "Could not open scene file: " << filename << std::endl;
        return;
    }

    json j;
    file >> j;
    const auto& scene_json = j["Scene"];

    // --- Global Scene Settings ---
    scene.background_color = parseVec3f(scene_json["BackgroundColor"]);

    if (scene_json.contains("ShadowRayEpsilon"))
        scene.shadow_ray_epsilon = std::stof(scene_json["ShadowRayEpsilon"].get<std::string>());
    else
        scene.shadow_ray_epsilon = 1e-6;

    if (scene_json.contains("IntersectionTestEpsilon"))
        scene.intersection_test_epsilon = std::stof(scene_json["IntersectionTestEpsilon"].get<std::string>());
    else
        scene.intersection_test_epsilon = 0;

    if (scene_json.contains("MaxRecursionDepth"))
        scene.max_recursion_depth = std::stoi(scene_json["MaxRecursionDepth"].get<std::string>());
    else
        scene.max_recursion_depth = 6;

    // --- Cameras ---
    const auto& cameras_json = scene_json["Cameras"]["Camera"];
    auto parse_camera = [&](const json& cam_json) {
      Camera_ cam;

      cam.id = std::stoi(cam_json["_id"].get<std::string>());
      cam.position = parseVec3f(cam_json["Position"]);
      cam.up = parseVec3f(cam_json["Up"]);
      cam.near_distance = std::stof(cam_json["NearDistance"].get<std::string>());
      std::stringstream res_ss(cam_json["ImageResolution"].get<std::string>());
      res_ss >> cam.image_width >> cam.image_height;
      cam.image_name = cam_json["ImageName"];

      if (cam_json.contains("GazePoint"))
      {
        Vec3f_ gaze_point = parseVec3f(cam_json["GazePoint"]);
        Vec3f_ gaze_vec = {
            gaze_point.x - cam.position.x,
            gaze_point.y - cam.position.y,
            gaze_point.z - cam.position.z
        };
        float len = std::sqrt(gaze_vec.x * gaze_vec.x + gaze_vec.y * gaze_vec.y + gaze_vec.z * gaze_vec.z);
        if (len > 0)
        { // Avoid division by zero
          gaze_vec.x /= len;
          gaze_vec.y /= len;
          gaze_vec.z /= len;
        }
        cam.gaze = gaze_vec;

        // 2. Calculate 'near_plane' extents from FovY
        float fov_y_degrees = std::stof(cam_json["FovY"].get<std::string>());
        float aspect_ratio = (float)cam.image_width / (float)cam.image_height;

        // Formula: top = near_distance * tan(fov_y / 2)
        float fov_y_radians = fov_y_degrees * (M_PI / 180.0);
        float t = cam.near_distance * std::tan(fov_y_radians / 2.0f);
        float r = t * aspect_ratio;

        cam.near_plane.l = -r;
        cam.near_plane.r = r;
        cam.near_plane.b = -t;
        cam.near_plane.t = t;
      }
      else
      {
        // --- TYPE 2: Your original camera format ---
        cam.gaze = parseVec3f(cam_json["Gaze"]);
        cam.near_plane = parseVec4f(cam_json["NearPlane"]);
      }

      if (!perpendicular(cam.gaze, cam.up))
      {
				float len_gaze = std::sqrt(cam.gaze.x * cam.gaze.x + cam.gaze.y * cam.gaze.y + cam.gaze.z * cam.gaze.z);
				Vec3f_ w = Vec3f_{ -cam.gaze.x / len_gaze,
          -cam.gaze.y / len_gaze,
          -cam.gaze.z / len_gaze };
				float len_up = std::sqrt(cam.up.x * cam.up.x + cam.up.y * cam.up.y + cam.up.z * cam.up.z);
        Vec3f_ v_ = Vec3f_{ cam.up.x / len_up,
          cam.up.y / len_up,
					cam.up.z / len_up };
        Vec3f_ u = crossProduct(v_, w);
				Vec3f_ v = crossProduct(w, u);
				cam.up = v;
      }

      scene.cameras.push_back(cam);
      };

    if (cameras_json.is_array())
    {
      for (const auto& cam_json : cameras_json) parse_camera(cam_json);
    }
    else
    {
      parse_camera(cameras_json);
    }

    // --- Lights ---
    scene.ambient_light = parseVec3f(scene_json["Lights"]["AmbientLight"]);
    const auto& point_lights_json = scene_json["Lights"]["PointLight"];
    auto parse_point_light = [&](const json& pl_json) {
        PointLight_ pl;
        pl.id = std::stoi(pl_json["_id"].get<std::string>());
        pl.position = parseVec3f(pl_json["Position"]);
        pl.intensity = parseVec3f(pl_json["Intensity"]);
        scene.point_lights.push_back(pl);
    };
    if (point_lights_json.is_array()) {
        for (const auto& pl_json : point_lights_json) parse_point_light(pl_json);
    } else {
        parse_point_light(point_lights_json);
    }

    // --- Materials ---
    const auto& materials_json = scene_json["Materials"]["Material"];
    auto parse_material = [&](const json& mat_json) {
        Material_ mat;
        mat.id = std::stoi(mat_json["_id"].get<std::string>());
        if (mat_json.contains("AmbientReflectance")) mat.ambient_reflectance = parseVec3f(mat_json["AmbientReflectance"]);
				else mat.ambient_reflectance = { 0.0f, 0.0f, 0.0f };
        if (mat_json.contains("DiffuseReflectance")) mat.diffuse_reflectance = parseVec3f(mat_json["DiffuseReflectance"]);
				else mat.diffuse_reflectance = { 0.0f, 0.0f, 0.0f };
        if (mat_json.contains("SpecularReflectance")) mat.specular_reflectance = parseVec3f(mat_json["SpecularReflectance"]);
				else mat.specular_reflectance = { 0.0f, 0.0f, 0.0f };
        if (mat_json.contains("PhongExponent")) mat.phong_exponent = std::stof(mat_json["PhongExponent"].get<std::string>());
				else mat.phong_exponent = 0.0f;
				if (mat_json.contains("_type")) mat.type = mat_json["_type"].get<std::string>();
				else mat.type = "none";
				if (mat_json.contains("MirrorReflectance")) mat.mirror_reflectance = parseVec3f(mat_json["MirrorReflectance"]);
				else mat.mirror_reflectance = { 0.0f, 0.0f, 0.0f };
				if (mat_json.contains("RefractionIndex")) mat.refraction_index = std::stof(mat_json["RefractionIndex"].get<std::string>());
				else mat.refraction_index = 1.0f;
				if (mat_json.contains("AbsorptionCoefficient")) mat.absorption_coefficient = parseVec3f(mat_json["AbsorptionCoefficient"]);
				else mat.absorption_coefficient = { 0.0f, 0.0f, 0.0f };
        if (mat_json.contains("AbsorptionIndex")) mat.absorption_index = std::stof(mat_json["AbsorptionIndex"].get<std::string>());
				else mat.absorption_index = 0.0f;
        scene.materials.push_back(mat);
    };
    if (materials_json.is_array()) {
        for (const auto& mat_json : materials_json) parse_material(mat_json);
    } else {
        parse_material(materials_json);
    }

    // --- Vertex Data ---
    std::stringstream vd_ss(scene_json["VertexData"]["_data"].get<std::string>());
    float x, y, z;
    while (vd_ss >> x >> y >> z) {
        scene.vertex_data.push_back({x, y, z});
    }

    // --- Objects ---
    const auto& objects_json = scene_json["Objects"];

    // Parse Meshes
    if (objects_json.contains("Mesh"))
    {
      const auto& meshes_json = objects_json["Mesh"];
      auto parse_mesh = [&](const json& mesh_json) {
        Mesh_ mesh;
        mesh.id = std::stoi(mesh_json["_id"].get<std::string>());
        mesh.material_id = std::stoi(mesh_json["Material"].get<std::string>());
        if(mesh_json.contains("_shadingMode"))
          mesh.smooth_shading = (mesh_json["_shadingMode"].get<std::string>()).compare("smooth") == 0 ? true : false;
        else
					mesh.smooth_shading = false;

        const auto& faces_json = mesh_json["Faces"];

        if (faces_json.contains("_data"))
        {
          std::stringstream faces_ss(faces_json["_data"].get<std::string>());
          int v0, v1, v2;
          while (faces_ss >> v0 >> v1 >> v2)
          {
            mesh.faces.push_back({ mesh.material_id, v0 - 1, v1 - 1, v2 - 1 });
          }
        }
        else if (faces_json.contains("_plyFile"))
        {
          std::string ply_filename = faces_json["_plyFile"];

          std::string scene_dir = "";
          auto pos = filename.find_last_of("/\\");
          if (pos != std::string::npos)
          {
            scene_dir = filename.substr(0, pos + 1); // Keep the slash
          }
          std::string full_ply_path = scene_dir + ply_filename;

          // --- THE FIX ---
          // Call the corrected helper, passing the main scene object
          parsePlyFile(full_ply_path, mesh, scene);
        }

        scene.meshes.push_back(mesh);
        };

      if (meshes_json.is_array())
      {
        for (const auto& mesh_json : meshes_json) parse_mesh(mesh_json);
      }
      else
      {
        parse_mesh(meshes_json);
      }
    }

    // Parse Triangles
    if (objects_json.contains("Triangle")) {
        const auto& triangles_json = objects_json["Triangle"];
         auto parse_triangle = [&](const json& tri_json) {
            Triangle_ tri;
            tri.material_id = std::stoi(tri_json["Material"].get<std::string>());
             std::stringstream indices_ss(tri_json["Indices"].get<std::string>());
            indices_ss >> tri.v0_id >> tri.v1_id >> tri.v2_id;
            tri.v0_id--; tri.v1_id--; tri.v2_id--;
            scene.triangles.push_back(tri);
         };
         if (triangles_json.is_array()) {
            for (const auto& tri_json : triangles_json) parse_triangle(tri_json);
        } else {
            parse_triangle(triangles_json);
        }
    }

    // Parse Spheres
    if (objects_json.contains("Sphere")) {
        const auto& spheres_json = objects_json["Sphere"];
        auto parse_sphere = [&](const json& sphere_json) {
            Sphere_ sphere;
            sphere.id = std::stoi(sphere_json["_id"].get<std::string>());
            sphere.material_id = std::stoi(sphere_json["Material"].get<std::string>());
            sphere.center_vertex_id = std::stoi(sphere_json["Center"].get<std::string>()) - 1;
            sphere.radius = std::stof(sphere_json["Radius"].get<std::string>());
            scene.spheres.push_back(sphere);
        };
        if (spheres_json.is_array()) {
             for (const auto& sphere_json : spheres_json) parse_sphere(sphere_json);
        } else {
            parse_sphere(spheres_json);
        }
    }
    if (objects_json.contains("Plane"))
    {
      const auto& planes_json = objects_json["Plane"];
      auto parse_plane = [&](const json& plane_json) {
				Plane_ plane;
				plane.id = std::stoi(plane_json["_id"].get<std::string>());
				plane.material_id = std::stoi(plane_json["Material"].get<std::string>());
				plane.point_vertex_id = std::stoi(plane_json["Point"].get<std::string>()) - 1;
				plane.normal = parseVec3f(plane_json["Normal"]);
				scene.planes.push_back(plane);
				};
      if (planes_json.is_array()) {
          for (const auto& plane_json : planes_json) parse_plane(plane_json);
      } else {
				parse_plane(planes_json);
       }
     }
}

// A simple function to print a summary of the parsed scene
void printSceneSummary(const Scene_& scene) {
    std::cout << "--- Scene parsing successful ---" << std::endl;
    std::cout << "Cameras: " << scene.cameras.size() << std::endl;
    std::cout << "Lights: " << scene.point_lights.size() << std::endl;
    std::cout << "Materials: " << scene.materials.size() << std::endl;
    std::cout << "Vertices: " << scene.vertex_data.size() << std::endl;
    std::cout << "--------------------------------" << std::endl;
}

// --- Main function to print all scene data ---

void printScene(const Scene_& scene) {
    std::cout << "----------------------------------------" << std::endl;
    std::cout << "         DETAILED SCENE DATA            " << std::endl;
    std::cout << "----------------------------------------" << std::endl;

    // --- Global Settings ---
    std::cout << "\n[SCENE GLOBALS]" << std::endl;
    std::cout << "  Background Color: " << scene.background_color << std::endl;
    std::cout << "  Shadow Ray Epsilon: " << scene.shadow_ray_epsilon << std::endl;
    std::cout << "  Max Recursion Depth: " << scene.max_recursion_depth << std::endl;
    std::cout << " Intersection Test Epsilon" << scene.intersection_test_epsilon << std::endl;

    // --- Lights ---
    std::cout << "\n[LIGHTS]" << std::endl;
    std::cout << "  Ambient Light: " << scene.ambient_light << std::endl;
    for (const auto& light : scene.point_lights) {
        std::cout << "  Point Light ID " << light.id << ":" << std::endl;
        std::cout << "    Position: " << light.position << std::endl;
        std::cout << "    Intensity: " << light.intensity << std::endl;
    }

    // --- Cameras ---
    std::cout << "\n[CAMERAS]" << std::endl;
    for (const auto& cam : scene.cameras) {
        std::cout << "  Camera ID " << cam.id << ":" << std::endl;
        std::cout << "    Name: " << cam.image_name << std::endl;
        std::cout << "    Position: " << cam.position << std::endl;
        std::cout << "    Gaze: " << cam.gaze << std::endl;
        std::cout << "    Up: " << cam.up << std::endl;
        std::cout << "    Near Plane: " << cam.near_plane << std::endl;
        std::cout << "    Near Distance: " << cam.near_distance << std::endl;
        std::cout << "    Resolution: " << cam.image_width << "x" << cam.image_height << std::endl;
    }

    // --- Materials ---
    std::cout << "\n[MATERIALS]" << std::endl;
    for (const auto& mat : scene.materials) {
        std::cout << "  Material ID " << mat.id << ":" << std::endl;
        if (!mat.type.empty()) {
             std::cout << "    Type: " << mat.type << std::endl;
        }
        std::cout << "    Ambient: " << mat.ambient_reflectance << std::endl;
        std::cout << "    Diffuse: " << mat.diffuse_reflectance << std::endl;
        std::cout << "    Specular: " << mat.specular_reflectance << " (exp: " << mat.phong_exponent << ")" << std::endl;
        // Add more material properties if you parse them (mirror, refraction, etc.)
    }

    // --- Vertex Data ---
    std::cout << "\n[VERTEX DATA (" << scene.vertex_data.size() << " vertices)]" << std::endl;
    // We'll just print the first few to avoid a huge log
    int vertices_to_print = (int)scene.vertex_data.size();
    for (int i = 0; i < vertices_to_print; ++i) {
        std::cout << "  Vertex " << i + 1 << ": " << scene.vertex_data[i] << std::endl;
    }
    // --- Objects ---
    std::cout << "\n[OBJECTS]" << std::endl;
    for (const auto& mesh : scene.meshes) {
        std::cout << "  Mesh ID " << mesh.id << ":" << std::endl;
        std::cout << "    Material ID: " << mesh.material_id << std::endl;
        std::cout << "    Face Count: " << mesh.faces.size() << std::endl;
        // Print first face for verification
        if (!mesh.faces.empty()) {
            const auto& face = mesh.faces[0];
            std::cout << "    First Face Indices: " << face.v0_id << ", " << face.v1_id << ", " << face.v2_id << std::endl;
        }
    }
    for (const auto& tri : scene.triangles) {
        std::cout << "  Standalone Triangle:" << std::endl;
        std::cout << "    Material ID: " << tri.material_id << std::endl;
        std::cout << "    Indices: " << tri.v0_id << ", " << tri.v1_id << ", " << tri.v2_id << std::endl;
    }
    for (const auto& sphere : scene.spheres) {
        std::cout << "  Sphere ID " << sphere.id << ":" << std::endl;
        std::cout << "    Material ID: " << sphere.material_id << std::endl;
        std::cout << "    Center Vertex ID: " << sphere.center_vertex_id << std::endl;
        std::cout << "    Radius: " << sphere.radius << std::endl;
    }
     for (const auto& plane : scene.planes) {
        std::cout << "  Plane ID " << plane.id << ":" << std::endl;
        std::cout << "    Material ID: " << plane.material_id << std::endl;
        std::cout << "    Point Vertex ID: " << plane.point_vertex_id << std::endl;
        std::cout << "    Normal: " << plane.normal << std::endl;
    }

    std::cout << "\n----------------------------------------" << std::endl;
}

