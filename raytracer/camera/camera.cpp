#include "camera.h"
#include "../include/config.h"


Camera::Camera()
	: id(0),
	position(0, 0, 0),
	gaze(0, 0, 0),
	up(0, 0, 0),
	near_plane{ 0, 0, 0, 0 },
	near_distance(0),
	image_width(0),
	image_height(0),
	image_name("")
{

}

Camera::Camera(const Camera_& cam,
	const std::vector<Translation_>& translations,
	const std::vector<Scaling_>& scalings,
	const std::vector<Rotation_>& rotations)
	: id(cam.id),
	position(cam.position.x, cam.position.y, cam.position.z),
	gaze(cam.gaze.x, cam.gaze.y, cam.gaze.z),
	up(cam.up.x, cam.up.y, cam.up.z),
	near_distance(cam.near_distance),
	image_width(cam.image_width),
	image_height(cam.image_height),
	image_name(cam.image_name),
	transformations(cam.transformations)
{
	composite_transformation_matrix = calculateCompositeTransformationMatrix(
		translations,
		scalings,
		rotations);
		
	gaze.normalize();
	
	glm::vec4 transformed_position = composite_transformation_matrix * glm::vec4(position.x, position.y, position.z, 1.0f);
	glm::vec4 transformed_gaze = composite_transformation_matrix * glm::vec4(gaze.x, gaze.y, gaze.z, 0.0f);
	glm::vec4 transformed_up = composite_transformation_matrix * glm::vec4(up.x, up.y, up.z, 0.0f);

	position = Vec3(transformed_position.x, transformed_position.y, transformed_position.z);
	gaze = Vec3(transformed_gaze.x, transformed_gaze.y, transformed_gaze.z);
	up = Vec3(transformed_up.x, transformed_up.y, transformed_up.z);

	double zoom_factor = gaze.length();
	if (zoom_factor < 1e-6) zoom_factor = 1.0; 
	
	Vec3 w = gaze.normalize() * -1.0;
	Vec3 u = up.cross(w).normalize();
	Vec3 v = w.cross(u); // Already normalized because w and u are orthonormal

	near_plane[0] = cam.near_plane.l;
	near_plane[1] = cam.near_plane.r;
	near_plane[2] = cam.near_plane.b;
	near_plane[3] = cam.near_plane.t;

	double l = near_plane[0] / zoom_factor;
	double r = near_plane[1] / zoom_factor;
	double b = near_plane[2] / zoom_factor;
	double t = near_plane[3] / zoom_factor;

	Vec3 m = position + (gaze.normalize() * near_distance);
	q = m + u * l + v * t;
	su = u * ((r - l) / image_width);
	sv = v * ((b - t) / image_height);
}

Camera::~Camera()
{
}

void Camera::render(IN const BaseRayTracer& rendering_technique,
										OUT std::vector<std::vector<Color>>& image) const
{
	image.resize(image_height, std::vector<Color>(image_width, Color(0, 0, 0)));

#if DEBUG_PIXEL
	Vec3 pixel_center = q + su * (WIDTH + 0.5) + sv * (HEIGHT + 0.5);
	Ray primary_ray(position, (pixel_center - position).normalize());

	Color pixel_color = rendering_technique.traceRay(primary_ray);
	pixel_color = pixel_color.clamp();
	image[HEIGHT][WIDTH] = pixel_color;
#else

#if !MULTI_THREADING
	for (int i = 0; i < image_height; ++i)
	{
		for (int j = 0; j < image_width; ++j)
		{
			Vec3 pixel_center = q + su * (j + 0.5) + sv * (i + 0.5);
			Ray primary_ray(position, (pixel_center - position).normalize());

			Color pixel_color = rendering_technique.traceRay(primary_ray);
			pixel_color = pixel_color.clamp();
			image[i][j] = pixel_color;
		}
	}
#else
	const int numThreads = std::thread::hardware_concurrency();
	std::vector<std::thread> threads(numThreads);
	for (int threadId = 0; threadId < numThreads; threadId++)
	{
		threads[threadId] = std::thread([this, threadId, numThreads, 
			&rendering_technique, &image]() {
			//int k = 0;
			for (int i = threadId; i < image_height; i+=numThreads)
			{
				//k++;
				//int l = 0;
				for (int j = 0; j < image_width; ++j)
				{
					//l++;
					Vec3 pixel_center = q + su * (j + 0.5) + sv * (i + 0.5);
					
					Ray primary_ray(position, (pixel_center - position).normalize());

					Color pixel_color = rendering_technique.traceRay(primary_ray);
					pixel_color = pixel_color.clamp();
					image[i][j] = pixel_color;
					//std::cout << "i:" << i << " j:" << j << std::endl;
				}
				//std::cout << i  << std::endl;
			}
		});
	}
	for (int threadId = 0; threadId < numThreads; ++threadId)
	{
		threads[threadId].join();
	}
#endif

#endif
	

}


glm::mat4 Camera::calculateCompositeTransformationMatrix(
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
std::vector<std::string> transformations;

