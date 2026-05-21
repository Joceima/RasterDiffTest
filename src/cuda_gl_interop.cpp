#include "cuda_gl_interop.hpp"
#include <GL/glew.h>
#include <iostream> 

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
                m_mappedPtr = nullptr;
                cudaGraphicsUnmapResources(1, &cudaPboResource,0);
            }
            cudaGraphicsUnregisterResource(cudaPboResource);
            glDeleteBuffers(1, &m_pbo);
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
    }

        // https://learnopengl.com/Advanced-OpenGL/Framebuffers
    void CudaGLInterop::initGLBuffers()
    {   
        // configuration du fbo
        //glBindTexture(1, m_fbo);
        //glBindFramebuffer(GL_FRAMEBUFFER, m_fbo);
        // CORRECTION DE GEMINI 
        glGenFramebuffers(1, &m_fbo);
        glBindFramebuffer(GL_FRAMEBUFFER, m_fbo);
        // voir si je dois mettre l'accès objet en version 4.5 ? 
        // attacher unrue texture 
        unsigned int texture;
        glGenTextures(1, &texture);
        glBindTexture(GL_TEXTURE_2D, texture);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, m_width, m_height, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        // attacher la texture du frame buffer
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, texture, 0);
    
        // che pas comparer après si le code fonctionne
        // cours opengl moteur3D cm6
        // binder fbo sur la cible GL_DRAW_FRAMEBUFFER
        // indique que le FS va écrire dans les textures attachées au FBO
        // ici on ne dessine plus dans le framebuffer par défault, donc pas d'impact sur ce qui est affiché "offscreen rendering"
        //glBindFrameBuffer(GL_DRAW_FRAMEBUFFER, m_fbo);
        //glNamedFramebufferTexture(m_fbo, attachment, textureId, mipmapLevel);
    
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
}
