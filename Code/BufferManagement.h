#ifndef BUFFERMANAGEMENT
#define BUFFERMANAGEMENT

#include "platform.h"
#include <glad/glad.h>


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