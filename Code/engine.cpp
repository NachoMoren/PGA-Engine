//
// engine.cpp : Put all your graphics stuff in this file. This is kind of the graphics module.
// In here, you should type all your OpenGL commands, and you can also type code to handle
// input platform events (e.g to move the camera or react to certain shortcuts), writing some
// graphics related GUI options, and so on.
//

#include "engine.h"
#include <imgui.h>
#include <stb_image.h>
#include <stb_image_write.h>

#define MIPMAP_BASE_LEVEL 0
#define MIPMAP_MAX_LEVEL  4

GLuint CreateProgramFromSource(String programSource, const char* shaderName)
{
    GLchar  infoLogBuffer[1024] = {};
    GLsizei infoLogBufferSize = sizeof(infoLogBuffer);
    GLsizei infoLogSize;
    GLint   success;

    char versionString[] = "#version 430\n";
    char shaderNameDefine[128];
    sprintf_s(shaderNameDefine, "#define %s\n", shaderName);
    char vertexShaderDefine[] = "#define VERTEX\n";
    char fragmentShaderDefine[] = "#define FRAGMENT\n";

    const GLchar* vertexShaderSource[] = {
        versionString,
        shaderNameDefine,
        vertexShaderDefine,
        programSource.str
    };
    const GLint vertexShaderLengths[] = {
        (GLint) strlen(versionString),
        (GLint) strlen(shaderNameDefine),
        (GLint) strlen(vertexShaderDefine),
        (GLint) programSource.len
    };
    const GLchar* fragmentShaderSource[] = {
        versionString,
        shaderNameDefine,
        fragmentShaderDefine,
        programSource.str
    };
    const GLint fragmentShaderLengths[] = {
        (GLint) strlen(versionString),
        (GLint) strlen(shaderNameDefine),
        (GLint) strlen(fragmentShaderDefine),
        (GLint) programSource.len
    };

    GLuint vshader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vshader, ARRAY_COUNT(vertexShaderSource), vertexShaderSource, vertexShaderLengths);
    glCompileShader(vshader);
    glGetShaderiv(vshader, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        glGetShaderInfoLog(vshader, infoLogBufferSize, &infoLogSize, infoLogBuffer);
        ELOG("glCompileShader() failed with vertex shader %s\nReported message:\n%s\n", shaderName, infoLogBuffer);
    }

    GLuint fshader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fshader, ARRAY_COUNT(fragmentShaderSource), fragmentShaderSource, fragmentShaderLengths);
    glCompileShader(fshader);
    glGetShaderiv(fshader, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        glGetShaderInfoLog(fshader, infoLogBufferSize, &infoLogSize, infoLogBuffer);
        ELOG("glCompileShader() failed with fragment shader %s\nReported message:\n%s\n", shaderName, infoLogBuffer);
    }

    GLuint programHandle = glCreateProgram();
    glAttachShader(programHandle, vshader);
    glAttachShader(programHandle, fshader);
    glLinkProgram(programHandle);
    glGetProgramiv(programHandle, GL_LINK_STATUS, &success);
    if (!success)
    {
        glGetProgramInfoLog(programHandle, infoLogBufferSize, &infoLogSize, infoLogBuffer);
        ELOG("glLinkProgram() failed with program %s\nReported message:\n%s\n", shaderName, infoLogBuffer);
    }
    
    glUseProgram(0);

    glDetachShader(programHandle, vshader);
    glDetachShader(programHandle, fshader);
    glDeleteShader(vshader);
    glDeleteShader(fshader);

    return programHandle;
}

u32 LoadProgram(App* app, const char* filepath, const char* programName)
{
    String programSource = ReadTextFile(filepath);

    Program program = {};
    program.handle = CreateProgramFromSource(programSource, programName);
    program.filepath = filepath;
    program.programName = programName;
    program.lastWriteTimestamp = GetFileLastWriteTimestamp(filepath);

    GLint attributeCount = 0;
    glGetProgramiv(program.handle, GL_ACTIVE_ATTRIBUTES, &attributeCount);

    for (GLuint i = 0; i < attributeCount; i++)
    {
        GLsizei length = 0;
        GLint size = 0;
        GLenum type = 0;
        GLchar name[256];
		glGetActiveAttrib(program.handle, i, ARRAY_COUNT(name), &length, &size, &type, name);

        u8 location = glGetAttribLocation(program.handle, name);
        program.vertexInputLayout.attributes.push_back({ location, (u8)size });
    }

    app->programs.push_back(program);

    return app->programs.size() - 1;
}

Image LoadImage(const char* filename)
{
    Image img = {};
    stbi_set_flip_vertically_on_load(true);
    img.pixels = stbi_load(filename, &img.size.x, &img.size.y, &img.nchannels, 0);
    if (img.pixels)
    {
        img.stride = img.size.x * img.nchannels;
    }
    else
    {
        ELOG("Could not open file %s", filename);
    }
    return img;
}

void FreeImage(Image image)
{
    stbi_image_free(image.pixels);
}

GLuint CreateTexture2DFromImage(Image image)
{
    GLenum internalFormat = GL_RGB8;
    GLenum dataFormat     = GL_RGB;
    GLenum dataType       = GL_UNSIGNED_BYTE;

    switch (image.nchannels)
    {
        case 3: dataFormat = GL_RGB; internalFormat = GL_RGB8; break;
        case 4: dataFormat = GL_RGBA; internalFormat = GL_RGBA8; break;
        default: ELOG("LoadTexture2D() - Unsupported number of channels");
    }

    GLuint texHandle;
    glGenTextures(1, &texHandle);
    glBindTexture(GL_TEXTURE_2D, texHandle);
    glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, image.size.x, image.size.y, 0, dataFormat, dataType, image.pixels);
    ELOG("LoadTexture2D() - Data format: %i", image.nchannels);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_R, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glGenerateMipmap(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, 0);

    return texHandle;
}

u32 LoadTexture2D(App* app, const char* filepath)
{
    for (u32 texIdx = 0; texIdx < app->textures.size(); ++texIdx)
        if (app->textures[texIdx].filepath == filepath)
            return texIdx;

    Image image = LoadImage(filepath);

    if (image.pixels)
    {
        Texture tex = {};
        tex.handle = CreateTexture2DFromImage(image);
        tex.filepath = filepath;

        u32 texIdx = app->textures.size();
        app->textures.push_back(tex);

        FreeImage(image);
        return texIdx;
    }
    else
    {
        return UINT32_MAX;
    }
}

GLuint FindVAO(Mesh& mesh, u32 submeshIndex, const Program& program) {
    
    Submesh& submesh = mesh.submeshes[submeshIndex];

    for (u32 i = 0; i < (u32)submesh.vaos.size(); ++i) {
        if (submesh.vaos[i].programHandle == program.handle) {
            return submesh.vaos[i].handle;
        }
    }
    // Create a new VAO for this submesh
	GLuint vaoHandle = 0;
    glGenVertexArrays(1, &vaoHandle);
    glBindVertexArray(vaoHandle);

    glBindBuffer(GL_ARRAY_BUFFER, mesh.vertexBufferHandle);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mesh.indexBufferHandle);

    for (u32 i = 0; i < program.vertexInputLayout.attributes.size(); i++) {
        bool attributeWasLinked = false;

        for (u32 j = 0; j < submesh.vertexBufferLayout.attributes.size(); j++) {
            if (program.vertexInputLayout.attributes[i].location == submesh.vertexBufferLayout.attributes[j].location) {
                const u32 index = submesh.vertexBufferLayout.attributes[j].location;
                const u32 ncomp = submesh.vertexBufferLayout.attributes[j].componentCount;
                const u32 offset = submesh.vertexBufferLayout.attributes[j].offset + submesh.vertexOffset;
                const u32 stride = submesh.vertexBufferLayout.stride;
                glVertexAttribPointer(index, ncomp, GL_FLOAT, GL_FALSE, stride, (void*)(u64)offset);
                glEnableVertexAttribArray(index);

                attributeWasLinked = true;
                break;
            }
        }
        assert(attributeWasLinked);
    }

    glBindVertexArray(0);

    // Store the new VAO in the submesh
    Vao vao = { vaoHandle, program.handle };
    submesh.vaos.push_back(vao);
    
    
	return vaoHandle;
}



glm::mat4 TransformScale(const vec3& scaleFactors)
{
    return glm::scale(scaleFactors);
}

glm::mat4 TransformPositionScale(const vec3& position, const vec3& scaleFactors)
{
    glm::mat4 ReturnValue = glm::translate(position);
    ReturnValue = glm::scale(ReturnValue, scaleFactors);
    return ReturnValue;
}

glm::mat4 TransformPositionRotationScale(const vec3& position, const vec3& rotation, const vec3& scaleFactors)
{
	glm::mat4 ReturnValue = glm::translate(position);

	ReturnValue = glm::rotate(ReturnValue, glm::radians(rotation.x), glm::vec3(1.0f, 0.0f, 0.0f));
	ReturnValue = glm::rotate(ReturnValue, glm::radians(rotation.y), glm::vec3(0.0f, 1.0f, 0.0f));
	ReturnValue = glm::rotate(ReturnValue, glm::radians(rotation.z), glm::vec3(0.0f, 0.0f, 1.0f));

	ReturnValue = glm::scale(ReturnValue, scaleFactors);

	return ReturnValue;
}



