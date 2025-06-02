//
// engine.h: This file contains the types and functions relative to the engine.
//

#pragma once

#include "platform.h"
#include <glad/glad.h>
#include <unordered_map>
#include "BufferManagement.h"
#include "ModelLoadHelper.h"

typedef glm::vec2  vec2;
typedef glm::vec3  vec3;
typedef glm::vec4  vec4;
typedef glm::ivec2 ivec2;
typedef glm::ivec3 ivec3;
typedef glm::ivec4 ivec4;

struct Image
{
    void* pixels;
    ivec2 size;
    i32   nchannels;
    i32   stride;
};

struct Camera
{
    glm::mat4 projection;
    glm::mat4 view;

    glm::vec3 position;
	glm::vec3 front;
	glm::vec3 up;
	glm::vec3 right;

    float yaw; 
	float pitch;
    float speed; 
	float sensitivity;
    float aspectRatio;
    float zNear;
    float zFar;
	float fov;
};

struct Texture
{
    GLuint      handle;
    std::string filepath;
};

struct Material
{
    std::string     name;
    vec3            albedo;
    vec3            emissive;
    f32             smoothness;
    u32             albedoTextureIdx;
    u32             emissiveTextureIdx;
    u32             specularTextureIdx;
    u32             normalsTextureIdx;
    u32             bumpTextureIdx;
};

struct VertexBufferAttribute
{
    u8 location;
    u8 componentCount;
    u8 offset;
};


struct VertexBufferLayout
{
    std::vector<VertexBufferAttribute> attributes;
    u8 stride;
};

struct VertexShaderAttribute
{
    u8 location;
    u8 componentCount;
};



enum Mode
{
    Mode_TexturedQuad,
    Mode_Forward,
    Mode_Deferred
};

struct VertexShaderLayout
{
    std::vector<VertexShaderAttribute> attributes;
};

struct Vao
{
    GLuint handle;
    GLuint programHandle;
};

struct Submesh
{
    VertexBufferLayout vertexBufferLayout;
    std::vector<float> vertices;
    std::vector<u32> indices;
    u32 vertexOffset;
    u32 indexOffset;

    std::vector<Vao> vaos;
};

struct Mesh
{
    std::vector<Submesh>    submeshes;
    GLuint                  vertexBufferHandle;
    GLuint                  indexBufferHandle;
};

struct Model
{
    u32 meshIdx;
    std::vector<u32> materialIdx;
};

struct Program
{
    GLuint             handle;
    std::string        filepath;
    std::string        programName;
    u64                lastWriteTimestamp; // What is this for?
    VertexBufferLayout vertexInputLayout;
};

struct VertexV3V2
{
	glm::vec3 pos;
	glm::vec2 uv;
};

const VertexV3V2 vertices[] = {
    {glm::vec3(-1.0, -1.0, 0.0), glm::vec2(0.0, 0.0)}, // bottom-left vertex
    {glm::vec3(1.0, -1.0, 0.0), glm::vec2(1.0, 0.0)}, // bottom-right vertex
    {glm::vec3(1.0, 1.0, 0.0), glm::vec2(1.0, 1.0)}, // top-right vertex
    {glm::vec3(-1.0, 1.0, 0.0), glm::vec2(0.0, 1.0)}, // top-left vertex
};

const u16 indices[] = {
	0, 1, 2,
	0, 2, 3
};

struct Entity {
    glm::mat4 worldMatrix; 
    u32 modelIndex; 
    u32 localParamsOffset; 
	u32 localParamsSize;
    std::string name;

    // Transform 
	vec3 position;
	vec3 rotation;
	vec3 scale;
};
enum LightType
{
	LightType_Directional,
	LightType_Point
};

struct Light
{
	LightType type;
    vec3 color;
    vec3 direction;
	vec3 position;
	float intensity;
    std::string name; 

    // Constructor
    Light(LightType type, vec3 color, vec3 direction, vec3 position, float intensity, std::string name) 
    {
		this->type = type;
		this->color = color;
		this->direction = direction;
		this->position = position;
		this->intensity = intensity;
		this->name = name;
    }
};


