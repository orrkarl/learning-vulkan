#include "MVPTransform.h"

MVPTransform mkTransform(const glm::mat4& model, const glm::mat4& view, const glm::mat4& projection)
{
    return projection * view * model;
}
