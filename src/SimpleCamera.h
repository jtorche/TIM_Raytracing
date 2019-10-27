#include "timCore/type.h"

class SimpleCamera
{
public:
    SimpleCamera() = default;

    void setDirection(bool _left, bool _right, bool _forward, bool _backward) 
    { 
        m_left = _left; m_right = _right; m_forward = _forward; m_backward = _backward;
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

    const tim::mat4& getViewMat() const { return m_viewMat; }
    const tim::vec3& getPos() const { return pos; }

private:
    bool m_left = false, m_right = false, m_forward = false, m_backward = false;
    bool m_leftMouseButton = false;
    float m_mouseDx = 0, m_mouseDy = 0;

    tim::vec3 pos = { 5,0,1 };
    tim::vec3 dir = { -1,0,0 };
    tim::vec3 up = { 0,0,1 };
    tim::mat4 m_viewMat = {};
};