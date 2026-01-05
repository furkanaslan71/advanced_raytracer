# Ray Tracer Phase 5 Blog: HDR Rendering & Advanced Lighting

## Previously Mentioned Code Changes
After some considiretaion and my frind Alp Beysir's suggestions and code roastings, I decided to remove overall unnecessary usage of classes and templates. Except for the basic `struct`'s to hold data, the only classes are left are object classes and `raytracer`. The sampling, ray generation functions taken away from the `camera` classes and unified in the `raytracer`. The raytracer class simply holds all the necessary data (scene data, rendering context and area light sampling context) and runs the shading methods on them. The reason I made raytracer a class is that, otherwise I had to inform all other translation units that methods are defined about the global data. If they are in the same class, hence same namespace, they can access all the necessary data. Even though, this class is like a god class and holds too much methods, and have code duplicaitons. These could be simplified. Anyways, here is the header: 

```cpp
enum class BackgroundType : bool {
    Color,
    Texture
};

struct RenderContext {
    union {
        Color background_color;
        Texture* background_tex;
    }background_info;
    float shadow_ray_epsilon;
    float intersection_test_epsilon;
    int max_recursion_depth;
    BackgroundType b_type;
    Image* env_map;

    RenderContext() = default;
};

struct SamplingContext {
    const std::vector<std::vector<std::vector<glm::vec3>>>* area_light_samples;
    int sample_index;
};

class Raytracer {
public:
    Raytracer(std::unique_ptr<Scene> _scene, const RenderContext& _render_context);

    void renderScene() const;
private:
    void renderOneCamera(std::shared_ptr<BaseCamera> camera, std::vector<std::vector<Color>>& output) const;

    Color traceRay(const Ray& ray, const SamplingContext& sampling_context, const CameraContext& cam_context) const ;

    Color computeColor(const Ray& ray, int depth, const SamplingContext& sampling_context, const CameraContext& cam_context) const;

    Color applyShading(const Ray& ray, int depth, HitRecord& rec, const SamplingContext& sampling_context, const CameraContext& cam_context) const;

    bool hitPlanes(const Ray& ray, Interval ray_t, HitRecord& rec) const;

    void renderLoop(int threadId, int stride, std::shared_ptr<BaseCamera> camera, std::vector<std::vector<Color>>& output) const;

    std::unique_ptr<Scene> scene;
    RenderContext render_context;
};
```

I also removed the templated `Triangle` and `Mesh` magic to have a cleaner and simpler code. In branch-predictor we trust. 

## Futile BVH Optimization
In the shading logic, to determine whether the hit point is in shadow or not, I was casting a shadow ray and checking the intersection via `BVH<T>::intersect`. This is the general interseciton function that is being used to find hit points. As you can infer, this was unnecessary. Since the verdict about the shadow depens on the existance of the hit success, and not finding the closest one. So to further optimize, I added another verison of this function called `isShadowed` which immediately returns after the first successful hit. To benchmark, I was using the `dragon_dynamic.json` scene. Before the optimization, it took 70s to render. Could you guess the rendering time after the optimization? What would be your guess, 50 - 60 (:D)? It took 80s after the change. But how? I still don't know. I chatted with the LLMs about this and the answers were vague. The most logical conclusion I could come up with, adding another function maybe could result in instruction cache misses. I still don't know how it really works behind the scene or the compiler's behaviour, it is still absurd to me. The only rational outcome of this phenomenon is I will definetely take compiler course in the next semester. Also tried not making a new function and modified the original function by giving it another argument and if checking and it was still 80s. Then, made the function a templated function as I know in which part of code it will be called with true and false, this way I won't be lose any performance because of branching and it became 72s. Still worse than the original not optimized way. I left the code in this version hoping that in different scenes it will come handy but I am clueless and still slow (:/). 

## Tonemapping

I started with implementing tonemapping as it could be counted as post-process; thus, easier to integrate with the existing code-base. Started with `photographic`, following the paper linked in the pdf. Everything was fine except for this formula: 

![](images/reinhard.png)

