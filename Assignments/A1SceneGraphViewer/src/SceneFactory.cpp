// SceneFactory.cpp

#include "SceneFactory.hpp"
#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>
#include <assimp/scene.h>
#include <d3dx12/d3dx12.h>
#include <gimslib/d3d/UploadHelper.hpp>
#include <gimslib/dbg/HrException.hpp>
#include <iostream>

/// <summary>
/// Converts the index buffer required for D3D12 renndering from an aiMesh.
/// </summary>
/// <param name="mesh">The ai mesh containing an index buffer.</param>
/// <returns></returns>
std::vector<gims::ui32v3> static getTriangleIndicesFromAiMesh(aiMesh const* const mesh)
{
  std::vector<gims::ui32v3> result;

  if (!mesh || mesh->mPrimitiveTypes != aiPrimitiveType_TRIANGLE)
  {
    return result; // Ensure the mesh contains only triangles.
  }

  for (unsigned int i = 0; i < mesh->mNumFaces; ++i)
  {
    const aiFace& face = mesh->mFaces[i];
    if (face.mNumIndices == 3)
    { // Ensure the face is a triangle.
      result.emplace_back(gims::ui32v3 {face.mIndices[0], face.mIndices[1], face.mIndices[2]});
    }
  }

  return result;
}

void static addTextureToDescriptorHeap(
    const Microsoft::WRL::ComPtr<ID3D12Device>& device, aiTextureType aiTextureTypeValue, gims::i32 offsetInDescriptors,
    aiMaterial const* const inputMaterial, const std::vector<Texture2DD3D12>& m_textures, Material& material,
    std::unordered_map<std::filesystem::path, gims::ui32> textureFileNameToTextureIndex, gims::ui32 defaultTextureIndex)
{
  if (inputMaterial->GetTextureCount(aiTextureTypeValue) == 0)
  {
    m_textures[defaultTextureIndex].addToDescriptorHeap(device, material.srvDescriptorHeap, offsetInDescriptors);
  }
  else
  {
    aiString path;
    inputMaterial->GetTexture((aiTextureType)aiTextureTypeValue, 0, &path);
    m_textures[textureFileNameToTextureIndex[path.C_Str()]].addToDescriptorHeap(device, material.srvDescriptorHeap,
                                                                                offsetInDescriptors);
  }
}

std::unordered_map<std::filesystem::path, gims::ui32> static textureFilenameToIndex(aiScene const* const inputScene)
{
  std::unordered_map<std::filesystem::path, gims::ui32> textureFileNameToTextureIndex;

  gims::ui32 textureIdx = 3;
  for (gims::ui32 mIdx = 0; mIdx < inputScene->mNumMaterials; mIdx++)
  {
    for (gims::ui32 textureType = aiTextureType_NONE; textureType < aiTextureType_UNKNOWN; textureType++)
    {
      for (gims::ui32 i = 0; i < inputScene->mMaterials[mIdx]->GetTextureCount((aiTextureType)textureType); i++)
      {
        aiString path;
        inputScene->mMaterials[mIdx]->GetTexture((aiTextureType)textureType, i, &path);

        const char* const                                                           texturePathCstr = path.C_Str();
        const std::unordered_map<std::filesystem::path, gims::ui32>::const_iterator textureIter =
            textureFileNameToTextureIndex.find(texturePathCstr);
        if (textureIter == textureFileNameToTextureIndex.end())
        {
          textureFileNameToTextureIndex.emplace(texturePathCstr, static_cast<gims::ui32>(textureIdx));
          textureIdx++;
        }
      }
    }
  }
  return textureFileNameToTextureIndex;
}

