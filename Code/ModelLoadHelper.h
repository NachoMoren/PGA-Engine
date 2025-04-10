#ifndef MODEL_LOAD_HELPER
#define MODEL_LOAD_HELPER
#include <assimp/cimport.h>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include "platform.h"

#include <vector>

struct App; 
struct Mesh;
struct Material;

namespace ModelHelper {
	void ProcessAssimpMesh(const aiScene* scene, aiMesh* mesh, Mesh* myMesh, u32 baseMeshMaterialIndex, std::vector<u32>& submeshMaterialIndices);


	void ProcessAssimpNode(const aiScene* scene, aiNode* node, Mesh* myMesh, u32 baseMeshMaterialIndex, std::vector<u32>& submeshMaterialIndices);

	void ProcessAssimpMaterial(App* app, aiMaterial* material, Material& myMaterial, String directory);

	u32 LoadModel(App* app, const char* filename);

	//u32 LoadTexture2D(App* app, const char* filepath);
}

#endif // !MODEL_LOAD_HELPER