void Init(App* app)
{
    InitOpenGLInfo(app);
	InitBuffers(app);
    InitFramebuffers(app);
	InitCamera(app);

    app->normalTexIdx = LoadTexture2D(app, "color_normal.png");
	app->whiteTexIdx = LoadTexture2D(app, "color_white.png");
	app->blackTexIdx = LoadTexture2D(app, "color_black.png");
	app->magentaTexIdx = LoadTexture2D(app, "color_magenta.png");

	app->normalWaterTex = LoadTexture2D(app, "Water/normalmap.png");
	app->dudvWaterTex = LoadTexture2D(app, "Water/dudvmap.png");   

    glGetIntegerv(GL_MAX_UNIFORM_BLOCK_SIZE, &app->maxUniformBufferSize);
    glGetIntegerv(GL_UNIFORM_BUFFER_OFFSET_ALIGNMENT, &app->uniformBlockAlignment);
    app->localUniformBuffer = CreateConstantBuffer(app->maxUniformBufferSize);

    // Load primitive
    for (int i = 0; i < app->primitives.size(); ++i) {
		std::string path = "Primitives/" + app->primitives[i] + ".obj";
		u32 primitiveIdx = ModelHelper::LoadModel(app, path.c_str());
		app->primitiveIdxs.push_back(primitiveIdx);
    }

    // Load patrick model
    u32 patrickModel = ModelHelper::LoadModel(app, "Patrick/Patrick.obj");
    Entity e; 
	e.position = vec3(0.0f, 3.5f, 0.0f);
	e.rotation = vec3(0.0f, 0.0f, 0.0f);
	e.scale = vec3(1.0f, 1.0f, 1.0f);
	e.modelIndex = patrickModel;
	e.worldMatrix = TransformPositionRotationScale(e.position, e.rotation, e.scale);
	e.name = "Patrick";
	app->entities.push_back(e);

    // Psyduck model
	u32 psyduckModel = ModelHelper::LoadModel(app, "Psyduck/Psyduck.obj");
	Entity psyduck;
	psyduck.position = vec3(5.0f, 0.0f, 0.0f);
	psyduck.rotation = vec3(0.0f, 0.0f, 0.0f);
	psyduck.scale = vec3(2.0f, 2.0f, 2.0f);
	psyduck.modelIndex = psyduckModel;
	psyduck.worldMatrix = TransformPositionRotationScale(psyduck.position, psyduck.rotation, psyduck.scale);
	psyduck.name = "Psyduck";
	app->entities.push_back(psyduck);

    u32 pondModel = ModelHelper::LoadModel(app, "LowPolyTrees/LowPolyTrees.obj");
    Entity pond;
    pond.position = vec3(0.0f, 5.0f, 0.0f);
    pond.rotation = vec3(0.0f, 0.0f, 0.0f);
    pond.scale = vec3(15.0f, 15.0f, 15.0f);
    pond.modelIndex = pondModel;
    pond.worldMatrix = TransformPositionRotationScale(pond.position, pond.rotation, pond.scale);
    pond.name = "Pond";
    app->entities.push_back(pond);

    // Pond scene
	/*u32 pondModel = ModelHelper::LoadModel(app, "Pond/Pond.obj");
	Entity pond;
	pond.position = vec3(0.0f, -22.0f, 0.0f);
	pond.rotation = vec3(0.0f, 0.0f, 0.0f);
	pond.scale = vec3(0.2f, 0.2f, 0.2f);
	pond.modelIndex = pondModel;
	pond.worldMatrix = TransformPositionRotationScale(pond.position, pond.rotation, pond.scale);
	pond.name = "Pond";
	app->entities.push_back(pond);*/

 //   // Load plane  
	//Entity plane;
	//plane.position = vec3(0.0f, 0.0f, 0.0f);
	//plane.rotation = vec3(0.0f, 0.0f, 0.0f);
	//plane.scale = vec3(20.0f, 1.0f, 20.0f);
	//plane.modelIndex = app->primitiveIdxs[4];
	//plane.worldMatrix = TransformPositionRotationScale(plane.position, plane.rotation, plane.scale);
	//plane.name = "Plane";
	//app->entities.push_back(plane);

	// Load sphere
	//Entity sphere;
	//sphere.position = vec3(5.0f, 2.0f, -5.0f);
	//sphere.rotation = vec3(0.0f, 0.0f, 0.0f);
	//sphere.scale = vec3(3.0f, 3.0f, 3.0f);
	//sphere.modelIndex = app->primitiveIdxs[1];
	//sphere.worldMatrix = TransformPositionRotationScale(sphere.position, sphere.rotation, sphere.scale);
	//sphere.name = "Sphere";
	//app->entities.push_back(sphere);
	//
 //   // Load cube
	//Entity cube;
	//cube.position = vec3(-5.0f, 1.0f, 5.0f);
	//cube.rotation = vec3(0.0f, 0.0f, 0.0f);
	//cube.scale = vec3(2.0f, 2.0f, 2.0f);
	//cube.modelIndex = app->primitiveIdxs[0];
	//cube.worldMatrix = TransformPositionRotationScale(cube.position, cube.rotation, cube.scale);
	//cube.name = "Cube";
	//app->entities.push_back(cube);

	//// Load cone
	//Entity cone;
	//cone.position = vec3(7.0f, 1.0f, 5.0f);
	//cone.rotation = vec3(0.0f, 0.0f, 0.0f);
	//cone.scale = vec3(3.0f, 3.0f, 3.0f);
	//cone.modelIndex = app->primitiveIdxs[2];
	//cone.worldMatrix = TransformPositionRotationScale(cone.position, cone.rotation, cone.scale);
	//cone.name = "Cone";
	//app->entities.push_back(cone);

	//// Load torus
	//Entity torus;
	//torus.position = vec3(-7.0f, 1.0f, -8.0f);
	//torus.rotation = vec3(0.0f, 0.0f, 0.0f);
	//torus.scale = vec3(3.0f, 3.0f, 3.0f);
	//torus.modelIndex = app->primitiveIdxs[5];
	//torus.worldMatrix = TransformPositionRotationScale(torus.position, torus.rotation, torus.scale);
	//torus.name = "Torus";
	//app->entities.push_back(torus);

    // Lights
    // Directional
	app->lights.push_back(Light(LightType_Directional, vec3(1.0f, 1.0f, 1.0f), vec3(-1.0f, -0.3f, -1.0f), vec3(0.0f, -10.0f, 0.0f), 1.0f, "Directional Light Init"));

	// Point
	app->lights.push_back(Light(LightType_Point, vec3(1.0f, 1.0f, 1.0f), vec3(0.0f, 0.0f, 0.0f), vec3(14.0f, 2.0f, 13.0f), 1.0f, "Point Light Init 1"));
	app->lights.push_back(Light(LightType_Point, vec3(1.0f, 1.0f, 1.0f), vec3(0.0f, 0.0f, 0.0f), vec3(-7.0f, 2.0f, -12.0f), 1.0f, "Point Light Init 2"));
	app->lights.push_back(Light(LightType_Point, vec3(1.0f, 1.0f, 1.0f), vec3(0.0f, 0.0f, 0.0f), vec3(-2.0f, 5.0f, 10.0f), 1.0f, "Point Light Init 3"));
	app->lights.push_back(Light(LightType_Point, vec3(1.0f, 1.0f, 1.0f), vec3(0.0f, 0.0f, 0.0f), vec3(9.0f, 2.0f, -13.0f), 1.0f, "Point Light Init4"));
	app->lights.push_back(Light(LightType_Point, vec3(1.0f, 1.0f, 1.0f), vec3(0.0f, 0.0f, 0.0f), vec3(-13.0f, 2.0f, 15.0f), 1.0f, "Point Light Init 5"));

    // Skybox VAOs
    glGenVertexArrays(1, &app->skyboxVAO);
    glGenBuffers(1, &app->skyboxVBO);
    glBindVertexArray(app->skyboxVAO);
    glBindBuffer(GL_ARRAY_BUFFER, app->skyboxVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(skyboxVertices), &skyboxVertices, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);

    // Load shaders
    // Forward program and uniforms
    app->forwardProgramIdx = LoadProgram(app, "RENDER_GEOMETRY.glsl", "RENDER_GEOMETRY");
	app->forwardProgram_uTexture = glGetUniformLocation(app->programs[app->forwardProgramIdx].handle, "uTexture");

	// Deferred programs and uniforms
	// Geometry pass + Lighting pass + Debug lights programs
	app->texturedMeshProgramIdx = LoadProgram(app, "shaders.glsl", "GEOMETRY_RENDER");
    app->texturedMeshProgram_uTexture = glGetUniformLocation(app->programs[app->texturedMeshProgramIdx].handle, "uTexture");
	app->texturedMeshProgram_uNear = glGetUniformLocation(app->programs[app->texturedMeshProgramIdx].handle, "uNear");
	app->texturedMeshProgram_uFar = glGetUniformLocation(app->programs[app->texturedMeshProgramIdx].handle, "uFar");

    app->lightProgramIdx = LoadProgram(app, "shaders.glsl", "LIGHTING_RENDER");
	app->lightProgram_uAlbedo = glGetUniformLocation(app->programs[app->lightProgramIdx].handle, "uAlbedo");
	app->lightProgram_uNormal = glGetUniformLocation(app->programs[app->lightProgramIdx].handle, "uNormal");
	app->lightProgram_uPosition = glGetUniformLocation(app->programs[app->lightProgramIdx].handle, "uPosition");

    app->debugLightProgramIdx = LoadProgram(app, "shaders.glsl", "SHOW_LIGHTS");
	app->uProjectionMatrix = glGetUniformLocation(app->programs[app->debugLightProgramIdx].handle, "uProjectionMatrix");
	app->uLightColor = glGetUniformLocation(app->programs[app->debugLightProgramIdx].handle, "uLightColor");

    app->blitBrightestPixelProgramIdx = LoadProgram(app, "BLIT_BRIGHTEST.glsl", "BLIT_BRIGHTEST");
	app->colorTexture = glGetUniformLocation(app->programs[app->blitBrightestPixelProgramIdx].handle, "uTexture");
	app->threshold = glGetUniformLocation(app->programs[app->blitBrightestPixelProgramIdx].handle, "threshold");

	app->blurProgramIdx = LoadProgram(app, "shaders.glsl", "BLUR");
	app->colorMap = glGetUniformLocation(app->programs[app->blurProgramIdx].handle, "uColorMap");
    app->dir = glGetUniformLocation(app->programs[app->blurProgramIdx].handle, "uDir");
	app->kernelRadius = glGetUniformLocation(app->programs[app->blurProgramIdx].handle, "kernelRadius");
	app->inputLod = glGetUniformLocation(app->programs[app->blurProgramIdx].handle, "uInputLod");
	app->inputLodIntensity = glGetUniformLocation(app->programs[app->blurProgramIdx].handle, "uLodIntensity");

    app->bloomProgramIdx = LoadProgram(app, "shaders.glsl", "BLOOM");
	app->mainTexture = glGetUniformLocation(app->programs[app->bloomProgramIdx].handle, "uMainTexture");
    app->colorMapBlend = glGetUniformLocation(app->programs[app->bloomProgramIdx].handle, "uColorMap");
    app->maxLod = glGetUniformLocation(app->programs[app->bloomProgramIdx].handle, "uMaxLod");

    app->cubemapProgramIdx = LoadProgram(app, "CUBEMAP.glsl", "CUBEMAP");
    app->uSkybox = glGetUniformLocation(app->programs[app->cubemapProgramIdx].handle, "skybox");
    app->uSkyboxProjection = glGetUniformLocation(app->programs[app->cubemapProgramIdx].handle, "projection");
    app->uSkyboxView = glGetUniformLocation(app->programs[app->cubemapProgramIdx].handle, "view");

	app->waterProgramIdx = LoadProgram(app, "shaders.glsl", "WATER_EFFECT");
	app->waterProgram_uView = glGetUniformLocation(app->programs[app->waterProgramIdx].handle, "uView");
	app->waterProgram_uProjection = glGetUniformLocation(app->programs[app->waterProgramIdx].handle, "uProjection");
	app->waterProgram_viewportSize = glGetUniformLocation(app->programs[app->waterProgramIdx].handle, "uViewportSize");
	app->waterProgram_uViewInverse = glGetUniformLocation(app->programs[app->waterProgramIdx].handle, "uViewInverse");
	app->waterProgram_uProjectionInverse = glGetUniformLocation(app->programs[app->waterProgramIdx].handle, "uProjectionInverse");
	app->waterProgram_uReflectionMap = glGetUniformLocation(app->programs[app->waterProgramIdx].handle, "uReflectionMap");
	app->waterProgram_uReflectionDepth = glGetUniformLocation(app->programs[app->waterProgramIdx].handle, "uReflectionDepth");
	app->waterProgram_uRefractionMap = glGetUniformLocation(app->programs[app->waterProgramIdx].handle, "uRefractionMap");
	app->waterProgram_uRefractionDepth = glGetUniformLocation(app->programs[app->waterProgramIdx].handle, "uRefractionDepth");
	app->waterProgram_normalMap = glGetUniformLocation(app->programs[app->waterProgramIdx].handle, "uNormalMap");
	app->waterProgram_dudvMap = glGetUniformLocation(app->programs[app->waterProgramIdx].handle, "uDudvMap");
	app->waterProgram_moveFactor = glGetUniformLocation(app->programs[app->waterProgramIdx].handle, "moveFactor");
    app->mode = Mode_Deferred;
}

