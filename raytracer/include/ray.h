#ifndef RAY_H
#define RAY_H

#include "vec3.h"

class Ray {
public:
    Vec3 origin;
    Vec3 direction;
    Ray();
    ~Ray();
    Ray(const Vec3& _origin, const Vec3& _direction);
};
#endif //RAY_H
