#ifndef __CAMERA_HPP__
#define __CAMERA_HPP__

#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

namespace M3D_RASTER_DIFF
{
	// Freefly camera.
	class Camera
	{
	  public:
		Camera();

		inline const glm::mat4 & getViewMatrix() const { return _viewMatrix; }
		inline const glm::mat4 & getProjectionMatrix() const { return _projectionMatrix; }

		void setPosition( const glm::vec3 & p_position );
		void setLookAt( const glm::vec3 & p_lookAt );
		void setFovy( const float p_fovy );

		glm::vec3 getPosition() const
		{ return _position;
		}

		void setScreenSize( const int p_width, const int p_height );

		void moveFront( const float p_delta );
		void moveRight( const float p_delta );
		void moveUp( const float p_delta );
		void rotate( const float p_yaw, const float p_pitch );

		void print() const;


	  private:
		void _computeViewMatrix();
		void _computeProjectionMatrix();
		void _updateVectors();


	  private:
		glm::vec3 _position		= glm::vec3(0.f, 0.f, 0.f);
		glm::vec3 _invDirection = glm::vec3( 0.f, 0.f, 1.f );  // Dw dans le cours.
		glm::vec3 _right		= glm::vec3( -1.f, 0.f, 0.f ); // Rw dans le cours.
		glm::vec3 _up			= glm::vec3( 0.f, 1.f, 0.f );  // Uw dans le cours.
		// Angles defining the orientation in degrees
		float _yaw	 = 90.f;
		float _pitch = 0.f;

		int	  _screenWidth	= 1280;
		int	  _screenHeight = 720;
		float _aspectRatio	= float( _screenWidth ) / _screenHeight;
		float _fovy			= 60.f;
		float _zNear		= 0.1f;
		float _zFar			= 1000.f;

		glm::mat4 _viewMatrix		= glm::mat4( 1.f );
		glm::mat4 _projectionMatrix = glm::mat4( 1.f );
	};
} // namespace M3D_RASTER_DIFF

#endif // __CAMERA_HPP__