I was getting plain black for `cube_point_hdr`. When I looked to the slide, this formula was different. The division by N is done in the exp, not outside. Correcting the formula fixed the scene, but I might expecting a brighter output for the wrong version not black. After implementing other operators, for filmic it was brighter version of the expected output, the other two was resulting in plain black. Maybe some kind of overflow occurs, IDK. Once correcting the `photographic`, and ensuring the input/output flow is correct, the other two operators were much easier to implement.  

PHOT:

![](images/cube_point_hdr.exr_phot.png)

FILM:

![](images/cube_point_hdr.exr_film.png)

ACES:

![](images/cube_point_hdr.exr_aces.png)

Here is the code:

```cpp
#define LUM(v) 0.2126 * v.r + 0.7152 * v.g + 0.0722 * v.b

Tonemap::Tonemap(const Tonemap_ &tm)
    : TMOOptions(tm.TMOOptions), saturation(tm.saturation), gamma(tm.gamma), extension(tm.extension)
{
    if (tm.TMO == "Photographic")
    {
        tmo = TMO::Photographic;
    }
    else if (tm.TMO == "Filmic")
    {
        tmo = TMO::Filmic;
    }
    else if (tm.TMO == "ACES")
    {
        tmo = TMO::ACES;
    }
    else
    {
        throw std::runtime_error("Unsupported TMO type");
    }
}

float logAvgLuminance(const std::vector<std::vector<Color>>& image)
{
    Expects(!image.empty());
    constexpr float eps = 1e-6f;
    int N = image.size() * image[0].size();
    float power = 0.0f;
    for (const auto& v : image)
    {
        for (const auto& c : v)
        {
            power += glm::log(eps + LUM(c));
        }
    }
    return glm::exp(power / N);
}

float scaledLuminance(float inv_log_avg, float key, const Color& color, float Yi)
{
    return key * inv_log_avg * Yi;
}

float gammaCorrect(float inv_g, float color_channel)
{
    if (degamma)
        return glm::clamp(color_channel, 0.0f, 1.0f) * 255.0f;
    return glm::clamp(glm::pow(color_channel, inv_g), 0.0f, 1.0f) * 255.0f;
}

void reinhard(const std::vector<std::vector<Color>>& input_img,
    std::vector<std::vector<Color>>& output_img, const Tonemap& tonemap)
{
    float log_avg_lum = logAvgLuminance(input_img);
    float inv_log_avg_lum = 1.0f / log_avg_lum;
    float inv_g = 1.0f / tonemap.gamma;

    float inv_Lwhite_2 = 0.0f;
    if (tonemap.TMOOptions.y > 0)
    {
        std::vector<float> luminances;
        for (const auto& row : input_img)
            for (const auto& col : row)
                luminances.push_back(LUM(col));

        std::sort(luminances.begin(), luminances.end());
        size_t index = std::round(luminances.size() * ((100.0f - tonemap.TMOOptions.y) / 100.0f));
        index = std::clamp(index, (size_t)0, luminances.size() - 1);

        float L_white_raw = luminances[index];
        float L_white_scaled = L_white_raw * tonemap.TMOOptions.x * inv_log_avg_lum;
        inv_Lwhite_2 = 1.0f / (L_white_scaled * L_white_scaled);
    }

    for (size_t i = 0; i < input_img.size(); i++)
    {
        for (size_t j = 0; j < input_img[i].size(); j++)
        {
            float Yi = LUM(input_img[i][j]);
            float L_scaled = scaledLuminance(inv_log_avg_lum, tonemap.TMOOptions.x, input_img[i][j], Yi);

            float Yo;
            if (tonemap.TMOOptions.y > 0)
            {
                Yo = (L_scaled * (1.0f + L_scaled * inv_Lwhite_2)) / (1.0f + L_scaled);
            }
            else
            {
                Yo = L_scaled / (1.0f + L_scaled);
            }

            float Yi_safe = std::max(Yi, 1e-6f);
            float ratio = Yo / Yi_safe;

            output_img[i][j].r = gammaCorrect(inv_g, glm::pow(input_img[i][j].r * ratio, tonemap.saturation));
            output_img[i][j].g = gammaCorrect(inv_g, glm::pow(input_img[i][j].g * ratio, tonemap.saturation));
            output_img[i][j].b = gammaCorrect(inv_g, glm::pow(input_img[i][j].b * ratio, tonemap.saturation));
        }
    }
}

void filmic(const std::vector<std::vector<Color>>& input_img,
    std::vector<std::vector<Color>>& output_img, const Tonemap& tonemap)
{
    float log_avg_lum = logAvgLuminance(input_img);
    float inv_log_avg_lum = 1.0f / log_avg_lum;

    auto map_filmic = [](float L) {
        float a = 0.22f;
        float b = 0.30f;
        float c = 0.10f;
        float d = 0.20f;
        float e = 0.01f;
        float f = 0.30f;
        return ((L * (a * L + c * b) + d * e) / (L * (a * L + b) + d * f)) - (e / f);
    };

    std::vector<float> luminances;
    for (const auto& row : input_img)
        for (const auto& col : row)
            luminances.push_back(LUM(col));

    std::sort(luminances.begin(), luminances.end());
    size_t index = std::round(luminances.size() * ((100.0f - tonemap.TMOOptions.y) / 100.0f));
    float L_white_raw = luminances[std::min(index, luminances.size() - 1)];

    float L_white_scaled = L_white_raw * tonemap.TMOOptions.x * inv_log_avg_lum;
    float map_W = map_filmic(L_white_scaled);

    float inv_g = 1.0f / tonemap.gamma;

    for (size_t i = 0; i < input_img.size(); i++)
    {
        for (size_t j = 0; j < input_img[i].size(); j++)
        {
            float Yi = LUM(input_img[i][j]);
            float L_scaled = scaledLuminance(inv_log_avg_lum, tonemap.TMOOptions.x, input_img[i][j], Yi);

            float Yo = map_filmic(L_scaled) / map_W;

            Yi = std::max(Yi, 1e-6f);
            float Ro = Yo * glm::pow((input_img[i][j].r / Yi), tonemap.saturation);
            float Go = Yo * glm::pow((input_img[i][j].g / Yi), tonemap.saturation);
            float Bo = Yo * glm::pow((input_img[i][j].b / Yi), tonemap.saturation);

            output_img[i][j].r = gammaCorrect(inv_g, Ro);
            output_img[i][j].g = gammaCorrect(inv_g, Go);
            output_img[i][j].b = gammaCorrect(inv_g, Bo);
        }
    }
}
```

