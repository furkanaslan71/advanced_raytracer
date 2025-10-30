#ifndef BVH_H
#define BVH_H

#include "hittable.h"
#include <memory>
#include <vector>
#include <algorithm>
#include <random>

class BvhNode : public Hittable{
public:
	BvhNode();
  BvhNode(std::vector<std::shared_ptr<Hittable>>& objects, int begin, int end);

  bool hit(const Ray& ray, Interval ray_t, HitRecord& rec) const override;

  AABB getAABB() const override;

private:
    std::shared_ptr<Hittable> left;
    std::shared_ptr<Hittable> right;
    AABB bounding_box;
};

#endif // !BVH_H