/// <summary>
/// Reads the color from the Asset Importer specific (pKey, type, idx) triple.
/// Use the Asset Importer Macros AI_MATKEY_COLOR_AMBIENT, AI_MATKEY_COLOR_DIFFUSE, etc. which map to these arguments
/// correctly.
///
/// If that key does not exist a null vector is returned.
/// </summary>
/// <param name="pKey">Asset importer specific parameter</param>
/// <param name="type"></param>
/// <param name="idx"></param>
/// <param name="material">The material from which we wish to extract the color.</param>
/// <returns>Color or 0 vector if no color exists.</returns>
gims::f32v4 static getColor(char const* const pKey, unsigned int type, unsigned int idx,
                            aiMaterial const* const material)
{
  aiColor3D color;
  if (material->Get(pKey, type, idx, color) == aiReturn_SUCCESS)
  {
    return gims::f32v4(color.r, color.g, color.b, 0.0f);
  }
  else
  {
    return gims::f32v4(0.0f);
  }
}

gims::f32m4 static convertAssimpMatrixToGims(const aiMatrix4x4& assimpMatrix)
{
  return glm::transpose(glm::make_mat4(&assimpMatrix.a1));
}

gims::i32 static getTexture(aiTextureType textureType, unsigned int textureIndex, aiMaterial const* const material,
                            std::unordered_map<std::filesystem::path, gims::ui32> textureFileNameToTextureIndex)
{
  gims::i32 defaultTextureIndexToReturn = 0;
  aiString  textureName("");
  aiReturn  textureRetrievingResult = material->GetTexture(textureType, textureIndex, &textureName);
  if (textureRetrievingResult == aiReturn_FAILURE)
  {
    if (textureType == aiTextureType_AMBIENT)
    {
      defaultTextureIndexToReturn = 1;
    }
    else if (textureType == aiTextureType_DIFFUSE)
    {
      defaultTextureIndexToReturn = 0;
    }
    else if (textureType == aiTextureType_SPECULAR)
    {
      defaultTextureIndexToReturn = 1;
    }
    else if (textureType == aiTextureType_EMISSIVE)
    {
      defaultTextureIndexToReturn = 1;
    }
    else if (textureType == aiTextureType_HEIGHT)
    {
      defaultTextureIndexToReturn = 2;
    }
  }
  else
  {
    defaultTextureIndexToReturn = textureFileNameToTextureIndex.find(textureName.C_Str())->second;
  }
  return defaultTextureIndexToReturn;
}

Scene SceneGraphFactory::createFromAssImpScene(const std::filesystem::path                       pathToScene,
                                               const Microsoft::WRL::ComPtr<ID3D12Device>&       device,
                                               const Microsoft::WRL::ComPtr<ID3D12CommandQueue>& commandQueue)
{
  Scene outputScene;

  const std::filesystem::path absolutePath = std::filesystem::weakly_canonical(pathToScene);
  if (!std::filesystem::exists(absolutePath))
  {
    throw std::exception((absolutePath.string() + std::string(" does not exist.")).c_str());
  }

  const gims::ui32 arguments = aiPostProcessSteps::aiProcess_Triangulate | aiProcess_GenSmoothNormals |
                               aiProcess_GenUVCoords | aiProcess_ConvertToLeftHanded | aiProcess_OptimizeMeshes |
                               aiProcess_RemoveRedundantMaterials | aiProcess_ImproveCacheLocality |
                               aiProcess_FindInvalidData | aiProcess_FindDegenerates;

  Assimp::Importer imp;
  imp.SetPropertyBool(AI_CONFIG_PP_FD_REMOVE, true);
  const aiScene* inputScene = imp.ReadFile(absolutePath.string(), arguments);
  if (!inputScene)
  {
    throw std::exception((absolutePath.string() + std::string(" can't be loaded. with Assimp.")).c_str());
  }
  const std::unordered_map<std::filesystem::path, gims::ui32> textureFileNameToTextureIndex =
      textureFilenameToIndex(inputScene);

  createMeshes(inputScene, device, commandQueue, outputScene);

  createNodes(inputScene, outputScene, inputScene->mRootNode);

  computeSceneAABB(outputScene, outputScene.m_aabb, 0, glm::identity<gims::f32m4>());
  createTextures(textureFileNameToTextureIndex, absolutePath.parent_path(), device, commandQueue, outputScene);
  createMaterials(inputScene, textureFileNameToTextureIndex, device, outputScene);

  return outputScene;
}

