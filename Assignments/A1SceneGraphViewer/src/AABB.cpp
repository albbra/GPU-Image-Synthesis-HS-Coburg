// AABB.cpp

#include "AABB.hpp"

AABB::AABB()
    : m_lowerLeftBottom(std::numeric_limits<gims::f32>::max())
    , m_upperRightTop(-std::numeric_limits<gims::f32>::max())
{
}

AABB::AABB(gims::f32v3 const* const positions, gims::ui32 nPositions)
    : m_lowerLeftBottom(std::numeric_limits<gims::f32>::max())
    , m_upperRightTop(-std::numeric_limits<gims::f32>::max())

{
  for (gims::ui32 i = 0; i < nPositions; i++)
  {
    const glm::vec3& p = positions[i];
    m_lowerLeftBottom  = glm::min(m_lowerLeftBottom, p);
    m_upperRightTop    = glm::max(m_upperRightTop, p);
  }
}

AABB::AABB(const gims::f32v3& lowerLeft, const gims::f32v3& upperRight)
    : m_lowerLeftBottom(lowerLeft)
    , m_upperRightTop(upperRight)
{
}

gims::f32m4 AABB::getNormalizationTransformation() const
{
  gims::f32v3     centroid          = (m_lowerLeftBottom + m_upperRightTop) / gims::f32(2);
  glm::highp_mat4 translationMatrix = glm::translate(glm::mat4(1.0f), -centroid);

  gims::f32v3     axesLengths       = m_upperRightTop - m_lowerLeftBottom;
  gims::f32       longestAxisLength = std::max(axesLengths.x, std::max(axesLengths.y, axesLengths.z));
  gims::f32v3     scalingFactors    = (glm::vec3(1.0f) / longestAxisLength);
  glm::highp_mat4 scalingMatrix     = glm::scale(glm::mat4(1.0f), scalingFactors);

  return scalingMatrix * translationMatrix;
}

AABB AABB::getUnion(const AABB& other) const
{
  return {glm::min(m_lowerLeftBottom, other.m_lowerLeftBottom), glm::max(m_upperRightTop, other.m_upperRightTop)};
}

const gims::f32v3& AABB::getLowerLeftBottom() const
{
  return m_lowerLeftBottom;
}

const gims::f32v3& AABB::getUpperRightTop() const
{
  return m_upperRightTop;
}

AABB AABB::getTransformed(gims::f32m4& transformation) const
{
  return {transformation * gims::f32v4(m_lowerLeftBottom, 1.0f), transformation * gims::f32v4(m_upperRightTop, 1.0f)};
}
