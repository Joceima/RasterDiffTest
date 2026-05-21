#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#define TINYOBJLOADER_IMPLEMENTATION
#include "tiny_obj_loader.h"

#include <iostream>
#include <vector>
#include <fstream>
#include <sstream> 
#include "utils/read_file.hpp"
#include "common/camera.hpp"

#include <random>
#include <glm/gtc/random.hpp>

#include "cuda_gl_interop.hpp"


using namespace glm;
using namespace M3D_RASTER_DIFF;

const unsigned int _windowWidth = 512;
const unsigned int _windowHeight =  512;

class Vertex {
    public:
        glm::vec3 position;
        glm::vec3 normal;
        glm::vec2 texCoord;
};

struct MeshData {
    std::vector<Vertex> vertices;
    std::vector<unsigned int> indices;
};

GLuint VAO;
GLuint VBO;
GLuint _program;
std::string _shaderFolder = "src/shaders/";
glm::vec3 _boxCenter;
glm::vec3 _boxExtents;

// ==========================================
// 0. UTILITAIRE
// ==========================================
// aide de gemini pour remplacer le unity.random
// il utilise une distribution uniforme pour générer des valeurs aléatoires 
// https://en.cppreference.com/cpp/numeric/random/uniform_real_distribution
float randomFloat()
{
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<> dis(0.0, 1.0);
    float randomValue = dis(gen);
    std::cout << "Random float : " << randomValue << std::endl;
    return randomValue;
}

glm::vec3 randomVec3Cube() {
    return glm::vec3(randomFloat() * 2.0f - 1.0f,
                    randomFloat() * 2.0f - 1.0f,
                    randomFloat() * 2.0f - 1.0f                    
    );
}


// ==========================================
// 1. CHARGEMENT DU MESH
// ==========================================
std::vector<Vertex> load3dModel(const std::string& modelPath)
{
    // https://blog.42yeah.is/rendering/2023/04/08/tinyobjloader.html
    tinyobj::attrib_t attributes;
    std::vector<tinyobj::shape_t> shapes;
    std::vector<tinyobj::material_t> materials;
    std::string warnings;
    std::string errors;

    bool ret = tinyobj::LoadObj(&attributes, &shapes, &materials, &warnings, &errors, modelPath.c_str(), "");

    if(!ret) {
        std::cerr<< "Erreur de chargement du mesh :" << errors << std::endl;
    }

    std::vector<Vertex> vertices;

    for(int i = 0 ; i < shapes.size(); i++)
    {
        tinyobj::shape_t &shape = shapes[i];
        tinyobj::mesh_t &mesh = shape.mesh;

        for(int j = 0; j < mesh.indices.size(); j++)
        {
            tinyobj::index_t idx  = mesh.indices[j];
            glm::vec3 position = {
                attributes.vertices[idx.vertex_index * 3],
                attributes.vertices[idx.vertex_index * 3 + 1],
                attributes.vertices[idx.vertex_index * 3 + 2]
            };

            glm::vec3 normal = {
                attributes.normals[idx.normal_index * 3],
                attributes.normals[idx.normal_index * 3 + 1],
                attributes.normals[idx.normal_index * 3 + 2]
            };

            glm::vec2 texCoord = {
                attributes.texcoords[idx.texcoord_index * 2],
                attributes.texcoords[idx.texcoord_index * 2 + 1]
            };

            Vertex vert = {position, normal, texCoord};
            vertices.push_back(vert);
        } 
    }
    return vertices;
}

void computeBoundingBox(const std::vector<Vertex>& vertices, glm::vec3& boxCenter, glm::vec3& boxExtents)
{
    if(vertices.empty())
    {
        boxCenter = glm::vec3(0.0f);
        boxExtents = glm::vec3(0.0f);
        return;
    }

    glm::vec3 minBounds = vertices[0].position;
    glm::vec3 maxBounds = vertices[0].position;

    for(const auto vert : vertices )
    {
        minBounds = glm::min(vert.position, minBounds);
        maxBounds = glm::max(vert.position, maxBounds);
    }

    boxCenter = (minBounds + maxBounds) * 0.5f;

    boxExtents = maxBounds - minBounds;

}


