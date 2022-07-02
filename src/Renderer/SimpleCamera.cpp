#include "SimpleCamera.h"
#include <iostream>

namespace tim
{
    void SimpleCamera::update(float _time)
    {
        const float g_RotationMouseSpeed = 0.01f;
        const float g_CameraSpeed = m_boost ? 5 : 1;

        vec3 horizontalAxis = linalg::cross(dir, up);

        if (m_forward)
            pos += dir * _time * g_CameraSpeed;
        if (m_backward)
            pos -= dir * _time * g_CameraSpeed;
        if (m_left)
            pos -= horizontalAxis * _time * g_CameraSpeed;
        if (m_right)
            pos += horizontalAxis * _time * g_CameraSpeed;

        if (m_leftMouseButton)
        {
            vec3 horizontalAxis = linalg::cross(dir, up);
            vec4 horizontalAxisRot = linalg::rotation_quat(horizontalAxis, m_mouseDy * g_RotationMouseSpeed);
            dir = linalg::qrot(horizontalAxisRot, dir);
            // up = linalg::qrot(horizontalAxisRot, up);

            vec4 verticalAxisRot = linalg::rotation_quat(up, -m_mouseDx * g_RotationMouseSpeed);
            dir = linalg::qrot(verticalAxisRot, dir);

            m_mouseDx = 0;
            m_mouseDy = 0;
        }

        m_viewMat = linalg::view_matrix(pos, pos + dir, up);
    }
}