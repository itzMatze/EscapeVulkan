#include "Camera.hpp"

#include <glm/ext/matrix_clip_space.hpp>
#include <glm/ext/matrix_transform.hpp>

inline constexpr glm::vec3 front(0.0f, 0.0f, -1.0f);
inline constexpr glm::vec3 right(1.0f, 0.0f, 0.0f);
inline constexpr glm::vec3 up(0.0f, -1.0f, 0.0f);

Camera::Camera(float fov, float width, float height) : fov(fov), yaw(0.0f), pitch(0.0f), roll(0.0f), near(0.1f), far(10000.0f), position(0.0f, 0.0f, 3.0f)
{
    projection = glm::perspective(glm::radians(fov), width / height, near, far);
    orientation = glm::quatLookAt(glm::normalize(-position), glm::normalize(glm::vec3(0.0f, -1.0f, 0.0f)));
}

void Camera::updateVP()
{
    // rotate initial coordinate system to camera orientation
    glm::quat q_front = glm::normalize(orientation * glm::quat(0.0f, front) * glm::conjugate(orientation));
    glm::quat q_right = glm::normalize(orientation * glm::quat(0.0f, right) * glm::conjugate(orientation));
    glm::quat q_up = glm::normalize(orientation * glm::quat(0.0f, up) * glm::conjugate(orientation));
    w = glm::normalize(glm::vec3(q_front.x, q_front.y, q_front.z));
    u = glm::normalize(glm::vec3(q_right.x, q_right.y, q_right.z));
    v = glm::normalize(glm::vec3(q_up.x, q_up.y, q_up.z));

    // calculate incremental change in angles with respect to camera coordinate system
    glm::quat q_pitch = glm::angleAxis(glm::radians(pitch), u);
    glm::quat q_yaw = glm::angleAxis(glm::radians(yaw), v);
    glm::quat q_roll = glm::angleAxis(glm::radians(roll), w);

    // apply incremental change to camera orientation
    orientation = glm::normalize(q_yaw * q_pitch * q_roll * orientation);

    // calculate view matrix
    glm::quat revers_orient = glm::conjugate(orientation);
    glm::mat4 rotation = glm::mat4_cast(revers_orient);
    view = glm::translate(rotation, -position);

    // reset angles as the changes were applied to camera orientation
    pitch = 0;
    yaw = 0;
    roll = 0;
    vp = projection * view;
}

glm::mat4 Camera::getVP()
{
    return is_tracking_camera ? projection * glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.0f, -10.0f)) * view : vp;
}

void Camera::translate(glm::vec3 amount)
{
    position += amount;
    view = glm::translate(view, amount);
}

void Camera::onMouseMove(float xRel, float yRel)
{
    yaw += xRel * mouse_sensitivity;
    pitch += yRel * mouse_sensitivity;
}

void Camera::moveFront(float amount)
{
    translate(amount * w);
}

void Camera::moveRight(float amount)
{
    translate(amount * u);
}

void Camera::moveDown(float amount)
{
    translate(up * amount);
}

void Camera::rotate(float amount) 
{
    roll -= amount;
}

void Camera::updateScreenSize(float width, float height)
{
    projection = glm::perspective(glm::radians(fov), width / height, near, far);
}

const glm::vec3& Camera::getPosition() const
{
    return position;
}

float Camera::getNear() const
{
    return near;
}

float Camera::getFar() const
{
    return far;
}