void InitBuffers(App* app) 
{
    // Geometry
    glGenBuffers(1, &app->embeddedVertices);
    glBindBuffer(GL_ARRAY_BUFFER, app->embeddedVertices);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    glGenBuffers(1, &app->embeddedElements);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, app->embeddedElements);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

    // Attribute state
    glGenVertexArrays(1, &app->vao);
    glBindVertexArray(app->vao);
    glBindBuffer(GL_ARRAY_BUFFER, app->embeddedVertices);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(VertexV3V2), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(VertexV3V2), (void*)(3 * sizeof(float)));
	glEnableVertexAttribArray(1);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, app->embeddedElements);
	glBindVertexArray(0);
}

void InitFramebuffers(App* app)
{
    // Albedo Texture
    app->colorAttachmentTexture = CreateTextureAttachment(GL_RGBA16F, GL_RGBA, GL_UNSIGNED_BYTE, app->displaySize.x, app->displaySize.y);

    // Position Texture
	app->positionAttachmentTexture = CreateTextureAttachment(GL_RGBA16F, GL_RGB, GL_FLOAT, app->displaySize.x, app->displaySize.y);

	// Normal Texture
	app->normalAttachmentTexture = CreateTextureAttachment(GL_RGBA16F, GL_RGB, GL_FLOAT, app->displaySize.x, app->displaySize.y);

	// Depth Texture
	app->depthAttachmentTexture = CreateTextureAttachment(GL_RGBA16F, GL_RGBA, GL_UNSIGNED_BYTE, app->displaySize.x, app->displaySize.y);

    // Depth Component
    GLuint depthAttachmentHandle;
	depthAttachmentHandle = CreateTextureAttachment(GL_DEPTH_COMPONENT24, GL_DEPTH_COMPONENT, GL_FLOAT, app->displaySize.x, app->displaySize.y);

    //gBuffer
    glGenFramebuffers(1, &app->gBuffer);
    glBindFramebuffer(GL_FRAMEBUFFER, app->gBuffer);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, app->colorAttachmentTexture, 0);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, app->positionAttachmentTexture, 0);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT2, GL_TEXTURE_2D, app->normalAttachmentTexture, 0);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT3, GL_TEXTURE_2D, app->depthAttachmentTexture, 0);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, depthAttachmentHandle, 0);

    CheckFramebufferStatus();

    GLuint drawBuffers[] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1, GL_COLOR_ATTACHMENT2, GL_COLOR_ATTACHMENT3 };
    glDrawBuffers(ARRAY_COUNT(drawBuffers), drawBuffers);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    glGenFramebuffers(1, &app->skyboxVBO);
    glBindFramebuffer(GL_FRAMEBUFFER, app->skyboxVBO);

    // Cubemap texture
    std::string skyboxStr = "Miramar";
    std::vector<std::string> faces
    {
        "Skybox/" + skyboxStr + "/right.png",
        "Skybox/" + skyboxStr + "/left.png",
        "Skybox/" + skyboxStr + "/top.png",
        "Skybox/" + skyboxStr + "/bottom.png",
        "Skybox/" + skyboxStr + "/front.png",
        "Skybox/" + skyboxStr + "/back.png"
    };
    app->rtCubemap = LoadCubemap(faces);

    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    // Final texture
	app->mainAttachmentTexture = CreateTextureAttachment(GL_RGBA16F, GL_RGBA, GL_FLOAT, app->displaySize.x, app->displaySize.y);

	app->bloomAttachmentTexture = CreateTextureAttachment(GL_RGBA16F, GL_RGBA, GL_FLOAT, app->displaySize.x, app->displaySize.y);

    // Depth component 
    GLuint depthLightAttachmentHandle;
	depthLightAttachmentHandle = CreateTextureAttachment(GL_DEPTH_COMPONENT24, GL_DEPTH_COMPONENT, GL_FLOAT, app->displaySize.x, app->displaySize.y);

    // Light buffer
    glGenFramebuffers(1, &app->lightBuffer);
    glBindFramebuffer(GL_FRAMEBUFFER, app->lightBuffer);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, app->mainAttachmentTexture, 0);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, depthLightAttachmentHandle, 0);

	CheckFramebufferStatus();

	GLuint drawBuffersLight[] = { GL_COLOR_ATTACHMENT0};
	glDrawBuffers(ARRAY_COUNT(drawBuffersLight), drawBuffersLight);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	// Forward buffer
	glGenFramebuffers(1, &app->forwardBuffer);
	glBindFramebuffer(GL_FRAMEBUFFER, app->forwardBuffer);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, app->mainAttachmentTexture, 0);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, depthLightAttachmentHandle, 0);

	CheckFramebufferStatus();

	GLuint drawBuffersForward[] = { GL_COLOR_ATTACHMENT0 };
	glDrawBuffers(ARRAY_COUNT(drawBuffersForward), drawBuffersForward);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	// Bloom Framebuffer
	glGenFramebuffers(1, &app->bloomBuffer);
	glBindFramebuffer(GL_FRAMEBUFFER, app->bloomBuffer);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, app->bloomAttachmentTexture, 0);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, depthLightAttachmentHandle, 0);

    InitBloomMipmap(app);
    GenBloomFramebuffers(app);

	// Water Effect FBO and textures

    glGenTextures(1, &app->rtReflection);
    glBindTexture(GL_TEXTURE_2D, app->rtReflection);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, app->displaySize.x, app->displaySize.y, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);

    glGenTextures(1, &app->rtRefraction);
    glBindTexture(GL_TEXTURE_2D, app->rtRefraction);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, app->displaySize.x, app->displaySize.y, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);

	glGenTextures(1, &app->rtRefractionDepth);
	glBindTexture(GL_TEXTURE_2D, app->rtRefractionDepth);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT24, app->displaySize.x, app->displaySize.y, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);

	glGenTextures(1, &app->rtReflectionDepth);
	glBindTexture(GL_TEXTURE_2D, app->rtReflectionDepth);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT24, app->displaySize.x, app->displaySize.y, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);

    // Water effect FBO
    glGenFramebuffers(1, &app->reflectionBuffer);
    glBindFramebuffer(GL_FRAMEBUFFER, app->reflectionBuffer);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, app->rtReflection, 0);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, app->rtReflectionDepth, 0);

    CheckFramebufferStatus();

    GLuint drawReflectionBuffer[] = { GL_COLOR_ATTACHMENT0 };
    glDrawBuffers(ARRAY_COUNT(drawReflectionBuffer), drawReflectionBuffer);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    glGenFramebuffers(1, &app->refractionBuffer);
    glBindFramebuffer(GL_FRAMEBUFFER, app->refractionBuffer);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, app->rtRefraction, 0);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, app->rtRefractionDepth, 0);

    CheckFramebufferStatus();

    GLuint drawRefractionBuffer[] = { GL_COLOR_ATTACHMENT0 };
    glDrawBuffers(ARRAY_COUNT(drawRefractionBuffer), drawRefractionBuffer);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

	// Set default attachment
	app->currentAttachment = "Main";
	app->renderSelector["Color"] = app->colorAttachmentTexture;
	app->renderSelector["WithoutBloom"] = app->mainAttachmentTexture;
	app->renderSelector["Position"] = app->positionAttachmentTexture;
	app->renderSelector["Normal"] = app->normalAttachmentTexture;
	app->renderSelector["Depth"] = app->depthAttachmentTexture;
	app->renderSelector["Main"] = app->bloomAttachmentTexture;
	app->renderSelector["Reflection"] = app->rtReflection;
	app->renderSelector["Refraction"] = app->rtRefraction;
}

