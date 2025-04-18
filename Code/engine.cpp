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
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
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
	e.position = vec3(0.0f, 5.0f, 0.0f);
	e.rotation = vec3(0.0f, 0.0f, 0.0f);
	e.scale = vec3(1.0f, 1.0f, 1.0f);
	e.modelIndex = patrickModel;
	e.worldMatrix = TransformPositionRotationScale(e.position, e.rotation, e.scale);
	e.name = "Patrick";
	app->entities.push_back(e);

    // Lights
	app->lights.push_back(Light(LightType_Directional, vec3(1.0f, 1.0f, 1.0f), vec3(-1.0f, -1.0f, -1.0f), vec3(0.0f, 0.0f, 0.0f), 1.0f, "Directional Light"));
	app->lights.push_back(Light(LightType_Point, vec3(1.0f, 1.0f, 1.0f), vec3(0.0f, 0.0f, 0.0f), vec3(2.0f, 2.0f, 2.0f), 1.0f, "Point Light"));


    // Load shaders
    app->forwardProgramIdx = LoadProgram(app, "RENDER_GEOMETRY.glsl", "RENDER_GEOMETRY");
	app->texturedMeshProgram_uTexture = glGetUniformLocation(app->programs[app->forwardProgramIdx].handle, "uTexture");

	app->texturedMeshProgramIdx = LoadProgram(app, "shaders.glsl", "GEOMETRY_RENDER");
	app->lightProgramIdx = LoadProgram(app, "shaders.glsl", "LIGHTING_RENDER");
	
    app->mode = Mode_Forward;
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

    // Final texture
	app->mainAttachmentTexture = CreateTextureAttachment(GL_RGBA16F, GL_RGBA, GL_FLOAT, app->displaySize.x, app->displaySize.y);

    // Depth component 
    GLuint depthLightAttachmentHandle;
	depthLightAttachmentHandle = CreateTextureAttachment(GL_DEPTH_COMPONENT24, GL_DEPTH_COMPONENT, GL_FLOAT, app->displaySize.x, app->displaySize.y);

    // Light buffer
    glGenFramebuffers(1, &app->lightBuffer);
    glBindFramebuffer(GL_FRAMEBUFFER, app->lightBuffer);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, app->mainAttachmentTexture, 0);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, depthLightAttachmentHandle, 0);

	CheckFramebufferStatus();

	GLuint drawBuffersLight[] = { GL_COLOR_ATTACHMENT0 };
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

	// Set default attachment
	app->currentAttachment = "Main";
	app->renderSelector["Color"] = app->colorAttachmentTexture;
	app->renderSelector["Position"] = app->positionAttachmentTexture;
	app->renderSelector["Normal"] = app->normalAttachmentTexture;
	app->renderSelector["Depth"] = app->depthAttachmentTexture;
	app->renderSelector["Main"] = app->mainAttachmentTexture;
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

    AlignUniformBuffers(app);
}