## Advanced Lighting

### Directional and Spot Lights
Starter with these two as they are easy to implement just by following the logic and the formulas. 

```cpp
struct DirectionalLight {
	DirectionalLight() = default;
	explicit DirectionalLight(const DirectionalLight_& _dl)
		: direction(_dl.direction), radiance(_dl.radiance), id(_dl.id){}

	glm::vec3 direction;
	glm::vec3 radiance;
	int id;
};

struct SpotLight {
	SpotLight()= default;
	SpotLight(const SpotLight_& _sl)
		:
	position(_sl.position),
	direction(_sl.direction),
	intensity(_sl.intensity),
	coverage_angle(_sl.coverage_angle),
	falloff_angle(_sl.falloff_angle),
	id(_sl.id)
	{};

	glm::vec3 position;
	glm::vec3 direction;
	glm::vec3 intensity;
	float coverage_angle;
	float falloff_angle;
	int id;
};
```
And in the shading loop:

```cpp
	for (const auto& light : scene->light_sources.directional_lights)
	{
		glm::vec3 wi = -glm::normalize(light.direction);
		Ray shadowRay = Ray(rec.point + rec.normal * render_context.shadow_ray_epsilon, wi, ray.time);
		Ray shadowRayPlane = Ray(rec.point
			+ rec.normal
			* static_cast<float>(render_context.shadow_ray_epsilon)
			, wi, ray.time);
		HitRecord shadowRec;
		HitRecord planeShadowRec;
		if (!scene->world->intersect<true>(shadowRay, Interval(render_context.shadow_ray_epsilon, INFINITY), shadowRec)
			&& !hitPlanes(shadowRayPlane, Interval(0, INFINITY), planeShadowRec))
		{
			//diffuse
			color += Color(kd * light.radiance * glm::max(0.0f, glm::dot(rec.normal, wi)));
			//specular
			glm::vec3 wo = (ray.origin - rec.point);
			wo = glm::normalize(wo);

			glm::vec3 h = glm::normalize(wi + wo);
			float specular = std::pow(glm::max(0.0f, glm::dot(rec.normal, h)),
									  mat.phong_exponent);
			color += Color(ks * light.radiance * specular);
		}
	}
	for (const auto& light : scene->light_sources.spot_lights)
	{
	    glm::vec3 wi = light.position - rec.point;
	    double distance = glm::length(wi);
	    wi = glm::normalize(wi);

	    Ray shadowRay(
	        rec.point + rec.normal * render_context.shadow_ray_epsilon,
	        wi,
	        ray.time
	    );

	    Ray shadowRayPlane(
	        rec.point + rec.normal * render_context.shadow_ray_epsilon,
	        wi,
	        ray.time
	    );

	    HitRecord shadowRec;
	    HitRecord planeShadowRec;

	    if (!scene->world->intersect<true>(
	            shadowRay,
	            Interval(render_context.shadow_ray_epsilon, distance),
	            shadowRec)
	        && !hitPlanes(shadowRayPlane, Interval(0, distance), planeShadowRec))
	    {
	        glm::vec3 spotDir = glm::normalize(light.direction);
	        double cosAlpha = glm::dot(-wi, spotDir);

	        double cosCoverage = std::cos(glm::radians(light.coverage_angle * 0.5f));
	        double cosFalloff  = std::cos(glm::radians(light.falloff_angle  * 0.5f));

	        double spotFactor = 0.0;

	        if (cosAlpha >= cosFalloff)
	        {
	            spotFactor = 1.0;
	        }
	        else if (cosAlpha >= cosCoverage)
	        {
	            double s = (cosAlpha - cosCoverage) /
	                       (cosFalloff - cosCoverage);
	            spotFactor = std::pow(s, 4.0);
	        }
	        else
	        {
	            continue;
	        }

	        double attenuation = spotFactor / (distance * distance);

	        double NdotL = std::max(0.0f, glm::dot(rec.normal, wi));
	        color += Color(kd) * Color(light.intensity) * attenuation * NdotL;

	        glm::vec3 wo = glm::normalize(ray.origin - rec.point);
	        glm::vec3 h  = glm::normalize(wi + wo);

	        double spec = std::pow(
	            std::max(0.0f, glm::dot(rec.normal, h)),
	            mat.phong_exponent
	        );

	        color += Color(ks) * Color(light.intensity) * attenuation * spec;
	    }
	}
```