void InitBloomMipmap(App* app) 
{
    // Bloom
    if (app->rtBright != 0)
        glDeleteTextures(1, &app->rtBright);
	glGenTextures(1, &app->rtBright);
	glBindTexture(GL_TEXTURE_2D, app->rtBright);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_BASE_LEVEL, MIPMAP_BASE_LEVEL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, MIPMAP_MAX_LEVEL);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, app->displaySize.x/2, app->displaySize.y/2, 0, GL_RGBA, GL_FLOAT, NULL);
	glTexImage2D(GL_TEXTURE_2D, 1, GL_RGBA16F, app->displaySize.x / 4, app->displaySize.y / 4, 0, GL_RGBA, GL_FLOAT, NULL);
	glTexImage2D(GL_TEXTURE_2D, 2, GL_RGBA16F, app->displaySize.x / 8, app->displaySize.y / 8, 0, GL_RGBA, GL_FLOAT, NULL);
	glTexImage2D(GL_TEXTURE_2D, 3, GL_RGBA16F, app->displaySize.x / 16, app->displaySize.y / 16, 0, GL_RGBA, GL_FLOAT, NULL);
	glTexImage2D(GL_TEXTURE_2D, 4, GL_RGBA16F, app->displaySize.x / 32, app->displaySize.y / 32, 0, GL_RGBA, GL_FLOAT, NULL);
	//glGenerateMipmap(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, 0);

	// Bloom Mipmap
	if (app->rtBloomH != 0)
		glDeleteTextures(1, &app->rtBloomH);
	glGenTextures(1, &app->rtBloomH);
	glBindTexture(GL_TEXTURE_2D, app->rtBloomH);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_BASE_LEVEL, MIPMAP_BASE_LEVEL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, MIPMAP_MAX_LEVEL);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, app->displaySize.x / 2, app->displaySize.y / 2, 0, GL_RGBA, GL_FLOAT, NULL);
    glTexImage2D(GL_TEXTURE_2D, 1, GL_RGBA16F, app->displaySize.x / 4, app->displaySize.y / 4, 0, GL_RGBA, GL_FLOAT, NULL);
    glTexImage2D(GL_TEXTURE_2D, 2, GL_RGBA16F, app->displaySize.x / 8, app->displaySize.y / 8, 0, GL_RGBA, GL_FLOAT, NULL);
    glTexImage2D(GL_TEXTURE_2D, 3, GL_RGBA16F, app->displaySize.x / 16, app->displaySize.y / 16, 0, GL_RGBA, GL_FLOAT, NULL);
    glTexImage2D(GL_TEXTURE_2D, 4, GL_RGBA16F, app->displaySize.x / 32, app->displaySize.y / 32, 0, GL_RGBA, GL_FLOAT, NULL);
	//glGenerateMipmap(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, 0);
}

unsigned int LoadCubemap(std::vector<std::string> faces)
{
    unsigned int textureID;
    glGenTextures(1, &textureID);
    glBindTexture(GL_TEXTURE_CUBE_MAP, textureID);

    int width, height, nrChannels;
    for (unsigned int i = 0; i < faces.size(); i++)
    {
        unsigned char* data = stbi_load(faces[i].c_str(), &width, &height, &nrChannels, 0);
        if (data)
        {
            glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
            stbi_image_free(data);
        }
        else
        {
            ELOG("Cubemap texture failed to load at path: %s", faces[i]);
            stbi_image_free(data);
        }
    }
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

    return textureID;
}

void GenBloomFramebuffers(App* app) 
{
    GLuint drawBuffersBloom[] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1 };
    glGenFramebuffers(1, &app->fboBloom1);
    glBindFramebuffer(GL_FRAMEBUFFER, app->fboBloom1);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, app->rtBright, 0);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, app->rtBloomH, 0);
    CheckFramebufferStatus();
   
    glDrawBuffers(ARRAY_COUNT(drawBuffersBloom), drawBuffersBloom);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    glGenFramebuffers(1, &app->fboBloom2);
    glBindFramebuffer(GL_FRAMEBUFFER, app->fboBloom2);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, app->rtBright, 1);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, app->rtBloomH, 1);
    CheckFramebufferStatus();
    glDrawBuffers(ARRAY_COUNT(drawBuffersBloom), drawBuffersBloom);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    glGenFramebuffers(1, &app->fboBloom3);
    glBindFramebuffer(GL_FRAMEBUFFER, app->fboBloom3);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, app->rtBright, 2);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, app->rtBloomH, 2);
    CheckFramebufferStatus();
    glDrawBuffers(ARRAY_COUNT(drawBuffersBloom), drawBuffersBloom);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    glGenFramebuffers(1, &app->fboBloom4);
    glBindFramebuffer(GL_FRAMEBUFFER, app->fboBloom4);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, app->rtBright, 3);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, app->rtBloomH, 3);
    CheckFramebufferStatus();
    glDrawBuffers(ARRAY_COUNT(drawBuffersBloom), drawBuffersBloom);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    glGenFramebuffers(1, &app->fboBloom5);
    glBindFramebuffer(GL_FRAMEBUFFER, app->fboBloom5);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, app->rtBright, 4);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, app->rtBloomH, 4);
    CheckFramebufferStatus();
    glDrawBuffers(ARRAY_COUNT(drawBuffersBloom), drawBuffersBloom);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void CheckFramebufferStatus() {
    GLenum framebufferStatus = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    if (framebufferStatus != GL_FRAMEBUFFER_COMPLETE)
    {
        switch (framebufferStatus)
        {
        case GL_FRAMEBUFFER_UNDEFINED: ELOG("GL_FRAMEBUFFER_UNDEFINED"); break;
        case GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT: ELOG("GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT"); break;
        case GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT: ELOG("GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT"); break;
        case GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER: ELOG("GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER"); break;
        case GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER: ELOG("GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER"); break;
        case GL_FRAMEBUFFER_UNSUPPORTED: ELOG("GL_FRAMEBUFFER_UNSUPPORTED"); break;
        case GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE: ELOG("GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE"); break;
        case GL_FRAMEBUFFER_INCOMPLETE_LAYER_TARGETS: ELOG("GL_FRAMEBUFFER_INCOMPLETE_LAYER_TARGETS"); break;
        default: ELOG("Unknown framebuffer status error");
        }
    }
}

void PassBlur(App* app, u32 fbo, int w, int h, GLenum colorAttachment, GLuint texture, GLint inputLod, int dirX, int dirY, float inputLodIntensity) 
{
	glBindFramebuffer(GL_FRAMEBUFFER, fbo);
	glDrawBuffers(1, &colorAttachment);
	glViewport(0, 0, w, h);
	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glDisable(GL_DEPTH_TEST);
	glDisable(GL_BLEND);

    Program& blurProgram = app->programs[app->blurProgramIdx];
    glUseProgram(blurProgram.handle);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, texture);

    glBindVertexArray(app->vao);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, app->embeddedElements);

	glUniform1i(app->colorMap, 0);
    glUniform2f(app->dir, dirX, dirY);
	glUniform1i(app->inputLod, inputLod);
	glUniform1i(app->kernelRadius, app->kernelRad);
	glUniform1f(app->inputLodIntensity, inputLodIntensity);

    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_SHORT, 0);
    glBindVertexArray(0);

    glUseProgram(0);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void PassBlitBrightPixels(App* app, u32 fbo, int w, int h, GLenum colorAttachment, GLuint texture, float threshold)
{
    glBindFramebuffer(GL_FRAMEBUFFER, fbo);
    glDrawBuffers(1, &colorAttachment);
    glViewport(0, 0, w, h);
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	Program& blitBrightestPixelProgram = app->programs[app->blitBrightestPixelProgramIdx];
	glUseProgram(blitBrightestPixelProgram.handle);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, texture);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

	glBindVertexArray(app->vao);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, app->embeddedElements);

    glUniform1i(app->colorTexture, 0);
    glUniform1f(app->threshold, threshold);

    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_SHORT, 0);
    glBindVertexArray(0);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glUseProgram(0);
}

