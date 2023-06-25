#pragma once
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>
#include <glm/gtc/quaternion.hpp>

class Camera
{
public:
    glm::vec3 position;
    glm::quat orientation;
    glm::mat4 projection;
    glm::mat4 view;
    bool is_tracking_camera = false;

    Camera(float fov, float width, float height);
    void updateVP();
    glm::mat4 getV();
    glm::mat4 getVP();
    void translate(glm::vec3 v);
    void onMouseMove(float xRel, float yRel);
    void moveFront(float amount);
    void moveRight(float amount);
    void moveDown(float amount);
    void rotate(float amount);
    void updateScreenSize(float width, float height);
    const glm::vec3& getPosition() const;
    float getNear() const;
    float getFar() const;

private:
    glm::mat4 vp;
    glm::vec3 u, v, w;
    float near, far;
    float yaw, pitch, roll;
    float fov;
    const float mouse_sensitivity = 0.25f;
};
