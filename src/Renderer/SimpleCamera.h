#pragma once
#include "timCore/type.h"

namespace tim
{
    class SimpleCamera
    {
    public:
        SimpleCamera() = default;

        void setDirection(bool _left, bool _right, bool _forward, bool _backward, bool _boost)
        {
            m_left = _left; m_right = _right; m_forward = _forward; m_backward = _backward;
            m_boost = _boost;
        }
        void setPosition(vec3 _pos)
        {
            pos = _pos;
        }
        void setMouseDelta(float _dx, float _dy)
        {
            m_mouseDx = -_dx; m_mouseDy = _dy;
        }
        void setMouseButton(bool _leftButton)
        {
            m_leftMouseButton = _leftButton;
        }

        void update(float _time);

        const mat4& getViewMat() const { return m_viewMat; }
        const vec3& getPos() const { return pos; }
        const vec3& getDir() const { return dir; }
        const vec3& getUp() const { return up; }

    private:
        bool m_left = false, m_right = false, m_forward = false, m_backward = false;
        bool m_leftMouseButton = false, m_boost = false;
        float m_mouseDx = 0, m_mouseDy = 0;

        vec3 pos = { 0,0,1.5 };
        vec3 dir = { -1,0,0 };
        vec3 up = { 0,0,1 };
        mat4 m_viewMat = {};
    };
}