void PassBloom(App* app, u32 fbo, GLenum colorAttachment, GLuint texture, int maxLod)
{
    glBindFramebuffer(GL_FRAMEBUFFER, fbo);
    glDrawBuffers(1, &colorAttachment);
    glViewport(0, 0, app->displaySize.x, app->displaySize.y);
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glDisable(GL_DEPTH);
    glEnable(GL_BLEND);
    glBlendFunc(GL_ONE, GL_ONE);

    Program& bloomProgram = app->programs[app->bloomProgramIdx];
    glUseProgram(bloomProgram.handle);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, app->mainAttachmentTexture);
	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, texture);

    glBindVertexArray(app->vao);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, app->embeddedElements);

	glUniform1i(app->mainTexture, 0); 
    glUniform1i(app->colorMapBlend, 1);
    glUniform1i(app->maxLod, maxLod);

    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_SHORT, 0);
    glBindVertexArray(0);

	glEnable(GL_DEPTH);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glUseProgram(0);
}

GLuint CreateTextureAttachment(GLenum internalFormat, GLenum format, GLenum type, int width, int height) 
{
	GLuint target;
	glGenTextures(1, &target);
	glBindTexture(GL_TEXTURE_2D, target);
    glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, width, height, 0, format, type, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glBindTexture(GL_TEXTURE_2D, 0);
    return target; 
}

void InitOpenGLInfo(App* app) 
{
    //Get OPENGL info.
    app->openGLInfo += "OpenGL version:\n" + std::string(reinterpret_cast<const char*>(glGetString(GL_VERSION)));
    app->openGLInfo += "\n\nOpenGL renderer:\n" + std::string(reinterpret_cast<const char*>(glGetString(GL_RENDERER)));
    app->openGLInfo += "\n\nOpenGL vendor:\n" + std::string(reinterpret_cast<const char*>(glGetString(GL_VENDOR)));
    app->openGLInfo += "\n\nOpenGL GLSL version:\n" + std::string(reinterpret_cast<const char*>(glGetString(GL_SHADING_LANGUAGE_VERSION)));
    app->openGLInfo += "\n\nOpenGL extensions:\n";
    GLint num_extensions;
    glGetIntegerv(GL_NUM_EXTENSIONS, &num_extensions);
    for (int i = 0; i < num_extensions; ++i) {
        const unsigned char* str = glGetStringi(GL_EXTENSIONS, GLuint(i));
        app->openGLInfo += '\n' + std::string(reinterpret_cast<const char*>(str));
    }
}

void Gui(App* app)
{
    // Create Docking
	static bool dockOpened = true;

	ImGuiViewport* viewport = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(viewport->GetWorkPos());
    ImGui::SetNextWindowSize(viewport->GetWorkSize());

    ImGuiWindowFlags host_window_flags = 0;
    host_window_flags |= ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoDocking;
    host_window_flags |= ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus | ImGuiWindowFlags_NoBackground;

    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 10.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 1.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.5f, 0.5f));
    ImGui::Begin("DockSpace", &dockOpened, host_window_flags);
    ImGui::PopStyleVar(3);
    ImGuiID dockspaceId = ImGui::GetID("MyDockSpace");
    ImGui::DockSpace(dockspaceId, ImVec2(0.0f, 0.0f), ImGuiDockNodeFlags_PassthruCentralNode);
    ImGui::End();


    // Info window
    ImGui::Begin("Info");
    ImGui::Text("FPS: %f", 1.0f/app->deltaTime);
    ImGui::Text("%s", app->openGLInfo.c_str());
    ImGui::End();

    // Inspector

    ImGui::Begin("Inspector");
    GuiAddPrimitive(app);
    GuiAddLights(app);

	ImGui::Separator();
    // TODO: Add lights and primitives from inspector
    ImGui::Text("Select mode");
    const char* modes[] = { "Textured Quad", "Forward", "Deferred" };
    int currentMode = (int)app->mode;
    if (ImGui::BeginCombo("##Mode Selector", modes[currentMode])) {
        for (int i = 0; i < ARRAY_COUNT(modes); i++)
        {
            bool isSelected = (currentMode == i);
            if (ImGui::Selectable(modes[i], isSelected))
            {
                currentMode = i;
                app->mode = (Mode)i;
            }
            if (isSelected)
                ImGui::SetItemDefaultFocus();
        }
        ImGui::EndCombo();
    }
    ImGui::Separator();

    if (app->mode == Mode_Deferred) {
        ImGui::Text("Select buffer");
        if (ImGui::BeginCombo("##Attachment", app->currentAttachment.c_str())) {
            for (auto it : app->renderSelector) {
                if (ImGui::Selectable(it.first.c_str(), app->currentAttachment == it.first)) {
                    app->currentAttachment = it.first;
                }
                if (app->currentAttachment == it.first) {
                    ImGui::SetItemDefaultFocus();
                }
            }
            ImGui::EndCombo();
        }
        ImGui::Separator();
    }

	ImGui::Checkbox("Show Debug Lights", &app->showDebugLights);
    ImGui::Separator();

    ImGui::Text("Bloom Variables");
    ImGui::Text("Bloom Threshold");
    ImGui::SameLine();
	ImGui::SliderFloat("##Bloom Threshold", &app->valThreshold, 0.0f, 1.0f);
	ImGui::Text("Kernel Radius");
	ImGui::SameLine();
	ImGui::InputInt("##Kernel Raduis", &app->kernelRad, 1, 72);
    for (int i = 0; i < 5; ++i) {
		ImGui::Text("LOD %d Intensity", i);
		ImGui::SameLine();
		ImGui::SliderFloat(("##LOD " + std::to_string(i) + " Intensity").c_str(), &app->intensities[i], 0.0f, 4.0f);
    }
	ImGui::Separator();

	

    if (ImGui::CollapsingHeader("Water Plane", ImGuiTreeNodeFlags_DefaultOpen)) {

		ImGui::Text("Position: ");
		ImGui::DragFloat3("##Water Position", &app->waterPos[0], 0.5f, true);

		ImGui::Text("Scale: ");
		ImGui::DragFloat3("##Water Scale", &app->waterScale[0], 0.01f, 0.00001f, 10000.0f);
    }

	GuiInspectorCamera(app);
    GuiInspectorEntities(app);
	GuiInspectorLights(app);
    ImGui::End();

    if (ImGui::Begin("Scene")) {
        if(app->mode == Mode_Deferred)
            ImGui::Image((ImTextureID)app->renderSelector[app->currentAttachment], ImGui::GetContentRegionAvail(), ImVec2(0, 1), ImVec2(1, 0));
		else if (app->mode == Mode_Forward)
			ImGui::Image((ImTextureID)app->mainAttachmentTexture, ImGui::GetContentRegionAvail(), ImVec2(0, 1), ImVec2(1, 0));
    }
	ImGui::End();


}

void Update(App* app)
{
    // You can handle app->input keyboard/mouse here
    CameraMovement(app);
	CameraLookAt(app);

    AlignUniformBuffers(app, app->camera, false);
}

