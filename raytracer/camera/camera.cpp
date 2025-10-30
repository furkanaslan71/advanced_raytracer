#include "camera.h"


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

Camera::Camera(const Camera_& cam)
	: id(cam.id),
	position(cam.position.x, cam.position.y, cam.position.z),
	gaze(cam.gaze.x, cam.gaze.y, cam.gaze.z),
	up(cam.up.x, cam.up.y, cam.up.z),
	near_distance(cam.near_distance),
	image_width(cam.image_width),
	image_height(cam.image_height),
	image_name(cam.image_name)
{
	near_plane[0] = cam.near_plane.l;
	near_plane[1] = cam.near_plane.r;
	near_plane[2] = cam.near_plane.b;
	near_plane[3] = cam.near_plane.t;
	double l = near_plane[0];
	double r = near_plane[1];
	double b = near_plane[2];
	double t = near_plane[3];
	Vec3 gazeCopy = gaze;
	w = gazeCopy.normalize() * -1.0;
	u = up.cross(w).normalize();
	v = w.cross(u).normalize();
	m = position + (gazeCopy.normalize() * near_distance);
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
	/*
	const int numThreads = std::thread::hardware_concurrency();
	std::vector<std::thread> threads(numThreads);
	for (int threadId = 0; threadId < numThreads; threadId++)
	{
		threads[threadId] = std::thread([this, &world, threadId, numThreads, 
			&rendering_technique, &background_color, &light_sources,
			&material_manager, &renderer_info, &image]() {
			//int k = 0;
			for (int i = threadId; i < image_height; i+=numThreads)
			{
				//k++;
				//int l = 0;
				for (int j = 0; j < image_width; ++j)
				{
					//l++;
					Vec3 pixel_center = q + su * (j + 0.5) + sv * (i + 0.5);
					
					if (j == 770 && i == 545)
					{
						int debug = 1;
					}
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
	*/

}


