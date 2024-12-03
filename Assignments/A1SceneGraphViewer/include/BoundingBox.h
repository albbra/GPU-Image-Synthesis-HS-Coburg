// BoundingBox.h
#ifndef BOUNDING_BOX_CLASS
#define BOUNDING_BOX_CLASS

#include <d3d12.h>
#include <gimslib/types.hpp>
#include <array>
#include <wrl.h>
#include "TriangleMeshD3D12.hpp"

/// <summary>
/// A Bounding Box with always 8 Positions from AABB of the Mesh
/// </summary>
class BoundingBox
{
public:
	/// <summary>
	/// Constructor that creates a bounding box from the mesh's AABB.
	/// </summary>
	/// <param name="mesh">The mesh to derive the bounding box from.</param>
	/// <param name="device">Device for GPU buffer creation.</param>
	/// <param name="commandQueue">Command queue for uploading data to the GPU.</param>
	BoundingBox(TriangleMeshD3D12 mesh,
		const Microsoft::WRL::ComPtr<ID3D12Device>& device,
		const Microsoft::WRL::ComPtr<ID3D12CommandQueue>& commandQueue);

	/// <summary>
	/// Adds the commands necessary for rendering this bounding box to the provided commandList.
	/// </summary>
	/// <param name="commandList">The command list.</param>
	void addToCommandList(const Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList>& commandList) const;

	BoundingBox(const BoundingBox& other) = default;
	BoundingBox(BoundingBox&& other) noexcept = default;
	BoundingBox& operator=(const BoundingBox& other) = default;
	BoundingBox& operator=(BoundingBox&& other) noexcept = default;

	/// <summary>
    /// Returns the input element descriptors required for the pipeline.
    /// </summary>
    /// <returns>The input element descriptor.</returns>
	static const std::vector<D3D12_INPUT_ELEMENT_DESC>& getInputElementDescriptors();

private:
	std::array<gims::f32v3, 8> m_positions;              //! The 8 corner points of the bounding box.
	gims::ui32                 m_nIndices;               //! Number of indices in the index buffer.
	gims::ui32                 m_vertexBufferSize;       //! Vertex buffer size in bytes.
	gims::ui32                 m_indexBufferSize;        //! Index buffer size in bytes.
	Microsoft::WRL::ComPtr<ID3D12Resource> m_vertexBuffer; //! The vertex buffer on the GPU.
	D3D12_VERTEX_BUFFER_VIEW   m_vertexBufferView;       //! Vertex buffer view.
	Microsoft::WRL::ComPtr<ID3D12Resource> m_indexBuffer; //! The index buffer on the GPU.
	D3D12_INDEX_BUFFER_VIEW    m_indexBufferView;        //! Index buffer view.

	//! Input element descriptor defining the vertex format.
	static const std::vector<D3D12_INPUT_ELEMENT_DESC> m_inputElementDescs;
};
#endif // BOUNDING_BOX_CLASS