void Render(App* app)
{

    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glViewport(0, 0, app->displaySize.x, app->displaySize.y);

    switch (app->mode)
    {
        // TODO: Draw your textured quad here!
                // - clear the framebuffer
                // - set the viewport
                // - set the blending state
                // - bind the texture into unit 0
                // - bind the program 
                //   (...and make its texture sample from unit 0)
                // - bind the vao
                // - glDrawElements() !!!
        case Mode_TexturedQuad:
        {

            Program& texturedGeometryProgram = app->programs[app->texturedMeshProgramIdx];
            glUseProgram(texturedGeometryProgram.handle);
            glBindVertexArray(app->vao);

            glEnable(GL_BLEND);
            glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

            glUniform1i(app->programUniformTexture, 0);
            glActiveTexture(GL_TEXTURE0);
            GLuint texHandle = app->textures[app->diceTexIdx].handle;
            glBindTexture(GL_TEXTURE_2D, texHandle);

            glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

            glBindVertexArray(0);
            glUseProgram(0);
            break;
        }
			
        case Mode_Forward:
        {
            // Geometry Pass
			glBindFramebuffer(GL_FRAMEBUFFER, app->forwardBuffer);
            glViewport(0, 0, app->displaySize.x, app->displaySize.y);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

			glEnable(GL_DEPTH_TEST);

            Program& forwardProgram = app->programs[app->forwardProgramIdx];
            glUseProgram(forwardProgram.handle);

            glBindBufferRange(GL_UNIFORM_BUFFER, 0, app->localUniformBuffer.handle, app->globalParamsOffset, app->globalParamsSize);

            for (auto it = app->entities.begin(); it != app->entities.end(); ++it) {
                glBindBufferRange(GL_UNIFORM_BUFFER, 1, app->localUniformBuffer.handle, it->localParamsOffset, it->localParamsSize);

                Model& model = app->models[it->modelIndex];
                Mesh& mesh = app->meshes[model.meshIdx];

                for (u32 i = 0; i < mesh.submeshes.size(); ++i) {
                    GLuint vao = FindVAO(mesh, i, forwardProgram);
                    glBindVertexArray(vao);

                    u32 submeshMaterialIdx = model.materialIdx[i];
                    Material& submeshMaterial = app->materials[submeshMaterialIdx];


                    glActiveTexture(GL_TEXTURE0);
                    glBindTexture(GL_TEXTURE_2D, app->textures[submeshMaterial.albedoTextureIdx].handle);
                    glUniform1i(app->forwardProgram_uTexture, 0);

                    Submesh& submesh = mesh.submeshes[i];
                    glDrawElements(GL_TRIANGLES, submesh.indices.size(), GL_UNSIGNED_INT, (void*)(u64)submesh.indexOffset);

					glBindVertexArray(0);
                }
            }

			glUseProgram(0);
			glBindFramebuffer(GL_FRAMEBUFFER, 0);

        }
        break;
        case Mode_Deferred:
        {
            // Water textures
			// Reflection
			glBindFramebuffer(GL_FRAMEBUFFER, app->reflectionBuffer);
			Camera reflectionCamera = app->camera;
			reflectionCamera.position.y = 2 * (app->camera.position.y - app->waterPos.y); 
			reflectionCamera.pitch *= -1.0f; 
			CameraDirection(reflectionCamera);
			reflectionCamera.view = glm::lookAt(reflectionCamera.position, reflectionCamera.position + reflectionCamera.front, reflectionCamera.up);

			AlignUniformBuffers(app, reflectionCamera, true);

            PassWaterScene(app,reflectionCamera, app->reflectionBuffer, WaterScenePart::REFLECTION);
			RenderSkybox(app, reflectionCamera);
			glBindFramebuffer(GL_FRAMEBUFFER, 0);

            // Refraction
			glBindFramebuffer(GL_FRAMEBUFFER, app->refractionBuffer);

			Camera refractionCamera = app->camera;
			AlignUniformBuffers(app, refractionCamera, false);
			PassWaterScene(app,refractionCamera, app->refractionBuffer, WaterScenePart::REFRACTION);
			glBindFramebuffer(GL_FRAMEBUFFER, 0);

			// Geometry Pass
            DrawScene(app, app->texturedMeshProgramIdx, app->gBuffer, app->camera, WaterScenePart::NONE);

            // Render water
			Program& waterProgram = app->programs[app->waterProgramIdx];
			glUseProgram(waterProgram.handle);

            u32 waterMeshIdx = app->primitiveIdxs[4];
			Mesh& waterMesh = app->meshes[app->models[waterMeshIdx].meshIdx];
			GLuint vao = FindVAO(waterMesh, 0, waterProgram);

			glm::mat4 waterMatrix = TransformPositionRotationScale(app->waterPos, glm::vec3(0.0), app->waterScale);
			waterMatrix = app->camera.view * waterMatrix;
			app->moveFactor += app->waterMoveSpeed * app->deltaTime;
            app->moveFactor = std::fmod(app->moveFactor, 1.0f); // Keep moveFactor in range [0, 1]

			glBindVertexArray(vao);
			glUniformMatrix4fv(app->waterProgram_uProjection, 1, GL_FALSE, &app->camera.projection[0][0]);
            glUniformMatrix4fv(app->waterProgram_uView, 1, GL_FALSE, &waterMatrix[0][0]);
			glUniform2f(app->waterProgram_viewportSize, app->displaySize.x, app->displaySize.y);
			glUniformMatrix4fv(app->waterProgram_uViewInverse, 1, GL_FALSE, &glm::inverse(waterMatrix)[0][0]);
			glUniformMatrix4fv(app->waterProgram_uProjectionInverse, 1, GL_FALSE, &glm::inverse(app->camera.projection)[0][0]);
			glUniform1f(app->waterProgram_moveFactor, app->moveFactor);
			// Bind textures
			glUniform1i(app->waterProgram_uReflectionMap, 0);
			glUniform1i(app->waterProgram_uReflectionDepth, 1);
			glUniform1i(app->waterProgram_uRefractionMap, 2);
			glUniform1i(app->waterProgram_uRefractionDepth, 3);
			glUniform1i(app->waterProgram_normalMap, 4);
			glUniform1i(app->waterProgram_dudvMap, 5);

			glActiveTexture(GL_TEXTURE0);
			glBindTexture(GL_TEXTURE_2D, app->rtReflection);
			glActiveTexture(GL_TEXTURE1);
			glBindTexture(GL_TEXTURE_2D, app->rtReflectionDepth);
			glActiveTexture(GL_TEXTURE2);
			glBindTexture(GL_TEXTURE_2D, app->rtRefraction);
			glActiveTexture(GL_TEXTURE3);
			glBindTexture(GL_TEXTURE_2D, app->rtRefractionDepth);
			glActiveTexture(GL_TEXTURE4);
			GLuint normalWaterHandle = app->textures[app->normalWaterTex].handle;
            glBindTexture(GL_TEXTURE_2D, normalWaterHandle);
			glActiveTexture(GL_TEXTURE5);
			GLuint dudvWaterHandle = app->textures[app->dudvWaterTex].handle;
			glBindTexture(GL_TEXTURE_2D, dudvWaterHandle);

			glDrawElements(GL_TRIANGLES, waterMesh.submeshes[0].indices.size(), GL_UNSIGNED_INT, 0);
			glBindVertexArray(0);
			glUseProgram(0);
            glBindFramebuffer(GL_FRAMEBUFFER, 0);

			// Light Pass
			glBindFramebuffer(GL_FRAMEBUFFER, app->lightBuffer);

            glClearColor(0.0, 0.0, 0.0, 1.0);
            glClear(GL_COLOR_BUFFER_BIT);

			Program& lightProgram = app->programs[app->lightProgramIdx];
			glUseProgram(lightProgram.handle);

			glBindVertexArray(app->vao);
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, app->embeddedElements);
			glUniform1i(app->lightProgram_uAlbedo, 6);
			glUniform1i(app->lightProgram_uPosition, 7);
			glUniform1i(app->lightProgram_uNormal, 8);

			glActiveTexture(GL_TEXTURE6);
            glBindTexture(GL_TEXTURE_2D, app->colorAttachmentTexture);
			glActiveTexture(GL_TEXTURE7);
			glBindTexture(GL_TEXTURE_2D, app->positionAttachmentTexture);
			glActiveTexture(GL_TEXTURE8);
			glBindTexture(GL_TEXTURE_2D, app->normalAttachmentTexture);

            //Bind uniforms

			glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_SHORT, 0);
			glBindVertexArray(0);

			glBindFramebuffer(GL_READ_FRAMEBUFFER, app->gBuffer);
			glBindFramebuffer(GL_DRAW_FRAMEBUFFER, app->lightBuffer);

			glBlitFramebuffer(0, 0, app->displaySize.x, app->displaySize.y, 0, 0, app->displaySize.x, app->displaySize.y, GL_DEPTH_BUFFER_BIT, GL_NEAREST);
			glUseProgram(0);

            RenderSkybox(app, app->camera);

            // Blur/Bloom
			PassBlitBrightPixels(app, app->fboBloom1, app->displaySize.x / 2, app->displaySize.y / 2, GL_COLOR_ATTACHMENT0, app->mainAttachmentTexture, app->valThreshold);

            glActiveTexture(GL_TEXTURE0);
			glBindTexture(GL_TEXTURE_2D, app->rtBright);
			glGenerateMipmap(GL_TEXTURE_2D);

            // horizontal blur
			PassBlur(app, app->fboBloom1, app->displaySize.x / 2, app->displaySize.y / 2, GL_COLOR_ATTACHMENT1, app->rtBright, 0, 1, 0, app->intensities[0]);
			PassBlur(app, app->fboBloom2, app->displaySize.x / 4, app->displaySize.y / 4, GL_COLOR_ATTACHMENT1, app->rtBright, 1, 1, 0, app->intensities[1]);
            PassBlur(app, app->fboBloom3, app->displaySize.x / 8, app->displaySize.y / 8, GL_COLOR_ATTACHMENT1, app->rtBright, 2, 1, 0, app->intensities[2]);
			PassBlur(app, app->fboBloom4, app->displaySize.x / 16, app->displaySize.y / 16, GL_COLOR_ATTACHMENT1, app->rtBright, 3, 1, 0, app->intensities[3]);
			PassBlur(app, app->fboBloom5, app->displaySize.x / 32, app->displaySize.y / 32, GL_COLOR_ATTACHMENT1, app->rtBright, 4, 1, 0, app->intensities[4]);

			// vertical blur
			PassBlur(app, app->fboBloom1, app->displaySize.x / 2, app->displaySize.y / 2, GL_COLOR_ATTACHMENT0, app->rtBloomH, 0, 0, 1, app->intensities[0]);
			PassBlur(app, app->fboBloom2, app->displaySize.x / 4, app->displaySize.y / 4, GL_COLOR_ATTACHMENT0, app->rtBloomH, 1, 0, 1, app->intensities[1]);
			PassBlur(app, app->fboBloom3, app->displaySize.x / 8, app->displaySize.y / 8, GL_COLOR_ATTACHMENT0, app->rtBloomH, 2, 0, 1, app->intensities[2]);
			PassBlur(app, app->fboBloom4, app->displaySize.x / 16, app->displaySize.y / 16, GL_COLOR_ATTACHMENT0, app->rtBloomH, 3, 0, 1, app->intensities[3]);
			PassBlur(app, app->fboBloom5, app->displaySize.x / 32, app->displaySize.y / 32, GL_COLOR_ATTACHMENT0, app->rtBloomH, 4, 0, 1, app->intensities[4]);

            PassBloom(app, app->bloomBuffer, GL_COLOR_ATTACHMENT0, app->rtBright, MIPMAP_MAX_LEVEL);
            
            glBindFramebuffer(GL_FRAMEBUFFER, 0);

            if (app->showDebugLights) {
                if(app->currentAttachment == "Main")
                    glBindFramebuffer(GL_FRAMEBUFFER, app->bloomBuffer);
                else 
					glBindFramebuffer(GL_FRAMEBUFFER, app->lightBuffer);
                

                // Show lights for debug
                Program& debugLightProgram = app->programs[app->debugLightProgramIdx];
                glUseProgram(debugLightProgram.handle);
                for (int i = 0; i < app->lights.size(); ++i) {
                    Light& light = app->lights[i];
                    u32 meshIdx = 0;
                    float scale = 1.0f;
                    glm::mat4 modelMatrix;
                    if (light.type == LightType_Directional)
                    {
                        meshIdx = app->primitiveIdxs[2];
                        scale = 0.5f;
                        glm::vec3 coneDirection = glm::vec3(0.0f, 1.0f, 0.0f);
                        glm::vec3 lightDirection = glm::normalize(light.direction);
                        glm::vec3 rotationAxis = glm::normalize(glm::cross(lightDirection, coneDirection));
                        float rotationAngle = acos(glm::dot(lightDirection, coneDirection));
                        glm::mat4 rotationMatrix = glm::rotate(glm::mat4(1.0f), rotationAngle, rotationAxis);
                        modelMatrix = TransformPositionRotationScale(light.position, light.direction, vec3(scale));
                        modelMatrix = rotationMatrix * modelMatrix;
                    }
                    else
                    {
                        scale = 0.5f;
                        modelMatrix = TransformPositionRotationScale(light.position, light.direction, vec3(scale));
                        meshIdx = app->primitiveIdxs[1];

                    }
                    Mesh& mesh = app->meshes[app->models[meshIdx].meshIdx];
                    GLuint vao = FindVAO(mesh, 0, debugLightProgram);


                    modelMatrix = app->camera.projection * app->camera.view * modelMatrix;
                    glBindVertexArray(vao);
                    glUniformMatrix4fv(app->uProjectionMatrix, 1, GL_FALSE, &modelMatrix[0][0]);
                    glUniform3f(app->uLightColor, light.color.r, light.color.g, light.color.b);

                    glDrawElements(GL_TRIANGLES, mesh.submeshes[0].indices.size(), GL_UNSIGNED_INT, 0);
                    glBindVertexArray(0);
                }
                glUseProgram(0);
                glBindFramebuffer(GL_FRAMEBUFFER, 0);
            }
			
        }

        
        break; 
    }
}

