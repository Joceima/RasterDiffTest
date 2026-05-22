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

        void captureFrame();

        template<typename Type>
        Type *get()
        {
            // le problème vient d'ici 
            if(!m_mappedPtr)
            {
                cudaError_t err = cudaGraphicsMapResources(1, &cudaPboResource, 0);
                if(err != cudaSuccess)
                {
                    std::cerr << "[CUDA Error] Impossible de mapper la ressource : "<< cudaGetErrorString(err) << std::endl;
                }
                cudaGraphicsResourceGetMappedPointer(reinterpret_cast<void**>(&m_mappedPtr), nullptr, cudaPboResource);
            }
            return reinterpret_cast<Type*>(m_mappedPtr);
        }

        void unmap();

        void checkResult();

        GLuint getFboId() const { return m_fbo;};
        GLuint getTexture() const { return m_fboTexture;}

        private:
            void initGLBuffers();
            bool initCudaInterop();

        private: 
            unsigned int m_height;
            unsigned int m_width;

            GLuint m_fbo = 0;
            GLuint m_fboTexture = 0;
            GLuint m_rboDepth = 0; 
            GLuint m_pbo = 0;

            cudaGraphicsResource_t cudaPboResource = nullptr;
            void* m_mappedPtr = nullptr;

    };
}

#endif // __CUDA_GL_INTEROP_HPP