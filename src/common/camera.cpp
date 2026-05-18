#include "camera.hpp"
#include "glm/gtx/string_cast.hpp"
#include <iostream>

namespace M3D_RASTER_DIFF
{
	Camera::Camera()
	{
		_computeViewMatrix();
		_computeProjectionMatrix();
	}

	void Camera::setPosition( const glm::vec3 & p_position )
	{
		_position = p_position;
		_computeViewMatrix();
	}

	void Camera::setLookAt( const glm::vec3 & p_lookAt )
	{
		_invDirection = glm::normalize( _position - p_lookAt );
		_pitch		  = glm::clamp( glm::degrees( glm::asin( _invDirection.y ) ), -89.f, 89.f );
		_yaw		  = glm::degrees( glm::atan( _invDirection.z, _invDirection.x ) );
		_updateVectors();
	}

	void Camera::setFovy( const float p_fovy )
	{
		_fovy = p_fovy;
		_computeProjectionMatrix();
	}

	void Camera::setScreenSize( const int p_width, const int p_height )
	{
		_screenWidth  = p_width;
		_screenHeight = p_height;
		_aspectRatio  = float( _screenWidth ) / _screenHeight;
		_updateVectors();
		_computeViewMatrix();
		_computeProjectionMatrix();
	}


	void Camera::moveFront( const float p_delta )
	{
		_position -= _invDirection * p_delta;
		_computeViewMatrix();
	}

	void Camera::moveRight( const float p_delta )
	{
		_position += _right * p_delta;
		_computeViewMatrix();
	}

	void Camera::moveUp( const float p_delta )
	{
		_position += _up * p_delta;
		_computeViewMatrix();
	}

	void Camera::rotate( const float p_yaw, const float p_pitch )
	{
		_yaw   = glm::mod( _yaw + p_yaw, 360.f );
		_pitch = glm::clamp( _pitch + p_pitch, -89.f, 89.f );
		_updateVectors();
	}

	void Camera::print() const
	{
		std::cout << "======== Camera ========" << std::endl;
		std::cout << "Position: " << glm::to_string( _position ) << std::endl;
		std::cout << "View direction: " << glm::to_string( -_invDirection ) << std::endl;
		std::cout << "Right: " << glm::to_string( _right ) << std::endl;
		std::cout << "Up: " << glm::to_string( _up ) << std::endl;
		std::cout << "Yaw: " << _yaw << std::endl;
		std::cout << "Pitch: " << _pitch << std::endl;
		std::cout << "========================" << std::endl;
	}

	void Camera::_computeViewMatrix()
	{
		_viewMatrix = glm::lookAt( _position, _position - _invDirection, _up );
		// Note: glm::lookAt expects the camera position, the target position (where the camera looks at),
		// and the up vector to compute the view matrix.
	}

	void Camera::_computeProjectionMatrix()
	{
		_projectionMatrix = glm::perspective( glm::radians( _fovy ), _aspectRatio, _zNear, _zFar );
		// Note: glm::perspective expects the field of view in radians, aspect ratio, near and far clipping planes.
		//std::cout << "Projection matrix: " << glm::to_string( _projectionMatrix ) << std::endl;
	}

	void Camera::_updateVectors()
	{
		const float yaw	  = glm::radians( _yaw );
		const float pitch = glm::radians( _pitch );
		_invDirection	  = glm::normalize(
			glm::vec3( glm::cos( yaw ) * glm::cos( pitch ), glm::sin( pitch ), glm::sin( yaw ) * glm::cos( pitch ) ) );
		_right = glm::normalize( glm::cross( glm::vec3( 0.f, 1.f, 0.f ), _invDirection ) ); // We suppose 'y' as world up.
		_up	   = glm::normalize( glm::cross( _invDirection, _right ) );

		_computeViewMatrix();
	}

} // namespace M3D_RASTER_DIFF