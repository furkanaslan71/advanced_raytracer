# Derleyici olarak g++ kullanılacak
CXX = g++

# Derleyiciye verilecek bayraklar (flags)
# -std=c++20: C++20 standardını etkinleştirir.
# -g: Hata ayıklama (debugging) bilgilerini ekler.
# -Wall -Wextra: Olası tüm hatalar için uyarıları gösterir.
CXXFLAGS = -std=c++20 -g -Wall -Wextra

# Başlık dosyalarının (.h) bulunduğu klasörler
# Derleyiciye #include edilen dosyaları nerede arayacağını söyler.
INCLUDES = -Iinclude \
           -Iio \
           -Ilight \
           -Imaterial \
           -Irender \
           -Iobjects \
           -Isrc \
           -Icamera \
           -Iscene

# Oluşturulacak olan çalıştırılabilir dosyanın adı
TARGET = raytracer

# Derlenecek olan tüm .cpp kaynak dosyalarınız
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
          objects/plane.cpp

# Kaynak dosyalarından (.cpp) nesne dosyaları (.o) oluştur
# $(SOURCES:.cpp=.o) ifadesi, SOURCES listesindeki tüm .cpp uzantılarını .o ile değiştirir.
OBJECTS = $(SOURCES:.cpp=.o)

# Makefile'ın varsayılan hedefi 'all' olarak belirlenmiştir.
# Sadece 'make' komutu çalıştırıldığında bu hedef tetiklenir.
.PHONY: all clean

all: $(TARGET)

# Ana hedef (çalıştırılabilir dosya) oluşturma kuralı
# Bu kural, tüm nesne dosyalarını (.o) birbirine bağlayarak (linking)
# çalıştırılabilir dosyayı oluşturur.
$(TARGET): $(OBJECTS)
	$(CXX) $(CXXFLAGS) -o $(TARGET) $(OBJECTS)

# Kaynak dosyalarından (.cpp) nesne dosyalarını (.o) derleme kuralı
# Bu kural, her bir .cpp dosyasını nasıl .o dosyasına çevireceğini tanımlar.
# -c: Sadece derle, bağlama (link) yapma.
# -o $@: Çıktıyı hedefin adıyla ($@ -> örneğin main.o) oluştur.
# $<: İlk bağımlılığın adını ($< -> örneğin main.cpp) al.
%.o: %.cpp
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c -o $@ $<

# 'make clean' komutu çalıştırıldığında tetiklenecek hedef
# Derleme sırasında oluşturulan tüm dosyaları temizler.
clean:
	rm -f $(TARGET) $(OBJECTS)
