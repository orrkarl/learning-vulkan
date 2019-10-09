#pragma once

#include <glm/glm.hpp>

struct MVPTransform
{
public:
    glm::mat4 model;
    glm::mat4 view;
    glm::mat4 projection;

    glm::mat4 transform() const
    {
        return projection * view * model;
    }
};
