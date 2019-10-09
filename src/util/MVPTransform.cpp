#include "MVPTransform.h"

glm::mat4 MVPTransform::transform() const
{
    return projection * view * model;
}