void AlignUniformBuffers(App* app , Camera cam, bool reflection)
{
    BufferManagement::MapBuffer(app->localUniformBuffer, GL_WRITE_ONLY);

    // Light params
    app->globalParamsOffset = app->localUniformBuffer.head;
	PushVec3(app->localUniformBuffer, cam.position);
	PushUInt(app->localUniformBuffer, app->lights.size());

    for (size_t i = 0; i < app->lights.size(); ++i) {
        BufferManagement::AlignHead(app->localUniformBuffer, sizeof(vec4));

		Light& light = app->lights[i];
		PushUInt(app->localUniformBuffer, light.type);
		PushVec3(app->localUniformBuffer, light.color * light.intensity);
		PushVec3(app->localUniformBuffer, light.direction);
		PushVec3(app->localUniformBuffer, light.position);
    }

    app->globalParamsSize = app->localUniformBuffer.head - app->globalParamsOffset;

    for (auto it = app->entities.begin(); it != app->entities.end(); ++it)
    {
        BufferManagement::AlignHead(app->localUniformBuffer, app->uniformBlockAlignment);
		Entity& entity = *it;
		
		glm::mat4 worldMatrix = entity.worldMatrix;
		glm::mat4 worldViewProjection = cam.projection * cam.view * worldMatrix;

        entity.localParamsOffset = app->localUniformBuffer.head;
		PushMat4(app->localUniformBuffer, worldMatrix);
		PushMat4(app->localUniformBuffer, worldViewProjection);
        entity.localParamsSize = app->localUniformBuffer.head - entity.localParamsOffset;

    }

    // Clipping plane as binding
	BufferManagement::AlignHead(app->localUniformBuffer, app->uniformBlockAlignment);
	app->clippingPlaneOffset = app->localUniformBuffer.head;

	if (reflection) {
		PushVec4(app->localUniformBuffer, vec4(0.0f, 1.0f, 0.0f, -app->waterPos.y)); // Reflective plane
	}
	else {
		PushVec4(app->localUniformBuffer, vec4(0.0f, -1.0f, 0.0f, app->waterPos.y)); // Refractive plane
	}

	app->clippingPlaneSize = app->localUniformBuffer.head - app->clippingPlaneOffset;

    BufferManagement::UnmapBuffer(app->localUniformBuffer);
}

void InitCamera(App* app) {
    app->camera.aspectRatio = (float)app->displaySize.x / (float)app->displaySize.y;
    app->camera.zNear = 0.1f;
    app->camera.zFar = 600.0f;
	app->camera.fov = 60.0f;

	app->camera.position = vec3(1.1, 10.5, 15.39);
	app->camera.front = glm::normalize(vec3(-0.1f, -0.36f, -0.9f));
	app->camera.up = vec3(0.0f, 1.0f, 0.0f);
	app->camera.right = glm::normalize(glm::cross(app->camera.front, app->camera.up));

    app->camera.projection = glm::perspective(glm::radians(60.0f), app->camera.aspectRatio, app->camera.zNear, app->camera.zFar);
	app->camera.view = glm::lookAt(app->camera.position, app->camera.position + app->camera.front, app->camera.up);

	app->camera.speed = 10.0f;
	app->camera.sensitivity = 0.1f;
	app->camera.yaw = -90.0f;
    app->camera.pitch = 0.0f;
}

void CameraMovement(App* app) 
{
    // Camera controls like unity
	// Shift to move faster
	float currentSpeed = app->camera.speed;
	if (app->input.keys[K_SHIFT] == BUTTON_PRESSED)
		currentSpeed = app->camera.speed * 2.0f;

	// WASD + QE movement of the camera, WS used for ZOOM (front movement), AD used for RIGHT/LEFT movement, QE used for UP/DOWN movement	
    if(app->input.keys[K_W] == BUTTON_PRESSED)
		app->camera.position += app->camera.front * currentSpeed * app->deltaTime;
	if (app->input.keys[K_S] == BUTTON_PRESSED)
		app->camera.position -= app->camera.front * currentSpeed * app->deltaTime;
	if (app->input.keys[K_A] == BUTTON_PRESSED)
		app->camera.position -= app->camera.right * currentSpeed * app->deltaTime;
	if (app->input.keys[K_D] == BUTTON_PRESSED)
		app->camera.position += app->camera.right * currentSpeed * app->deltaTime;
	if(app->input.keys[K_Q] == BUTTON_PRESSED)
		app->camera.position -= app->camera.up * currentSpeed * app->deltaTime;
	if (app->input.keys[K_E] == BUTTON_PRESSED)
		app->camera.position += app->camera.up * currentSpeed * app->deltaTime;

    // Drag with middle mouse button 
    if (app->input.mouseButtons[MIDDLE] == BUTTON_PRESSED) {
        float dragSpeed = 0.008f;
        app->camera.position -= app->camera.right * app->input.mouseDelta.x * dragSpeed;
        app->camera.position += app->camera.up * app->input.mouseDelta.y * dragSpeed;
    }

    // Zoom with mouse middle button scroll
    if (app->input.mouseScroll.y != 0.0f) {
		app->camera.fov -= app->input.mouseScroll.y;

        // Needed so the camera dont flip
        if (app->camera.fov < 1.0f)
            app->camera.fov = 1.0f;
        if (app->camera.fov > 60.0f)
            app->camera.fov = 60;

		app->input.mouseScroll.y = 0.0f;  
		app->camera.projection = glm::perspective(glm::radians(app->camera.fov), app->camera.aspectRatio, app->camera.zNear, app->camera.zFar);
    }

	app->camera.view = glm::lookAt(app->camera.position, app->camera.position + app->camera.front, app->camera.up);
}

void CameraLookAt(App* app)
{
    if (app->input.mouseButtons[RIGHT] == ButtonState::BUTTON_PRESSED) {
        app->input.mouseDelta.x *= app->camera.sensitivity;
		app->input.mouseDelta.y *= -app->camera.sensitivity;
		app->camera.yaw += app->input.mouseDelta.x;
		app->camera.pitch += app->input.mouseDelta.y;

        // Avoid loosing controll on camera rotation
		if (app->camera.pitch > 89.0f)
			app->camera.pitch = 89.0f;
		if (app->camera.pitch < -89.0f)
			app->camera.pitch = -89.0f;

		CameraDirection(app->camera);
		
    }
}

void CameraDirection(Camera& cam) 
{
    glm::vec3 direction;
    direction.x = cos(glm::radians(cam.yaw)) * cos(glm::radians(cam.pitch));
    direction.y = sin(glm::radians(cam.pitch));
    direction.z = sin(glm::radians(cam.yaw)) * cos(glm::radians(cam.pitch));
    cam.front = glm::normalize(direction);
    cam.right = glm::normalize(glm::cross(cam.front, cam.up));
}