Cube Directional:

![](images/cube_directional.png)

Dragon new with Spot:

![](images/dragon_new_with_spot.png)

Dragon Spotlight MSAA:

![](images/dragon_spot_light_msaa.png)

### Environment Lights
The hardest part of this implementation was handled by the lecture slides as they provide the final equation for the inversion sampling, and all I had to do is just writing couple of equations in the code. However, at first, I had a major bug. 

```cpp
	if (scene->light_sources.env_light.img != nullptr && mat.type != "mirror")
	{
		glm::vec3 n = rec.normal;
		glm::vec3 u = rec.surface_tangents.u;
		glm::vec3 v = rec.surface_tangents.v;
		float pi = glm::pi<float>();

		Color env_contribution(0, 0, 0);
		int samples = 25;

		for (int s = 0; s < samples; ++s)
		{
			float s1 = generateRandomFloat(0, 1);
			float s2 = generateRandomFloat(0, 1);
			glm::vec3 l;
			Color sample_color(0, 0, 0);

			if (scene->light_sources.env_light.sampler == Sampler::uniform)
			{
				l = u * glm::sqrt(1 - s1 * s1) * glm::cos(s2 * 2 * pi) +
					v * glm::sqrt(1 - s1 * s1) * glm::sin(s2 * 2 * pi) + n * s1;

				l = glm::normalize(l);

				float cosTheta = std::max(0.0f, glm::dot(n, l));
				sample_color = Color(lookupEnvMap(scene->light_sources.env_light, l)) * (2.0f * pi * cosTheta);
			}
			else if (scene->light_sources.env_light.sampler == Sampler::cosine)
			{
				l = u * glm::sqrt(s1) * glm::cos(s2 * 2 * pi) +
					v * glm::sqrt(s1) * glm::sin(s2 * 2 * pi) + n * glm::sqrt(1 - s1);

				l = glm::normalize(l);

				float theta = glm::asin(glm::sqrt(s1));
				sample_color = Color(lookupEnvMap(scene->light_sources.env_light, l) * pi / glm::cos(theta));
			}

			Ray envShadowRay(rec.point + n * (float)render_context.shadow_ray_epsilon, l, ray.time);
			HitRecord envRec;

			if (!scene->world->intersect<true>(envShadowRay, Interval(render_context.shadow_ray_epsilon, INFINITY), envRec)
				&& !hitPlanes(envShadowRay, Interval(render_context.shadow_ray_epsilon, INFINITY), envRec))
			{
				env_contribution += sample_color;
			}
		}

		color += env_contribution / (float)samples;
	}
```

