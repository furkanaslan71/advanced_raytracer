#include "../include/ray.h"

Ray::Ray() {}

Ray::~Ray() {}

Ray::Ray(const Vec3& _origin, const Vec3& _direction) 
  : origin(_origin), 
  direction(_direction)
{
    direction.normalize();
}