struct App
{
    // Loop
    f32  deltaTime;
    bool isRunning;

    // Input
    Input input;

    // Graphics
    char gpuName[64];
    char openGlVersion[64];

    ivec2 displaySize;
    
	// Resources
    std::vector<Texture>    textures;
    std::vector<Material>   materials;
    std::vector<Mesh>       meshes;
    std::vector<Model>      models;
    std::vector<Program>    programs;
    std::vector<Entity>     entities;
    std::vector<Light>      lights;

    // program indices
    u32 lightProgramIdx;
    u32 texturedMeshProgramIdx;
	u32 debugLightProgramIdx;
	u32 forwardProgramIdx;

    u32 blitBrightestPixelProgramIdx;

    u32 patrickModel; 
    
    // texture indices
    u32 diceTexIdx;
    u32 whiteTexIdx;
    u32 blackTexIdx;
    u32 normalTexIdx;
    u32 magentaTexIdx;
 

    // Mode
    Mode mode;

    // Embedded geometry (in-editor simple meshes such as
    // a screen filling quad, a cube, a sphere...)
    GLuint embeddedVertices;
    GLuint embeddedElements;

    // Location of the texture uniform in the textured quad shader
    GLuint programUniformTexture;
	GLuint forwardProgram_uTexture;
	GLuint texturedMeshProgram_uTexture;
	GLuint texturedMeshProgram_uNear;
	GLuint texturedMeshProgram_uFar;
    GLuint lightProgram_uAlbedo; 
	GLuint lightProgram_uNormal;
	GLuint lightProgram_uPosition;
    GLuint uProjectionMatrix; 
	GLuint uLightColor;


	// Color attachment of the framebuffer
	GLuint colorAttachmentTexture;
	GLuint depthAttachmentTexture;
	GLuint normalAttachmentTexture;
	GLuint positionAttachmentTexture;
	GLuint mainAttachmentTexture;

	GLuint blitAttachmentTexture;

    // Render selector
	std::unordered_map<std::string, GLuint> renderSelector;
    std::string currentAttachment = "Main";

    // VAO object to link our screen filling quad with our textured quad shader
    GLuint vao;

    std::string openGLInfo;

    // Entity 
	GLint maxUniformBufferSize;
	GLint uniformBlockAlignment;
	Buffer localUniformBuffer;

    GLuint globalParamsOffset;
	GLuint globalParamsSize;
    
	// Framebuffers for deferred
    GLuint gBuffer;
	GLuint lightBuffer;

    // 

    // Framebuffer for forward
    GLuint forwardBuffer; 

    //Camera
    Camera camera; 	

    // Counters
	int directionalLightCount = 0;
	int pointLightCount = 0;
    int primitiveCount = 0;
	
    // primitives
    std::vector<std::string> primitives = {"Cube", "Sphere","Cone", "Cylinder","Plane", "Torus" };
	std::vector<u32> primitiveIdxs;
};

void Init(App* app);

void InitFramebuffers(App* app);

void InitBuffers(App* app);

void InitOpenGLInfo(App* app);

void Gui(App* app);

void Update(App* app);

void Render(App* app);

// Not sure what kind of variable viewSize is
void PassBlitBrightPixels(GLuint *framebuffer, const int width, const int height, GLenum colorattachment, GLuint inputTexture, GLint inputLOD, float threshold);

u32 LoadTexture2D(App* app, const char* filepath);

void InitCamera(App* app);

void AlignUniformBuffers(App* app);

void CameraMovement(App* app);

void CameraLookAt(App* app);

void CheckFramebufferStatus();

GLuint CreateTextureAttachment(GLenum internalFormat, GLenum format, GLenum type, int width, int height);

void GuiInspectorLights(App* app);

void GuiAddLights(App* app);

void GuiInspectorEntities(App* app);

void GuiAddPrimitive(App* app);

void GuiInspectorCamera(App* app);