int init()
{
    std::string modelPath = "data/bunny/Bunny.obj";
    //std::string modelMtlBaseDir = "data/Bunny.mtl";

    std::vector<Vertex> bunnyVertices = load3dModel(modelPath);

   
    computeBoundingBox(bunnyVertices, _boxCenter, _boxExtents);

    if(bunnyVertices.empty())
    {
        std::cerr << "Pas d'objet chargé" << std::endl;
        return false;
    }

    //initCamera();

    // render
    glGenVertexArrays(1, &VAO);
    glBindVertexArray(VAO);

    glGenBuffers(1, &VBO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(Vertex) * bunnyVertices.size(), &bunnyVertices[0], GL_STATIC_DRAW);
    
    // attribut 0 : position
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(float) * 8, nullptr);
    
    // attribut 1 : normale
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(float) * 8, (void *) (sizeof(float) * 3));
   
    //attribut 2 : coordonnées de texture
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(float) * 8, (void *) (sizeof(float) * 6));
    
    const std::string vertexShaderStr = readFile(_shaderFolder + "simple_shader.vert");
    const std::string fragmentShaderStr = readFile(_shaderFolder + "simple_shader.frag");

    GLuint vertexShader = glCreateShader( GL_VERTEX_SHADER );
    GLuint fragmentShader = glCreateShader( GL_FRAGMENT_SHADER);

    const GLchar * vSrc = vertexShaderStr.c_str();
    const GLchar * fSrc = fragmentShaderStr.c_str();

    glShaderSource(vertexShader, 1, &vSrc, NULL);
    glShaderSource(fragmentShader, 1, &fSrc, NULL);

    glCompileShader(vertexShader);
    glCompileShader(fragmentShader);


    // Check if compilation is ok .
		// COMPILATION des SHADERS
		GLint compiled;
		glGetShaderiv( vertexShader, GL_COMPILE_STATUS, &compiled );
		if ( !compiled )
		{
			GLchar log[ 1024 ];
			glGetShaderInfoLog( vertexShader, sizeof( log ), NULL, log );
			glDeleteShader( vertexShader );
			glDeleteShader( fragmentShader );
			std ::cerr << " Error compiling vertex shader : " << log << std ::endl;
		}

    _program = glCreateProgram();

    glAttachShader(_program, vertexShader);
    glAttachShader(_program, fragmentShader);
    glLinkProgram(_program);

    // ETAPE LINK
		// Check if link is ok .
	GLint linked;
	glGetProgramiv( _program, GL_LINK_STATUS, &linked );
	if ( !linked )
	{
		GLchar log[ 1024 ];
		glGetProgramInfoLog( _program, sizeof( log ), NULL, log );
		std ::cerr << " Error linking program : " << log << std ::endl;
	}

    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    glUseProgram(_program);

    std::cout << "Done" << std::endl;
    return bunnyVertices.size();
}

void render(int vertexCount, glm::mat4 mvpMatrix)
{
    glClearColor(0.2f, 0.2f, 0.2f, 1.0f);
    glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glEnable(GL_DEPTH_TEST);

    glUseProgram(_program);

    GLuint MatrixID = glGetUniformLocation(_program, "MVP");
    glUniformMatrix4fv(MatrixID, 1, GL_FALSE, glm::value_ptr(mvpMatrix));


    glBindVertexArray(VAO);
    glDrawArrays(GL_TRIANGLES, 0, vertexCount);
}


/*
// fonction du projet unity 
public void RandomizeCameraView()
	{
		Bounds targetBounds = mesh3DSceneBounds;
		float distance = targetBounds.extents.magnitude;
		distance *= (randomViewZoomRange.x + UnityEngine.Random.value * (randomViewZoomRange.y - randomViewZoomRange.x));
		cameraOptim.transform.position = targetBounds.center + UnityEngine.Random.onUnitSphere * distance;
		float3 lookAtCenter = new float3(targetBounds.center) + (new float3(UnityEngine.Random.value, UnityEngine.Random.value, UnityEngine.Random.value) * 2.0f - 1.0f) * targetBounds.extents * 0.5f;
		cameraOptim.transform.LookAt(lookAtCenter, UnityEngine.Random.onUnitSphere);
	}
*/