The check for material to not be a mirror at the beginning was missing at first. As a result of it, the environment light was being contributed more than once. With every bounce, it is added and as a result of it, I was getting a plain bright mirror sphere. I noticed that and fixed it, then I get plain black mirror sphere (:D). Then I noticed a bug that stayed silent for the whole semester. For the rays that is not reached the max recursion depth and hit nothing, I was returning black, and not environment color/texture. Also after fixing it, it was done. 

Dear Oguz Hocam Latlong PHOT:

![](images/empty_environment_latlong.exr_phot.png)

Dear Oguz Hocam Latlong FILM:

![](images/empty_environment_latlong.exr_film.png)

Dear Oguz Hocam Latlong ACES:

![](images/empty_environment_latlong.exr_aces.png)

Dear Oguz Hocam Probe PHOT:

![](images/empty_environment_light_probe.exr_phot.png)

Dear Oguz Hocam Probe FILM:

![](images/empty_environment_light_probe.exr_film.png)

Dear Oguz Hocam Probe ACES:

![](images/empty_environment_light_probe.exr_aces.png)

Glass Sphere:

![](images/glass_sphere_env.exr_phot.png)

Mirror Sphere:

![](images/mirror_sphere_env.exr_phot.png)

Sphere Env Light:

![](images/sphere_env_light.exr_phot.png)

Sphere Point PHOT:

![](images/sphere_point_hdr_texture.exr_phot.png)

Sphere Point FILM:

![](images/sphere_point_hdr_texture.exr_film.png)

Sphere Point ACES:

![](images/sphere_point_hdr_texture.exr_aces.png)

#### Head Env Light
God knows why even though my smooth shading code block are triggered when rendering and it works for older scenes, this scene is not being rendered smooth.

![](images/head_env_light.exr_phot.png)

#### Veach Ajar
The reason I couldn't render the veahc ajar scene at all in the previous phase, is because the input format that I didn't handle. The transformations was given composite and not translation, rotation, and scaling. Was this case written in somewhere of the previous pdfs? If that was the case I should have missed it. 

Here is the previous Veach Ajar: 

![](images/VeachAjar.png)

As you can see it is darkish than expected and this is from the previous homework, not something related to the tonemapping or whatsoever. This darkish appearance insists in this phase's veach_ajar as well. As the question I asked about the degamma in the forum answered in the 5th Jan, I couldn't do that adjustment, still it is now the answer, it just made the overall appearance brighter, but colors are still darkish. Come to think of it, degamma couldn't be the answer as the previous veach_ajar is also bugged. My guess is I have a wrong-doing in the parsing stage. 

Here are the not degamma adjusted (as the submitted version is like this) veach_ajars:

VeachAjar.exr_aces_key_0_18_s1_2_burn_0:

![](images/VeachAjar.exr_aces_key_0_18_s1_2_burn_0.png)

VeachAjar.exr_aces_key_0_18_s1_2_burn_1:

![alt text](images/VeachAjar.exr_aces_key_0_18_s1_2_burn_1.png)

