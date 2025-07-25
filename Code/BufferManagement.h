#ifndef BUFFERMANAGEMENT
#define BUFFERMANAGEMENT

#include "platform.h"
#include <glad/glad.h>




#define CreateConstantBuffer(size) BufferManagement::CreateBuffer(size, GL_UNIFORM_BUFFER, GL_STREAM_DRAW)
#define CreateStaticVertexBuffer(size) BufferManagement::CreateBuffer(size, GL_ARRAY_BUFFER, GL_STATIC_DRAW)
#define CreateStaticIndexBuffer(size) BufferManagement::CreateBuffer(size, GL_ELEMENT_ARRAY_BUFFER, GL_STATIC_DRAW)

#define PushData(buffer, data, size) BufferManagement::PushAlignedData(buffer, data, size, 1)
#define PushUInt(buffer, value) { u32 v = value; BufferManagement::PushAlignedData(buffer, &v, sizeof(v), 4); }
#define PushVec3(buffer, value) BufferManagement::PushAlignedData(buffer, value_ptr(value), sizeof(value), sizeof(vec4))
#define PushVec4(buffer, value) BufferManagement::PushAlignedData(buffer, value_ptr(value), sizeof(value), sizeof(vec4))
#define PushMat3(buffer, value) BufferManagement::PushAlignedData(buffer, value_ptr(value), sizeof(value), sizeof(vec4))
#define PushMat4(buffer, value) BufferManagement::PushAlignedData(buffer, value_ptr(value), sizeof(value), sizeof(vec4))

struct Buffer {
	GLsizei size;
	GLenum type;
	GLuint handle;
	u8* data;
	u32 head;
};

namespace BufferManagement
{
	bool IsPowerOf2(u32 value);
	u32  Align(u32 value, u32 alignment);
	Buffer CreateBuffer(u32 size, GLenum type, GLenum usage);
	void  BindBuffer(const Buffer& buffer);
	void  MapBuffer(Buffer& buffer, GLenum access);
	void  UnmapBuffer(Buffer& buffer);
	void  AlignHead(Buffer& buffer, u32 alignment);
	void  PushAlignedData(Buffer& buffer, const void* data, u32 size, u32 alignment);
}

#endif // !BUFFERMANAGEMENT