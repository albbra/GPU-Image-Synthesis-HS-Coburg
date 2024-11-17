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
    const auto& p     = positions[i];
    m_lowerLeftBottom = glm::min(m_lowerLeftBottom, p);
    m_upperRightTop   = glm::max(m_upperRightTop, p);
  }
}
gims::f32m4 AABB::getNormalizationTransformation() const
{
  // Assignment 2
  return gims::f32m4(1);
}
AABB AABB::getUnion(const AABB& other) const
{
  (void)other;
  // Assignment 2
  return *this;
}
const gims::f32v3& AABB::getLowerLeftBottom()
{
  return m_lowerLeftBottom;
}
const gims::f32v3& AABB::getUpperRightTop() const
{
  return m_upperRightTop;
}
AABB AABB::getTransformed(gims::f32m4& transformation) const
{
  (void)transformation;
  // Assignment 2
  return *this;
}