void SceneGraphFactory::createMeshes(aiScene const* const                              inputScene,
                                     const Microsoft::WRL::ComPtr<ID3D12Device>&       device,
                                     const Microsoft::WRL::ComPtr<ID3D12CommandQueue>& commandQueue, Scene& outputScene)
{
  if (!inputScene || !device || !commandQueue)
  {
    throw std::invalid_argument("Invalid arguments to createMeshes.");
  }

  // Iterate through all meshes in the scene
  for (unsigned int meshIdx = 0; meshIdx < inputScene->mNumMeshes; ++meshIdx)
  {
    aiMesh* mesh = inputScene->mMeshes[meshIdx];

    if (!mesh || mesh->mPrimitiveTypes != aiPrimitiveType_TRIANGLE)
    {
      continue; // Skip non-triangular meshes
    }

    // Extract vertex data
    std::vector<gims::f32v3> positions(mesh->mNumVertices);
    std::vector<gims::f32v3> normals(mesh->mNumVertices);
    std::vector<gims::f32v3> texCoords(mesh->mNumVertices, gims::f32v3(0.0f)); // Default to 0 if no UVs

    for (unsigned int i = 0; i < mesh->mNumVertices; ++i)
    {
      positions[i] = gims::f32v3(mesh->mVertices[i].x, mesh->mVertices[i].y, mesh->mVertices[i].z);

      if (mesh->HasNormals())
      {
        normals[i] = gims::f32v3(mesh->mNormals[i].x, mesh->mNormals[i].y, mesh->mNormals[i].z);
      }
      else
      {
        normals[i] = gims::f32v3(0.0f, 0.0f, 1.0f);
      }

      if (mesh->HasTextureCoords(0))
      {
        texCoords[i] = gims::f32v3(mesh->mTextureCoords[0][i].x, mesh->mTextureCoords[0][i].y, 0.0f);
      }
      else
      {
        texCoords[i] = gims::f32v3(0.0f, 0.0f, 0.0f);
      }
    }

    // Extract index data using the helper function
    std::vector<gims::ui32v3> indices = getTriangleIndicesFromAiMesh(mesh);

    // Determine material index
    gims::ui32 materialIndex = mesh->mMaterialIndex;

    // Create TriangleMeshD3D12 and add it to the scene's mesh list
    outputScene.m_meshes.emplace_back(positions.data(), normals.data(), texCoords.data(),
                                      static_cast<gims::ui32>(positions.size()), indices.data(),
                                      static_cast<gims::ui32>(indices.size() * 3), materialIndex, device, commandQueue);
  }
}

gims::ui32 SceneGraphFactory::createNodes(aiScene const* const inputScene, Scene& outputScene,
                                          aiNode const* const inputNode)
{
  if (!inputScene || !inputNode)
    throw std::invalid_argument("Input scene or node is null.");

  // Create a new node in the Scene
  Node newNode;

  // Convert the node's transformation matrix
  newNode.transformation = convertAssimpMatrixToGims(inputNode->mTransformation);

  // Map the node's meshes
  for (unsigned int i = 0; i < inputNode->mNumMeshes; ++i)
  {
    const unsigned int meshIndex = inputNode->mMeshes[i];
    if (meshIndex >= inputScene->mNumMeshes)
      throw std::out_of_range("Mesh index out of range in inputNode.");

    // Add the mesh index to the node
    newNode.meshIndices.push_back(meshIndex);
  }

  // Add the node to the outputScene's m_nodes array and get its index
  gims::ui32 currentIndex = static_cast<gims::ui32>(outputScene.m_nodes.size());
  outputScene.m_nodes.push_back(newNode);

  // Process child nodes recursively
  for (unsigned int i = 0; i < inputNode->mNumChildren; ++i)
  {
    // Recursively create child nodes and get their index
    gims::ui32 childIndex = createNodes(inputScene, outputScene, inputNode->mChildren[i]);

    // Add the child index to the current node's childIndices
    outputScene.m_nodes[currentIndex].childIndices.push_back(childIndex);
  }

  // Return the index of the newly created node
  return currentIndex;
}

