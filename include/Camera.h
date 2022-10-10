#pragma once
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/ext/matrix_clip_space.hpp>
#include <glm/ext/matrix_transform.hpp>
#include <glm/glm.hpp>

class Camera
{
public:
    glm::vec3 position;
    glm::mat4 projection;
    glm::mat4 view;

    Camera(float fov, float width, float height) : fov(fov), yaw(90.0f), pitch(0.0f), near(0.1f), far(1000.0f)
    {
        projection = glm::perspective(glm::radians(fov), width / height, near, far);
        position = glm::vec3(0.0f, 0.0f, -2.0f);
        up = glm::vec3(0.0f, -1.0f, 0.0f);
        onMouseMove(0.0f, 0.0f);
    }

    const glm::mat4& getVP() const
    {
        return vp;
    }

    void translate(glm::vec3 v)
    {
        position += v;
        view = glm::translate(view, -1.0f * v);
        update();
    }

    void onMouseMove(float xRel, float yRel)
    {
        yaw += xRel * mouse_sensitivity;
        pitch += yRel * mouse_sensitivity;
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
        look_at = glm::normalize(front);
        update();
    }

    void moveFront(float amount)
    {
        translate(glm::normalize(glm::vec3(1.0f, 0.0f, 1.0f) * look_at) * amount);
        update();
    }

    void moveSideways(float amount)
    {
        translate(glm::normalize(glm::cross(look_at, up)) * amount);
        update();
    }

    void moveDown(float amount)
    {
        translate(up * amount);
    }

    void updateScreenSize(float width, float height)
    {
        projection = glm::perspective(glm::radians(fov), width / height, 0.1f, 1000.0f);
        update();
    }

    const glm::vec3& getPosition() const
    {
        return position;
    }

    float getNear() const
    {
        return near;
    }

    float getFar() const
    {
        return far;
    }

private:
    glm::mat4 vp;
    float near;
    float far;
    float yaw;
    float pitch;
    const float mouse_sensitivity = 0.25f;
    glm::vec3 look_at;
    glm::vec3 up;
    float fov;

    void update()
    {
        view = glm::lookAt(position, position + look_at, up);
        vp = projection * view;
    }
};