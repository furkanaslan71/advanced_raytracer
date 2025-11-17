CXX = g++

CXXFLAGS = -std=c++20 -O3 -g -Wall -Wextra

INCLUDES = -Iinclude \
           -Iio \
           -Ilight \
           -Imaterial \
           -Irender \
           -Iobjects \
           -Isrc \
           -Icamera \
           -Iscene

TARGET = raytracer

SOURCES = camera/camera.cpp \
          src/parser.cpp \
          src/ray.cpp \
          src/aabb.cpp \
          render/render_manager.cpp \
          src/main.cpp \
          material/material_manager.cpp \
          render/base_ray_tracer.cpp \
          src/bvh.cpp \
          scene/scene.cpp \
          material/material.cpp \
          objects/plane.cpp \
          objects/hittable_instance.cpp \
          objects/mesh_instance.cpp \
          objects/base_mesh.cpp

OBJECTS = $(SOURCES:.cpp=.o)

.PHONY: all clean

all: $(TARGET)


$(TARGET): $(OBJECTS)
	$(CXX) $(CXXFLAGS) -o $(TARGET) $(OBJECTS)

%.o: %.cpp
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c -o $@ $<

clean:
	rm -f $(TARGET) $(OBJECTS)