void Render(App* app)
{

    glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
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
                    glUniform1i(app->texturedMeshProgram_uTexture, 0);

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
			// Geometry Pass
			glBindFramebuffer(GL_FRAMEBUFFER, app->gBuffer);
            glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
			glEnable(GL_DEPTH_TEST);

			Program& texturedMeshProgram = app->programs[app->texturedMeshProgramIdx];
			glUseProgram(texturedMeshProgram.handle);

            for (int i = 0; i < app->entities.size(); ++i) {
				Entity entity = app->entities[i];
				Model& model = app->models[entity.modelIndex];  
				Mesh& mesh = app->meshes[model.meshIdx];

				glBindBufferRange(GL_UNIFORM_BUFFER, 1, app->localUniformBuffer.handle, entity.localParamsOffset, entity.localParamsSize);

                for (u32 i = 0; i < mesh.submeshes.size(); ++i) {
					GLuint vao = FindVAO(mesh, i, texturedMeshProgram);
					glBindVertexArray(vao);
					u32 submeshMaterialIdx = model.materialIdx[i];
					Material& submeshMaterial = app->materials[submeshMaterialIdx];

					glActiveTexture(GL_TEXTURE0);
					glBindTexture(GL_TEXTURE_2D, app->textures[submeshMaterial.albedoTextureIdx].handle);
					glUniform1i(glGetUniformLocation(texturedMeshProgram.handle, "uTexture"), 0);
					glUniform1f(glGetUniformLocation(texturedMeshProgram.handle, "uNear"), app->camera.zNear);
					glUniform1f(glGetUniformLocation(texturedMeshProgram.handle, "uFar"), app->camera.zFar);

					Submesh& submesh = mesh.submeshes[i];
					glDrawElements(GL_TRIANGLES, submesh.indices.size(), GL_UNSIGNED_INT, (void*)(u64)submesh.indexOffset);
					glBindVertexArray(0);
                }
            }
			glUseProgram(0);
			glBindFramebuffer(GL_FRAMEBUFFER, 0);


			// Light Pass
			glBindFramebuffer(GL_FRAMEBUFFER, app->lightBuffer);

            glClearColor(0.1, 0.1, 0.1, 1.0);
            glClear(GL_COLOR_BUFFER_BIT);

			Program& lightProgram = app->programs[app->lightProgramIdx];
			glUseProgram(lightProgram.handle);

			glBindVertexArray(app->vao);
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, app->embeddedElements);
			glUniform1i(glGetUniformLocation(lightProgram.handle, "uAlbedo"), 0);
			glUniform1i(glGetUniformLocation(lightProgram.handle, "uPosition"), 1);
			glUniform1i(glGetUniformLocation(lightProgram.handle, "uNormal"), 2);

			glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, app->colorAttachmentTexture);
			glActiveTexture(GL_TEXTURE1);
			glBindTexture(GL_TEXTURE_2D, app->positionAttachmentTexture);
			glActiveTexture(GL_TEXTURE2);
			glBindTexture(GL_TEXTURE_2D, app->normalAttachmentTexture);

            //Bind uniforms
			glBindBufferRange(GL_UNIFORM_BUFFER, 0, app->localUniformBuffer.handle, app->globalParamsOffset, app->globalParamsSize);

			glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_SHORT, 0);
			glBindVertexArray(0);

			glBindFramebuffer(GL_READ_FRAMEBUFFER, app->gBuffer);
			glBindFramebuffer(GL_DRAW_FRAMEBUFFER, app->lightBuffer);

			glBlitFramebuffer(0, 0, app->displaySize.x, app->displaySize.y, 0, 0, app->displaySize.x, app->displaySize.y, GL_DEPTH_BUFFER_BIT, GL_NEAREST);
			glUseProgram(0);
			glBindFramebuffer(GL_FRAMEBUFFER, 0);
        }
        break; 
    }
}

void AlignUniformBuffers(App* app)
{
    BufferManagement::MapBuffer(app->localUniformBuffer, GL_WRITE_ONLY);

    // Light params
    app->globalParamsOffset = app->localUniformBuffer.head;
	PushVec3(app->localUniformBuffer, app->camera.position);
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
		glm::mat4 worldViewProjection = app->camera.projection * app->camera.view * worldMatrix;

        entity.localParamsOffset = app->localUniformBuffer.head;
		PushMat4(app->localUniformBuffer, worldMatrix);
		PushMat4(app->localUniformBuffer, worldViewProjection);
        entity.localParamsSize = app->localUniformBuffer.head - entity.localParamsOffset;

    }

    BufferManagement::UnmapBuffer(app->localUniformBuffer);
}

void InitCamera(App* app) {
    app->camera.aspectRatio = (float)app->displaySize.x / (float)app->displaySize.y;
    app->camera.zNear = 0.1f;
    app->camera.zFar = 100.0f;
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

        glm::vec3 direction; 
		direction.x = cos(glm::radians(app->camera.yaw)) * cos(glm::radians(app->camera.pitch));
		direction.y = sin(glm::radians(app->camera.pitch));
		direction.z = sin(glm::radians(app->camera.yaw)) * cos(glm::radians(app->camera.pitch));
		app->camera.front = glm::normalize(direction);  
		app->camera.right = glm::normalize(glm::cross(app->camera.front, app->camera.up));
		
    }
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

        ImGui::EndMenu();
    }
}

void GuiInspectorLights(App* app) 
{
    for (int i = 0; i < app->lights.size(); ++i) {
        ImGui::PushID(app->lights[i].name.c_str());
        if (ImGui::CollapsingHeader(app->lights[i].name.c_str(), ImGuiTreeNodeFlags_DefaultOpen)) {
            if (app->lights[i].type == LightType_Directional) {
                ImGui::Text("Position: ");
				ImGui::DragFloat3("##Position", &app->lights[i].position[0], 0.1f, true);
				ImGui::Text("Direction: ");
				ImGui::DragFloat3("##Direction", &app->lights[i].direction[0], 0.1f, true);
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

void GuiInspectorEntities(App* app) {
    
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