void SceneGraphFactory::computeSceneAABB(Scene& scene, AABB& aabb, gims::ui32 nodeIdx, gims::f32m4 transformation)
{
  // Validierung: Prüfen, ob der Knotenindex gültig ist
  if (nodeIdx >= scene.getNumberOfNodes())
  {
    return;
  }

  // Hole den aktuellen Knoten
  const Node& currentNode = scene.getNode(nodeIdx);

  // Transformation des aktuellen Knotens anwenden
  gims::f32m4 nodeTransformation = transformation * currentNode.transformation;

  // Verarbeite alle Meshes des Knotens
  for (gims::ui32 meshIdx : currentNode.meshIndices)
  {
    // Hole die AABB des Meshes
    const TriangleMeshD3D12& mesh     = scene.getMesh(meshIdx);
    AABB                     meshAABB = mesh.getAABB();

    // Transformiere die AABB mit der aktuellen Transformation
    AABB transformedAABB = meshAABB.getTransformed(nodeTransformation);

    // Füge die transformierte AABB zur globalen AABB hinzu
    aabb = aabb.getUnion(transformedAABB);
  }

  // Rekursiv alle Kindknoten traversieren
  for (gims::ui32 childIdx : currentNode.childIndices)
  {
    computeSceneAABB(scene, aabb, childIdx, nodeTransformation);
  }

  // Ausgabe am Ende
  if (nodeIdx == 0) // Annahme: Root-Knoten hat den Index 0
  {
    const gims::f32v3& lowerLeft  = aabb.getLowerLeftBottom();
    const gims::f32v3& upperRight = aabb.getUpperRightTop();

    // Debug Ausgabe
    std::cout << "AABB Lower Left Bottom: (" << lowerLeft.x << ", " << lowerLeft.y << ", " << lowerLeft.z << ")"
              << std::endl;

    std::cout << "AABB Upper Right Top: (" << upperRight.x << ", " << upperRight.y << ", " << upperRight.z << ")"
              << std::endl;
    std::cout << "\n";
  }
}

void SceneGraphFactory::createTextures(
    const std::unordered_map<std::filesystem::path, gims::ui32>& textureFileNameToTextureIndex,
    std::filesystem::path parentPath, const Microsoft::WRL::ComPtr<ID3D12Device>& device,
    const Microsoft::WRL::ComPtr<ID3D12CommandQueue>& commandQueue, Scene& outputScene)
{

  gims::ui8v4 defaultWhiteTextureData(255, 255, 255, 255);
  gims::ui8v4 defaultBlackTextureData(0, 0, 0, 255);
  gims::ui8v4 defaultNormalMapTextureData(0, 0, 255, 255);

  Texture2DD3D12 defaultWhiteTexture(&defaultWhiteTextureData, 1, 1, device, commandQueue);
  Texture2DD3D12 defaultBlackTexture(&defaultBlackTextureData, 1, 1, device, commandQueue);
  Texture2DD3D12 defaultNormalMapTexture(&defaultNormalMapTextureData, 1, 1, device, commandQueue);

  outputScene.m_textures.resize(textureFileNameToTextureIndex.size() + 3);
  outputScene.m_textures.at(0) = defaultWhiteTexture;
  outputScene.m_textures.at(1) = defaultBlackTexture;
  outputScene.m_textures.at(2) = defaultNormalMapTexture;

  for (const auto& [textureRelativePath, textureIndex] : textureFileNameToTextureIndex)
  {
    Texture2DD3D12 textureToAdd((parentPath / textureRelativePath), device, commandQueue);
    outputScene.m_textures.at(textureIndex) = textureToAdd;
  }
}

