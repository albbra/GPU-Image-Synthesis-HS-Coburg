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
    // Compute the scale and translation needed to normalize the AABB
    gims::f32v3 size = m_upperRightTop - m_lowerLeftBottom;
    gims::f32v3 center = (m_upperRightTop + m_lowerLeftBottom) * 0.5f;

    gims::f32v3 scale = gims::f32v3(1.0f) / size;

    gims::f32m4 normalizationMatrix(1.0f);
    normalizationMatrix = glm::scale(normalizationMatrix, scale);
    normalizationMatrix = glm::translate(normalizationMatrix, -center);

    return normalizationMatrix;
}

AABB AABB::getUnion(const AABB& other) const
{
  // Compute the new lower-left corner by taking the minimum of the two lower-left corners
  gims::f32v3 newLowerLeft = glm::min(m_lowerLeftBottom, other.m_lowerLeftBottom);

  // Compute the new upper-right corner by taking the maximum of the two upper-right corners
  gims::f32v3 newUpperRight = glm::max(m_upperRightTop, other.m_upperRightTop);

  // Create an array with the two points: lower-left and upper-right corners
  gims::f32v3 unionPoints[2] = {newLowerLeft, newUpperRight};

  // Return a new AABB created from these points
  return AABB(unionPoints, 2); // Passing the array and number of points
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
    // Transform the lower-left corner and upper-right corner of the bounding box
    gims::f32v3 transformedLowerLeft = transformation * gims::f32v4(m_lowerLeftBottom, 1.0f);
    gims::f32v3 transformedUpperRight = transformation * gims::f32v4(m_upperRightTop, 1.0f);

    // Create an array with the two transformed points: lower-left and upper-right corners
    gims::f32v3 transformedPoints[2] = {transformedLowerLeft, transformedUpperRight};

    // Return a new AABB created from the transformed corners
    return AABB(transformedPoints, 2); // Pass the transformed points to the constructor
}
