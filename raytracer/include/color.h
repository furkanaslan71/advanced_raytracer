#ifndef COLOR_H
#define COLOR_H
#include "vec3.h"

class Color {
public:
  double r, g, b;

  Color() : r(0), g(0), b(0) {}
  Color(double _r, double _g, double _b) : r(_r), g(_g), b(_b) {}
  Color clamp()
  {
    static double max = 255.0f;
    if (r > max) r = max;
    if (g > max) g = max;
    if (b > max) b = max;
    if (r < 0) r = 0;
    if (g < 0) g = 0;
    if (b < 0) b = 0;
    return *this;
  }

  Color(const Color& other)
  {
    r = other.r;
    g = other.g;
    b = other.b;
  }

  Color(const Vec3& vec)
  {
    r = vec.x;
    g = vec.y;
    b = vec.z;
  }


  inline Color operator*=(const Color& other)
  {
    r = r * other.r;
    g = g * other.g;
    b = b * other.b;
    return *this;
  }

  inline Color operator+=(const Color& other)
  {
    r = r + other.r;
    g = g + other.g;
    b = b + other.b;
    return *this;
  }

  inline Color operator+(const Color& c) const
  {
    return Color(r + c.r, g + c.g, b + c.b);
  }

  inline Color operator-(const Color& c) const
  {
    return Color(r - c.r, g - c.g, b - c.b);
  }

  inline Color operator*(const Color& c) const
  {
    return Color(r * c.r, g * c.g, b * c.b);
  }

  inline Color operator*(double s) const
  {
    return Color(r * s, g * s, b * s);
  }

};

#endif // COLOR_H