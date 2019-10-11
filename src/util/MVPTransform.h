#pragma once

#include <glm/glm.hpp>

using MVPTransform = glm::mat4;

MVPTransform mkTransform(const glm::mat4& model, const glm::mat4& view, const glm::mat4& projection);