// Générer par Gemini - à changer 
glm::mat4 _randomizeCameraView(const glm::vec3 boxCenter, const glm::vec3 boxExtents, const float screenWidth, const float screenHeight)
{
    float baseDistance = glm::length(boxExtents);

    float zoomMin = 1.1f;
    float zoomMax = 2.5f;
    float currentZoom = zoomMin + randomFloat() * (zoomMax - zoomMin);
    float finalDistance = baseDistance * currentZoom;

    glm::vec3 cameraPosition = boxCenter + glm::sphericalRand(1.0f) * finalDistance;
    glm::vec3 lookAtCenter = boxCenter + (randomVec3Cube() * boxExtents * 0.5f);
    glm::vec3 cameraUp = glm::sphericalRand(1.0f);

    if(glm::abs(glm::dot(cameraUp, glm::normalize(lookAtCenter - cameraPosition))) > 0.99f)
    {
        cameraUp = glm::vec3(0.0f, 1.0f, 0.0f);
    }

    glm::mat4 Projection = glm::perspective(glm::radians(45.0f), (float) _windowWidth/_windowHeight, 0.1f, 100.0f);
    glm::mat4 View = glm::lookAt(cameraPosition, lookAtCenter, cameraUp);
    glm::mat4 Model = glm::mat4(1.0);

    return Projection * View * Model;
}

int main()
{
    // https://www.opengl-tutorial.org/beginners-tutorials/tutorial-1-opening-a-window/
    if( !glfwInit())
    {
        std::cerr << "Failed to initialize GLFW. " << std::endl;
        return -1;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4); // We want OpenGL 3.3
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 5);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE); // We don't want the old OpenGL 

    // Open a window and create its OpenGL context
    GLFWwindow* window; // (In the accompanying source code, this variable is global for simplicity)
    window = glfwCreateWindow( _windowWidth, _windowHeight, "Tutorial 01", NULL, NULL);
    if( window == NULL ){
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window); // Initialize GLEW
    glewExperimental=true; // Needed in core profile
    if (glewInit() != GLEW_OK) {
        return -1;
    }

    int numVertices = init();
    if (numVertices == 0)
    {
        std::cerr << "Erreur : aucun sommet à dessiner" << std::endl;
        return -1;
    }

    // CAMERA
    Camera _camera;
    _camera.setScreenSize(_windowWidth, _windowHeight);
    _camera.setFovy(45.0f);
    _camera.setPosition(glm::vec3(0.0f, 1.0f, 5.0f));
    _camera.setLookAt(glm::vec3(0.0f, 0.0f, 0.0f));

    glm::mat4 Projection = _camera.getProjectionMatrix();
    glm::mat4 View = _camera.getViewMatrix();
    glm::mat4 Model = glm::mat4(1.0f);

    // Matrice finale = Projection * View * Model
    glm::mat4 MVP = Projection * View * Model;


    // optimisation loop
    //int optimisationStep = 10;
    //for(int i = 0; i < optimisation; i++)
    //{
    //    MVP = _randomizeCameraView(_boxCenter, _boxExtents, _windowWidth, _windowHeight);
    //    render(numVertices, MVP);
    //}

    M3D_RASTER_DIFF::CudaGLInterop interopManager(_windowWidth, _windowHeight);
    interopManager.init();

    // https://www.opengl-tutorial.org/beginners-tutorials/tutorial-1-opening-a-window/
    glfwSetInputMode(window, GLFW_STICKY_KEYS, GL_TRUE);
    do{
        glBindFramebuffer(GL_FRAMEBUFFER, interopManager.getFboId());
        
        glViewport(0, 0, _windowWidth, _windowHeight);
        glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        
        glBindFramebuffer(GL_FRAMEBUFFER, 0);


        if(glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS)
        {
            std::cout << "Space is pressed" << std::endl;
            MVP = _randomizeCameraView(_boxCenter, _boxExtents, _windowWidth, _windowHeight);
        }
        render(numVertices, MVP);
        
        interopManager.captureFrame();
        uchar4* devPixels = interopManager.get<uchar4>();
        interopManager.unmap();
        
        // Swap buffers
        glfwSwapBuffers(window);
        glfwPollEvents();

    } 
    while( glfwGetKey(window, GLFW_KEY_ESCAPE ) != GLFW_PRESS &&
           glfwWindowShouldClose(window) == 0 );
    return 0;
    
}