#ifndef __CUDA_GL_INTEROP_HPP
#define __CUDA_GL_INTEROP_HPP

#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <cuda_runtime.h>
#include <cuda_gl_interop.h>

#include <iostream>

namespace M3D_RASTER_DIFF
{
    class CudaGLInterop {
        public:
            CudaGLInterop(unsigned int width, unsigned int height );
            ~CudaGLInterop();

        bool init();

        bool initVboTest(unsigned int nbTrianglesInSoup);

        void captureFrame();

        template<typename Type>
        Type *getDevPixels()
        {
            return reinterpret_cast<Type*>(m_mappedPtr);
        }

        // problème ici, impossible de mapper la ressource pour le vbo 
        template<typename Type>
        Type *getDevVertices()
        {
            return reinterpret_cast<Type*>(m_vboMappedPtr);
        }

        void mapAll(); // test correction gemini 
        void unmapAll();
  

        void checkResult();

        GLuint getFboId() const { return m_fbo;};
        GLuint getTexture() const { return m_fboTexture;}
        GLuint getVboTrianglesSoupId() const { return m_vboTrianglesSoup; }

        private:
            void initGLBuffers();
            bool initCudaInterop();

            void initGLBuffersTriangleTest(unsigned int nbTrianglesInSoup);
            bool initCudaInteropTriangleTest();

        private: 
            unsigned int m_height;
            unsigned int m_width;

            GLuint m_fbo = 0;
            GLuint m_fboTexture = 0;
            GLuint m_rboDepth = 0; 
            GLuint m_pbo = 0;

            cudaGraphicsResource_t cudaPboResource = nullptr;
            void* m_mappedPtr = nullptr;

            GLuint m_vboTrianglesSoup;
            cudaGraphicsResource_t cudaVboResource = nullptr;
            void* m_vboMappedPtr = nullptr;

    };
}

#endif // __CUDA_GL_INTEROP_HPP