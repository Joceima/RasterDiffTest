NVCC = nvcc 

LIBS = -lGLEW -lglfw -lGL 

TARGET = rasterizer

INCLUDES = -I. -I./include -I./src/utils -I./src/common

SRC = src/main.cpp src/common/camera.cpp

all: 
	$(NVCC) $(INCLUDES) $(SRC) -o $(TARGET) $(LIBS) $(UTILS)

clean: 
	rm -f $(TARGET)