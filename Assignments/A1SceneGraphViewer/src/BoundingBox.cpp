#include "BoundingBox.h"
#include <d3dx12/d3dx12.h>
#include <gimslib/d3d/UploadHelper.hpp>
#include <stdexcept>

const std::vector<D3D12_INPUT_ELEMENT_DESC> BoundingBox::m_inputElementDescs = {
	{"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0} };

BoundingBox::BoundingBox(TriangleMeshD3D12 mesh,
	const Microsoft::WRL::ComPtr<ID3D12Device>& device,
	const Microsoft::WRL::ComPtr<ID3D12CommandQueue>& commandQueue)
	: m_nIndices(24) // 12 edges × 2 vertices per edge
	, m_vertexBufferSize(sizeof(gims::f32v3) * 8)
	, m_indexBufferSize(sizeof(gims::ui32) * 24)
{
	// Get lower-left-bottom and upper-right-top points
	gims::f32v3 lowerLeftBottom = mesh.getAABB().getLowerLeftBottom();
	gims::f32v3 upperRightTop = mesh.getAABB().getUpperRightTop();

	// Calculate the 8 corner points of the bounding box
	m_positions = {
		lowerLeftBottom,                                        // 0: LLB
		{upperRightTop.x, lowerLeftBottom.y, lowerLeftBottom.z}, // 1: LRB
		{lowerLeftBottom.x, upperRightTop.y, lowerLeftBottom.z}, // 2: LTF
		{upperRightTop.x, upperRightTop.y, lowerLeftBottom.z},   // 3: URB
		{lowerLeftBottom.x, lowerLeftBottom.y, upperRightTop.z}, // 4: LLT
		{upperRightTop.x, lowerLeftBottom.y, upperRightTop.z},   // 5: LRT
		{lowerLeftBottom.x, upperRightTop.y, upperRightTop.z},   // 6: ULF
		upperRightTop                                         // 7: URT
	};

	// Edge indices for drawing the bounding box as lines
	std::array<gims::ui32, 24> indices = {
		0, 1, 0, 2, 0, 4, // Edges from LLB
		1, 3, 1, 5,       // Edges from LRB
		2, 3, 2, 6,       // Edges from LTF
		3, 7,             // Edges from URB
		4, 5, 4, 6,       // Edges from LLT
		5, 7,             // Edges from LRT
		6, 7              // Edges from ULF
	};

	// Instantiate UploadHelper
	gims::UploadHelper uploadHelper(device, std::max(m_vertexBufferSize, m_indexBufferSize));

	// Vertex buffer creation and upload
	const CD3DX12_RESOURCE_DESC vertexBufferDesc = CD3DX12_RESOURCE_DESC::Buffer(m_vertexBufferSize);
	const CD3DX12_HEAP_PROPERTIES heapProps(D3D12_HEAP_TYPE_DEFAULT);

	if (FAILED(device->CreateCommittedResource(&heapProps, D3D12_HEAP_FLAG_NONE, &vertexBufferDesc,
		D3D12_RESOURCE_STATE_COMMON, nullptr, IID_PPV_ARGS(&m_vertexBuffer))))
	{
		throw std::runtime_error("Failed to create vertex buffer resource.");
	}

	m_vertexBufferView.BufferLocation = m_vertexBuffer->GetGPUVirtualAddress();
	m_vertexBufferView.SizeInBytes = m_vertexBufferSize;
	m_vertexBufferView.StrideInBytes = sizeof(gims::f32v3);

	uploadHelper.uploadBuffer(m_positions.data(), m_vertexBuffer, m_vertexBufferSize, commandQueue);

	// Index buffer creation and upload
	const CD3DX12_RESOURCE_DESC indexBufferDesc = CD3DX12_RESOURCE_DESC::Buffer(m_indexBufferSize);

	if (FAILED(device->CreateCommittedResource(&heapProps, D3D12_HEAP_FLAG_NONE, &indexBufferDesc,
		D3D12_RESOURCE_STATE_COMMON, nullptr, IID_PPV_ARGS(&m_indexBuffer))))
	{
		throw std::runtime_error("Failed to create index buffer resource.");
	}

	m_indexBufferView.BufferLocation = m_indexBuffer->GetGPUVirtualAddress();
	m_indexBufferView.SizeInBytes = m_indexBufferSize;
	m_indexBufferView.Format = DXGI_FORMAT_R32_UINT;

	uploadHelper.uploadBuffer(indices.data(), m_indexBuffer, m_indexBufferSize, commandQueue);
}

void BoundingBox::addToCommandList(const Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList>& commandList) const
{
	if (!commandList)
	{
		throw std::invalid_argument("Command list is null.");
	}

	// Set buffers and topology
	commandList->IASetVertexBuffers(0, 1, &m_vertexBufferView);
	commandList->IASetIndexBuffer(&m_indexBufferView);
	commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_LINELIST);

	// Issue draw command
	commandList->DrawIndexedInstanced(m_nIndices, 1, 0, 0, 0);
}

const std::vector<D3D12_INPUT_ELEMENT_DESC>& BoundingBox::getInputElementDescriptors()
{
	return m_inputElementDescs;
}