void GuiAddLights(App* app) 
{
    if (ImGui::BeginMenu("Add light")) {
        if (ImGui::MenuItem("Directional Light", nullptr)) {
			app->directionalLightCount++;
			std::string name = "Directional Light " + std::to_string(app->directionalLightCount);
			app->lights.push_back({ LightType::LightType_Directional, vec3(1.0, 1.0, 1.0), vec3(1.0, -1.0, 1.0), vec3(0.0, 0.0, 0.0), 1.0, name });
        }
        if (ImGui::MenuItem("Point Light", nullptr)) {
			app->pointLightCount++;
			std::string name = "Point Light " + std::to_string(app->pointLightCount);
			app->lights.push_back({ LightType::LightType_Point, vec3(1.0, 1.0, 1.0), vec3(1.0, 1.0, 1.0), vec3(0.0, 1.0, 1.0), 1.0, name });
        }
        if (ImGui::Button("Add 100 point lights")) {
            for (int i = 0; i < 100; ++i) {
                app->pointLightCount++;
                std::string name = "Point Light " + std::to_string(app->pointLightCount);
                app->lights.push_back({ LightType::LightType_Point, vec3(1.0, 1.0, 1.0), vec3(1.0, 1.0, 1.0), vec3(0.0, 1.0, 1.0), 1.0, name });
            }
        }
        if (ImGui::Button("Add 100 directional lights")) {
            for (int i = 0; i < 100; ++i) {
                app->directionalLightCount++;
                std::string name = "Directional Light " + std::to_string(app->directionalLightCount);
                app->lights.push_back({ LightType::LightType_Directional, vec3(1.0, 1.0, 1.0), vec3(1.0, -1.0, 1.0), vec3(0.0, 0.0, 0.0), 1.0, name });
            }
        }
        ImGui::EndMenu();
    }
}

void GuiInspectorLights(App* app) 
{
    if (ImGui::CollapsingHeader("Lights", ImGuiTreeNodeFlags_DefaultOpen)) {
		ImGui::Text("Number of lights: %d", app->lights.size());
		ImGui::Separator();
        for (int i = 0; i < app->lights.size(); ++i) {
            ImGui::PushID(app->lights[i].name.c_str());
            if (ImGui::CollapsingHeader(app->lights[i].name.c_str(), ImGuiTreeNodeFlags_DefaultOpen)) {
                if (app->lights[i].type == LightType_Directional) {
                    ImGui::Text("Position: ");
                    ImGui::DragFloat3("##Position", &app->lights[i].position[0], 0.1f, true);
                    ImGui::Text("Direction: ");
                    if (ImGui::DragFloat3("##Direction", &app->lights[i].direction[0], 0.01f)) {
                        app->lights[i].direction = glm::clamp(app->lights[i].direction, -10.0f, 10.0f);
                    }
                }
                else if (app->lights[i].type == LightType_Point) {
                    ImGui::Text("Position: ");
                    ImGui::DragFloat3("##Position", &app->lights[i].position[0], 0.1f, true);
                    ImGui::DragFloat("##Intensity", &app->lights[i].intensity, 0.1f, 0.00001f, 1.0f);
                }

                ImGui::Text("Color: ");
                ImGui::ColorEdit3("##Color", &app->lights[i].color[0], ImGuiColorEditFlags_Float);
            }
            ImGui::PopID();
        }
    }
    
}

void GuiInspectorEntities(App* app) {

    if (ImGui::CollapsingHeader("Entities", ImGuiTreeNodeFlags_DefaultOpen)) {
        for (int i = 0; i < app->entities.size(); ++i)
        {
            ImGui::PushID(app->entities[i].name.c_str());
            if (ImGui::CollapsingHeader(app->entities[i].name.c_str(), ImGuiTreeNodeFlags_DefaultOpen)) {
                ImGui::Text("Position: ");
                if (ImGui::DragFloat3("##Position", &app->entities[i].position[0], 0.5f, true))
                {
                    app->entities[i].worldMatrix = TransformPositionRotationScale(app->entities[i].position, app->entities[i].rotation, app->entities[i].scale);
                }
                ImGui::Text("Rotation: ");
                if (ImGui::DragFloat3("##Rotation", &app->entities[i].rotation[0], 1.0f, 0.0f, 360.0f))
                {
                    app->entities[i].worldMatrix = TransformPositionRotationScale(app->entities[i].position, app->entities[i].rotation, app->entities[i].scale);
                }
                ImGui::Text("Scale: ");
                if (ImGui::DragFloat3("##Scale", &app->entities[i].scale[0], 0.01f, 0.00001f, 10000.0f))
                {
                    app->entities[i].worldMatrix = TransformPositionRotationScale(app->entities[i].position, app->entities[i].rotation, app->entities[i].scale);
                }
            }
            ImGui::PopID();
        }
    }
    
}

void GuiAddPrimitive(App* app) 
{
    if (ImGui::BeginMenu("Add primitive")) 
    {
        for (size_t i = 0; i < app->primitiveIdxs.size(); ++i) 
        {
            if (ImGui::MenuItem(app->primitives[i].c_str(), nullptr)) 
            {
                app->primitiveCount++;
                std::string name = app->primitives[i].c_str();
				name += " " + std::to_string(app->primitiveCount);
                Entity e; 
				e.modelIndex = app->primitiveIdxs[i];
				e.position = vec3(0.0f, 0.0f, 0.0f);
				e.rotation = vec3(0.0f, 0.0f, 0.0f);
				e.scale = vec3(1.0f, 1.0f, 1.0f);
				e.worldMatrix = TransformPositionRotationScale(e.position, e.rotation, e.scale);
				e.name = name;

                // Asignar un material por defecto si no existe
                Model& model = app->models[e.modelIndex];
                for (u32 j = 0; j < model.materialIdx.size(); ++j) {
                    if (model.materialIdx[j] == UINT32_MAX) {
                        Material defaultMaterial = {};
                        defaultMaterial.albedoTextureIdx = app->normalTexIdx; 
                        model.materialIdx[j] = app->materials.size();
                        app->materials.push_back(defaultMaterial);
                    }
                }

				app->entities.push_back(e);
            }
        }
        ImGui::EndMenu();
    }
	
}

void GuiInspectorCamera(App* app) 
{
    if (ImGui::CollapsingHeader("Camera", ImGuiTreeNodeFlags_DefaultOpen)) 
    {
		ImGui::Text("Position: ");
		ImGui::DragFloat3("##Position", &app->camera.position[0], 0.5f, true);
		ImGui::Text("Front: ");
		ImGui::DragFloat3("##Front", &app->camera.front[0], 0.5f, true);
		ImGui::Text("Speed: ");
		ImGui::DragFloat("##Speed", &app->camera.speed, 0.1f, 0.00001f, 10000.0f);
    }
}

void DrawScene(App* app, u32 programIdx, GLuint fbo, Camera camera, WaterScenePart part) 
{
   
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glBindFramebuffer(GL_FRAMEBUFFER, fbo);
    glViewport(0, 0, app->displaySize.x, app->displaySize.y);

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);


    Program& texturedMeshProgram = app->programs[programIdx];
    glUseProgram(texturedMeshProgram.handle);

    glBindBufferRange(GL_UNIFORM_BUFFER, 0, app->localUniformBuffer.handle, app->globalParamsOffset, app->globalParamsSize);

    for (int i = 0; i < app->entities.size(); ++i) {
        Entity entity = app->entities[i];
        Model& model = app->models[entity.modelIndex];
        Mesh& mesh = app->meshes[model.meshIdx];

        glBindBufferRange(GL_UNIFORM_BUFFER, 1, app->localUniformBuffer.handle, entity.localParamsOffset, entity.localParamsSize);
        glBindBufferRange(GL_UNIFORM_BUFFER, 2, app->localUniformBuffer.handle, app->clippingPlaneOffset, app->clippingPlaneSize);

        for (u32 i = 0; i < mesh.submeshes.size(); ++i) {
            GLuint vao = FindVAO(mesh, i, texturedMeshProgram);
            glBindVertexArray(vao);
            u32 submeshMaterialIdx = model.materialIdx[i];
            Material& submeshMaterial = app->materials[submeshMaterialIdx];

            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, app->textures[submeshMaterial.albedoTextureIdx].handle);
            glUniform1i(app->texturedMeshProgram_uTexture, 0);
            glUniform1f(app->texturedMeshProgram_uNear, app->camera.zNear);
            glUniform1f(app->texturedMeshProgram_uFar, app->camera.zFar);

            Submesh& submesh = mesh.submeshes[i];
            glDrawElements(GL_TRIANGLES, submesh.indices.size(), GL_UNSIGNED_INT, (void*)(u64)submesh.indexOffset);
            glBindVertexArray(0);
        }
    }
    glUseProgram(0);
}

void PassWaterScene(App* app, Camera camera, GLuint fbo, WaterScenePart part) 
{
	glEnable(GL_DEPTH_TEST);
    glEnable(GL_CLIP_DISTANCE0);

	DrawScene(app, app->texturedMeshProgramIdx, fbo, camera, part);

	glDisable(GL_CLIP_DISTANCE0);
}

void RenderSkybox(App* app, Camera camera) 
{
    //Skybox pass
    //glBindFramebuffer(GL_FRAMEBUFFER, app->lightBuffer);

    glDepthMask(GL_FALSE);             // Disable depth writing
    glEnable(GL_DEPTH_TEST);           // Still test depth
    glDepthFunc(GL_LEQUAL);            // Allow equal depth to show background

    Program& skyboxProgram = app->programs[app->cubemapProgramIdx];
    glUseProgram(skyboxProgram.handle);
    glm::mat4 view = glm::mat4(glm::mat3(camera.view)); // remove translation from the view matrix
    glUniformMatrix4fv(app->uSkyboxView, 1, GL_FALSE, glm::value_ptr(view));
    glUniformMatrix4fv(app->uSkyboxProjection, 1, GL_FALSE, &camera.projection[0][0]);
    glUniform1i(app->uSkybox, 9);

    // skybox cube
    glBindVertexArray(app->skyboxVAO);
    glActiveTexture(GL_TEXTURE9);
    glBindTexture(GL_TEXTURE_CUBE_MAP, app->rtCubemap);
    glDrawArrays(GL_TRIANGLES, 0, 36);
    glBindVertexArray(0);

    glDepthMask(GL_TRUE);
    glDepthFunc(GL_LESS);
    glUseProgram(0);
}