#include "Camera.hpp"

#include <glm/ext/matrix_clip_space.hpp>
#include <glm/ext/matrix_transform.hpp>

Camera::Camera(float fov, float width, float height) : fov(fov), yaw(90.0f), pitch(0.0f), near(0.1f), far(10000.0f)
{
    projection = glm::perspective(glm::radians(fov), width / height, near, far);
    position = glm::vec3(0.0f, 0.0f, -2.0f);
    up = glm::vec3(0.0f, -1.0f, 0.0f);
    onMouseMove(0.0f, 0.0f);
}

const glm::mat4& Camera::getVP() const
{
    return vp;
}

void Camera::translate(glm::vec3 amount)
{
    position += amount;
    view = glm::translate(view, -1.0f * amount);
    update();
}

void Camera::onMouseMove(float xRel, float yRel)
{
    yaw -= xRel * mouse_sensitivity;
    pitch -= yRel * mouse_sensitivity;
    if (pitch > 89.0f)
    {
        pitch = 89.0f;
    }
    if (pitch < -89.0f)
    {
        pitch = -89.0f;
    }
    glm::vec3 front;
    front.x = cos(glm::radians(pitch)) * cos(glm::radians(yaw));
    front.y = sin(glm::radians(pitch));
    front.z = cos(glm::radians(pitch)) * sin(glm::radians(yaw));
    w = -glm::normalize(front);
    u = glm::normalize(glm::cross(up, w));
    v = glm::normalize(glm::cross(w, u));
    update();
}

void Camera::moveFront(float amount)
{
    translate((-w) * amount);
    update();
}

void Camera::moveRight(float amount)
{
    translate(glm::normalize(u) * amount);
    update();
}

void Camera::moveDown(float amount)
{
    translate(v * amount);
}

void Camera::updateScreenSize(float width, float height)
{
    projection = glm::perspective(glm::radians(fov), width / height, near, far);
    update();
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

void Camera::update()
{
    view = glm::lookAt(position, position - w, up);
    vp = projection * view;
}
