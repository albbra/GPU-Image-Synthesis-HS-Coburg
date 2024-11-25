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

AABB::AABB(const gims::f32v3& lowerLeft, const gims::f32v3& upperRight)
    : m_lowerLeftBottom(lowerLeft)
    , m_upperRightTop(upperRight)
{
}

gims::f32m4 AABB::getNormalizationTransformation() const
{
  gims::f32v3 size = m_upperRightTop - m_lowerLeftBottom;
  size             = glm::max(size, gims::f32v3(1e-6f)); // Avoid division by zero

  gims::f32v3 center = (m_upperRightTop + m_lowerLeftBottom) * 0.5f;
  gims::f32v3 scale  = gims::f32v3(1.0f) / size;

  return glm::translate(glm::scale(gims::f32m4(1.0f), scale), -center);
}

AABB AABB::getUnion(const AABB& other) const
{
  return AABB(glm::min(m_lowerLeftBottom, other.m_lowerLeftBottom), glm::max(m_upperRightTop, other.m_upperRightTop));
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
  // Define all 8 corners of the bounding box
  const gims::f32v3 corners[8] = {m_lowerLeftBottom,
                                  {m_upperRightTop.x, m_lowerLeftBottom.y, m_lowerLeftBottom.z},
                                  {m_lowerLeftBottom.x, m_upperRightTop.y, m_lowerLeftBottom.z},
                                  {m_lowerLeftBottom.x, m_lowerLeftBottom.y, m_upperRightTop.z},
                                  {m_upperRightTop.x, m_upperRightTop.y, m_lowerLeftBottom.z},
                                  {m_upperRightTop.x, m_lowerLeftBottom.y, m_upperRightTop.z},
                                  {m_lowerLeftBottom.x, m_upperRightTop.y, m_upperRightTop.z},
                                  m_upperRightTop};

  // Initialize transformed bounds
  gims::f32v3 transformedLowerLeft(std::numeric_limits<gims::f32>::max());
  gims::f32v3 transformedUpperRight(-std::numeric_limits<gims::f32>::max());

  // Transform each corner and update bounds
  for (const gims::f32v3& corner : corners)
  {
    gims::f32v3 transformedCorner = gims::f32v3(transformation * gims::f32v4(corner, 1.0f));
    transformedLowerLeft          = glm::min(transformedLowerLeft, transformedCorner);
    transformedUpperRight         = glm::max(transformedUpperRight, transformedCorner);
  }

  return AABB(transformedLowerLeft, transformedUpperRight);
}
