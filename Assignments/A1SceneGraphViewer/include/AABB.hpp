// AABB.hpp
#ifndef AXIS_ALIGNED_BOUNDING_BOX_CLASS
#define AXIS_ALIGNED_BOUNDING_BOX_CLASS

#include <gimslib/types.hpp>

/// <summary>
/// This class implmements an Axis-Aligned Bounding-Box (AABB).
/// </summary>
class AABB
{
public:
  /// <summary>
  /// Creates an invalid bounding box.
  /// </summary>
  AABB();
  AABB(const AABB& other)                = default;
  AABB(AABB&& other) noexcept            = default;
  AABB& operator=(const AABB& other)     = default;
  AABB& operator=(AABB&& other) noexcept = default;

  /// <summary>
  /// Computes a bounding box from the provided array of 3D positions.
  /// </summary>
  /// <param name="positions">Array of 3D positions.</param>
  /// <param name="nPositions">Number of positions.</param>
  AABB(gims::f32v3 const* const positions, gims::ui32 nPositions);

  /// <summary>
  /// Creates a bounding box from the provided 3D positions.
  /// </summary>
  /// <param name="lowerLeft">3D position</param>
  /// <param name="upperRight">3D position</param>
  AABB(const gims::f32v3& lowerLeft, const gims::f32v3& upperRight);

  /// <summary>
  /// Returns the affine matrix, that maps the bounding box to [-0.5...0.5]^3.
  /// </summary>
  /// <returns>A 4x4 affine matrix.</returns>
  gims::f32m4 getNormalizationTransformation() const;

  /// <summary>
  /// Computes and returns the union of this bounding box with the other bounding box.
  /// </summary>
  /// <param name="other">The other bounding box.</param>
  /// <returns>The union of two bounding boxes.</returns>
  AABB getUnion(const AABB& other) const;

  /// <summary>
  /// Returns the lower, left, bottom corner of the bounding box.
  /// </summary>
  /// <returns></returns>
  const gims::f32v3& getLowerLeftBottom() const;

  /// <summary>
  /// Returns the upper, right, top corner of the bounding box.
  /// </summary>
  /// <returns></returns>
  const gims::f32v3& getUpperRightTop() const;

  /// <summary>
  /// Returns a transformed copy of this bounding box.
  /// The corner points of the bounding box are transformed by the given matrix.
  /// </summary>
  /// <param name="transformation">A matrix that transforms points.</param>
  /// <returns>The transformed matrix.</returns>
  AABB getTransformed(gims::f32m4& transformation) const;

private:
  //! The lower left bottom corner of the AABB.
  gims::f32v3 m_lowerLeftBottom;
  //! The upper right top of the AABB.
  gims::f32v3 m_upperRightTop;
};
#endif // AXIS_ALIGNED_BOUNDING_BOX_CLASS
