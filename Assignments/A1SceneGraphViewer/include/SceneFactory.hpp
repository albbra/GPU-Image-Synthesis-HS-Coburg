// SceneFactory.hpp
#ifndef SCENE_FACTORY_CLASS
#define SCENE_FACTORY_CLASS

#include "Scene.hpp"
#include <filesystem>
#include <unordered_map>

struct aiScene;
struct aiNode;

class SceneGraphFactory
{
public:
  static Scene createFromAssImpScene(const std::filesystem::path                       pathToScene,
                                     const Microsoft::WRL::ComPtr<ID3D12Device>&       device,
                                     const Microsoft::WRL::ComPtr<ID3D12CommandQueue>& commandQueue);

private:
  static void createMeshes(aiScene const* const inputScene, const Microsoft::WRL::ComPtr<ID3D12Device>& device,
                           const Microsoft::WRL::ComPtr<ID3D12CommandQueue>& commandQueue, Scene& outputScene);

  static gims::ui32 createNodes(aiScene const* const inputScene, Scene& outputScene, aiNode const* const inputNode);

  static void computeSceneAABB(Scene& scene, AABB& aabb, gims::ui32 nodeIdx, gims::f32m4 transformation);

  static void createTextures(const std::unordered_map<std::filesystem::path, gims::ui32>& textureFileNameToTextureIndex,
                             std::filesystem::path parentPath, const Microsoft::WRL::ComPtr<ID3D12Device>& device,
                             const Microsoft::WRL::ComPtr<ID3D12CommandQueue>& commandQueue, Scene& outputScene);

  static void createMaterials(aiScene const* const                                  inputScene,
                              std::unordered_map<std::filesystem::path, gims::ui32> textureFileNameToTextureIndex,
                              const Microsoft::WRL::ComPtr<ID3D12Device>& device, Scene& outputScene);
};

#endif // SCENE_FACTORY_CLASS