void SceneGraphFactory::createMaterials(
    aiScene const* const                                  inputScene,
    std::unordered_map<std::filesystem::path, gims::ui32> textureFileNameToTextureIndex,
    const Microsoft::WRL::ComPtr<ID3D12Device>& device, Scene& outputScene)
{
  if (!inputScene)
  {
    throw std::runtime_error("Invalid input scene provided.");
  }

  // Iterate over all materials in the input scene
  for (unsigned int index = 0; index < inputScene->mNumMaterials; ++index)
  {
    const aiMaterial* aiMat = inputScene->mMaterials[index];

    Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> srv;
    D3D12_DESCRIPTOR_HEAP_DESC                   desc = {};
    desc.Type                                         = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
    desc.NumDescriptors                               = 5;
    desc.NodeMask                                     = 0;
    desc.Flags                                        = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
    if (FAILED(device->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&srv))))
    {
      throw std::runtime_error("Failed to create descriptor heap.");
    }

    // Extract material properties
    gims::f32v4 ambientColor(0.0f);
    gims::f32v4 diffuseColor(0.0f);
    gims::f32v4 emissiveColor(0.0f);
    gims::f32v4 specularColorAndExponent(0.0f);
    float       specularExponent = 1.0f;

    ambientColor             = getColor(AI_MATKEY_COLOR_AMBIENT, aiMat);
    diffuseColor             = getColor(AI_MATKEY_COLOR_DIFFUSE, aiMat);
    emissiveColor            = getColor(AI_MATKEY_COLOR_EMISSIVE, aiMat);
    specularColorAndExponent = getColor(AI_MATKEY_COLOR_SPECULAR, aiMat);

    if (AI_SUCCESS == aiGetMaterialFloat(aiMat, AI_MATKEY_SHININESS, &specularExponent))
    {
      specularColorAndExponent.w = specularExponent;
    }

    gims::i32 ambientTextureIndex   = getTexture(aiTextureType_AMBIENT, 0, aiMat, textureFileNameToTextureIndex);
    gims::i32 diffuseTextureIndex   = getTexture(aiTextureType_DIFFUSE, 0, aiMat, textureFileNameToTextureIndex);
    gims::i32 specularTextureIndex  = getTexture(aiTextureType_SPECULAR, 0, aiMat, textureFileNameToTextureIndex);
    gims::i32 emissiveTextureIndex  = getTexture(aiTextureType_EMISSIVE, 0, aiMat, textureFileNameToTextureIndex);
    gims::i32 normalMapTextureIndex = getTexture(aiTextureType_HEIGHT, 0, aiMat, textureFileNameToTextureIndex);

    outputScene.m_textures.at(ambientTextureIndex).addToDescriptorHeap(device, srv, 0);
    outputScene.m_textures.at(diffuseTextureIndex).addToDescriptorHeap(device, srv, 1);
    outputScene.m_textures.at(specularTextureIndex).addToDescriptorHeap(device, srv, 2);
    outputScene.m_textures.at(emissiveTextureIndex).addToDescriptorHeap(device, srv, 3);
    outputScene.m_textures.at(normalMapTextureIndex).addToDescriptorHeap(device, srv, 4);

    // Debug Ausgabe
    std::cout << "Material " << index << "\n";
    std::cout << "AmbientColor: " << ambientColor.r << " " << ambientColor.g << " " << ambientColor.b << " "
              << ambientColor.a << "\n";
    std::cout << "DiffuseColor: " << diffuseColor.r << " " << diffuseColor.g << " " << diffuseColor.b << " "
              << diffuseColor.a << "\n";
    std::cout << "EmissiveColor: " << emissiveColor.r << " " << emissiveColor.g << " " << emissiveColor.b << " "
              << emissiveColor.a << "\n";
    std::cout << "SpecularColorAndExponent: " << specularColorAndExponent.r << " " << specularColorAndExponent.g << " "
              << specularColorAndExponent.b << " " << specularColorAndExponent.a << "\n";
    std::cout << "\n";

    // Prepare the material constant buffer
    MaterialConstantBuffer materialBuffer   = {};
    materialBuffer.ambientColor             = ambientColor;
    materialBuffer.diffuseColor             = diffuseColor;
    materialBuffer.emissionColor            = emissiveColor;
    materialBuffer.specularColorAndExponent = specularColorAndExponent;

    // Create a GPU constant buffer for this material
    Material material;
    material.materialConstantBuffer = ConstantBufferD3D12(materialBuffer, device);
    material.srvDescriptorHeap = srv;

    // Add the material to the scene's material list
    outputScene.m_materials.push_back(material);
  }
}
