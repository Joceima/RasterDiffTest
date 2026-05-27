NVCC = nvcc 

LIBS = -lGLEW -lglfw -lGL 

TARGET = rasterizer

INCLUDES = -I. -I./include -I./src/utils -I./src/common -I./src

SRC = src/main.cpp src/common/camera.cpp src/cuda_gl_interop.cpp src/cuda/triangle_soup.cu

all: 
	$(NVCC) $(INCLUDES) $(SRC) -o $(TARGET) $(LIBS) $(UTILS)

clean: 
	rm -f $(TARGET)