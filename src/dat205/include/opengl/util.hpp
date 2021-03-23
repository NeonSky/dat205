#pragma once

#include <GL/glew.h>
#include <functional>

void bind_buffer(GLenum target, GLuint buffer, std::function<void()> f);
void bind_texture(GLenum target, GLuint texture, std::function<void()> f);