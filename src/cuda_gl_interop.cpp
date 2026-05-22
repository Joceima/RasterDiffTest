#include "cuda_gl_interop.hpp"
#include <GL/glew.h>
#include <iostream> 
#include <vector>

namespace M3D_RASTER_DIFF
{
    CudaGLInterop::CudaGLInterop(unsigned int width, unsigned int height) : m_width(width), m_height(height)
    {
        std::cout << "width : " << m_width << std::endl;
        std::cout << "height : " << m_height << std::endl;
    }

    CudaGLInterop::~CudaGLInterop()
    {
        if(m_fbo != GL_INVALID_INDEX)
        {
            glDeleteFramebuffers(1, &m_fbo);
        }
        if(m_fboTexture != GL_INVALID_INDEX)
        {
            glDeleteTextures(1, &m_fboTexture);
        }
        if(m_rboDepth != GL_INVALID_INDEX)
        {
            glDeleteRenderbuffers(1, &m_rboDepth);
        }
        CudaGLInterop::unmap();
    }

    void CudaGLInterop::unmap()
    {
        if(glIsBuffer(m_pbo))
        {
            if(m_mappedPtr)
            {
                cudaGraphicsUnmapResources(1, &cudaPboResource,0);
                m_mappedPtr = nullptr;
            }
            // error invalid ressource handle
            //cudaGraphicsUnregisterResource(cudaPboResource);
            //glDeleteBuffers(1, &m_pbo);
        }
    }

    bool CudaGLInterop::init()
    {
        initGLBuffers();
        return CudaGLInterop::initCudaInterop();
    }

    // gemini
    void CudaGLInterop::captureFrame()
    {
        //glViewport(0, 0, m_width, m_height);
        glBindFramebuffer(GL_READ_FRAMEBUFFER, m_fbo);
        glBindBuffer(GL_PIXEL_PACK_BUFFER, m_pbo);
        glReadPixels(0, 0, m_width, m_height, GL_RGBA, GL_UNSIGNED_BYTE, 0);
        glBindBuffer(GL_PIXEL_PACK_BUFFER, 0);
        glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);
        glFinish();
    }

        // https://learnopengl.com/Advanced-OpenGL/Framebuffers
    void CudaGLInterop::initGLBuffers()
    {   
        glGenFramebuffers(1, &m_fbo);
        glBindFramebuffer(GL_FRAMEBUFFER, m_fbo);
        // voir si je dois mettre l'accès objet en version 4.5 ? 
        glGenTextures(1, &m_fboTexture);
        glBindTexture(GL_TEXTURE_2D, m_fboTexture);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, m_width, m_height, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        // attacher la texture du frame buffer
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_fboTexture, 0);
    
        // configuration du rbo 
        // utilisation du rbo car c'est rapide pour switcher de buffer 
        glGenRenderbuffers(1, &m_rboDepth);
        glBindRenderbuffer(GL_RENDERBUFFER, m_rboDepth);
        glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, m_width, m_height);
        glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, m_rboDepth);
    
        // check si le frame buffer est complet
        if(glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
        {
            std::cout << "ERROR::FRAMEBUFFER:: Framebuffer is not complete !" << std::endl;
        } else {
            std::cout << "Framebuffer is complete !" << std::endl;
        }
    
        // initialisation du PBO - passerelle cuda opengl 
        glGenBuffers(1, &m_pbo);
        glBindBuffer(GL_PIXEL_PACK_BUFFER, m_pbo);
        glBufferData(GL_PIXEL_PACK_BUFFER, m_width * m_height * 4, NULL, GL_DYNAMIC_READ );
        
        // ici debinder
        glBindBuffer(GL_PIXEL_PACK_BUFFER, 0);
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
    }
    
    // côté host 
    // le problème vient d'ici 
    // en gros openGL ne se lance pas sur NVIDIA mais sur intel donc CUDA ne trouve rien 
    /*
    --- DIAGNOSTIC GPU ---
    OpenGL Vendor : Intel
    OpenGL Renderer : Mesa Intel(R) Graphics (ARL)
    ----------------------

    pour résoudre le problème il faut spécifier que l'ensemble de l'application doit s'exécuter sur NVDIA
    en lancant la commande suivante :
    __NV_PRIME_RENDER_OFFLOAD=1 __GLX_VENDOR_LIBRARY_NAME=nvidia ./rasterizer 

    --- DIAGNOSTIC GPU ---
    OpenGL Vendor : NVIDIA Corporation
    OpenGL Renderer : NVIDIA RTX PRO 1000 Blackwell Generation Laptop GPU/PCIe/SSE2
    ----------------------

    */
    bool CudaGLInterop::initCudaInterop()
    {
        cudaError_t err = cudaGraphicsGLRegisterBuffer(&cudaPboResource, m_pbo, cudaGraphicsRegisterFlagsReadOnly);
        if (err != cudaSuccess)
        {
            std::cerr << "Erreur registre Cuda Opengl : " << cudaGetErrorString(err) << std::endl;
            return false; 
        } else {
            std::cout << "Succès de cudaGraphicsGLRegisterBuffer" << std::endl;
            return true;
        }
    }

    void CudaGLInterop::checkResult()
    {
        uchar4* devPtr = this->get<uchar4>();
        if (!devPtr) {
            std::cerr << "[CPU Test] Erreur : Le pointeur CUDA est nul !" << std::endl;
            return;
        }
        std::vector<uchar4> hostPixels(m_width * m_height);

        cudaError_t err = cudaMemcpy(hostPixels.data(), devPtr, m_width * m_height * sizeof(uchar4), cudaMemcpyDeviceToHost);

        if (err != cudaSuccess) {
            std::cerr << "[CPU Test] Erreur cudaMemcpy : " << cudaGetErrorString(err) << std::endl;
            return;
        }

        int nonBlackPixels = 0;
        for (size_t i = 0; i < hostPixels.size(); ++i)
        {
            if (hostPixels[i].x > 0 || hostPixels[i].y > 0 || hostPixels[i].z > 0) {
                nonBlackPixels++;
            }
        }

        std::cout << "[CPU Test] Nombre de pixels colorés détectés par CUDA : " 
                  << nonBlackPixels << " / " << (m_width * m_height) << std::endl;
    }
}