VeachAjar.exr_film_key_0_18_s1_2_burn_0:

![alt text](images/VeachAjar.exr_film_key_0_18_s1_2_burn_0.png)

VeachAjar.exr_film_key_0_18_s1_2_burn_1:

![alt text](images/VeachAjar.exr_film_key_0_18_s1_2_burn_1.png) 

VeachAjar.exr_phot_key_0_09_s1_0_burn_1:

![alt text](images/VeachAjar.exr_phot_key_0_09_s1_0_burn_1.png) 

VeachAjar.exr_phot_key_0_18_s1_0_burn_0:

![alt text](images/VeachAjar.exr_phot_key_0_18_s1_0_burn_0.png)

VeachAjar.exr_phot_key_0_18_s1_0_burn_1:

![alt text](images/VeachAjar.exr_phot_key_0_18_s1_0_burn_1.png) 

VeachAjar.exr_phot_key_0_18_s1_2_burn_1:

![alt text](images/VeachAjar.exr_phot_key_0_18_s1_2_burn_1.png)

And here are the degamma adjusted versions: 

VeachAjar.exr_aces_key_0_18_s1_2_burn_0:

![](images/degamma_veach_ajar/VeachAjar.exr_aces_key_0_18_s1_2_burn_0.png)

VeachAjar.exr_aces_key_0_18_s1_2_burn_1:

![alt text](images/degamma_veach_ajar/VeachAjar.exr_aces_key_0_18_s1_2_burn_1.png)

VeachAjar.exr_film_key_0_18_s1_2_burn_0:

![alt text](images/degamma_veach_ajar/VeachAjar.exr_film_key_0_18_s1_2_burn_0.png)

VeachAjar.exr_film_key_0_18_s1_2_burn_1:

![alt text](images/degamma_veach_ajar/VeachAjar.exr_film_key_0_18_s1_2_burn_1.png) 

VeachAjar.exr_phot_key_0_09_s1_0_burn_1:

![alt text](images/degamma_veach_ajar/VeachAjar.exr_phot_key_0_09_s1_0_burn_1.png) 

VeachAjar.exr_phot_key_0_18_s1_0_burn_0:

![alt text](images/degamma_veach_ajar/VeachAjar.exr_phot_key_0_18_s1_0_burn_0.png)

VeachAjar.exr_phot_key_0_18_s1_0_burn_1:

![alt text](images/degamma_veach_ajar/VeachAjar.exr_phot_key_0_18_s1_0_burn_1.png) 

VeachAjar.exr_phot_key_0_18_s1_2_burn_1:

![alt text](images/degamma_veach_ajar/VeachAjar.exr_phot_key_0_18_s1_2_burn_1.png)


#### Teapot Roughness
It is rendered but somehow different than the expected.

![](images/teapot_roughness.exr_phot.png)

#### Cars
I couldn't render the cars. I started rendering and even in one hour, it didn't finish. I thought there must be a mistake and noticed that they could have degenaret triangles. I handled the case and and rerender and it was the same. I don't think the current speed of my raytracer is that bad. Compared to the rendering times of Erencan Ceyhan's blogpost from the previous years, other scenes didn't have that much of a drastic difference. Still, I made the numSamples 1 and tried to render but it was still not finishing. Couldn't fix it yet. 


## Benchmarks

Rendered on Intel i5 10th Gen 10750H

| Scene                         | Rendering Time (s) |
|------------------------------|--------------------|
| Teapot Roughness             | 711                |
| Dragon New Ply Spot          | 1.82               |
| Veach Ajar                   | 83.7               |
| Cube Directional             | 0.18               |
| Cube Point                   | 0.22               |
| Cube Point HDR               | 0.71               |
| Dragon Spotlight MSAA        | 4.09               |
| Empty Environment Latlong    | 0.41               |
| Empty Environment Light Probe| 0.38               |
| Glass Sphere Env             | 0.45               |
| Head Env Light               | 90.7               |
| Mirror Sphere Env            | 0.43               |
| Sphere Point HDR             | 0.93               |
| Sphere Env Light             | 250                |
