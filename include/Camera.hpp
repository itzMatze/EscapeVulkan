#pragma once
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>

class Camera
{
public:
    glm::vec3 position;
    glm::mat4 projection;
    glm::mat4 view;

    Camera(float fov, float width, float height);
    const glm::mat4& getVP() const;
    void translate(glm::vec3 v);
    void onMouseMove(float xRel, float yRel);
    void moveFront(float amount);
    void moveRight(float amount);
    void moveDown(float amount);
    void updateScreenSize(float width, float height);
    const glm::vec3& getPosition() const;
    float getNear() const;
    float getFar() const;

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

    void update();
};