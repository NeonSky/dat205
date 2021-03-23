#include "opengl/util.hpp"

void bind_buffer(GLenum target, GLuint buffer, std::function<void()> f) {
    glBindBuffer(target, buffer);

    f();

    glBindBuffer(target, 0);
}

void bind_texture(GLenum target, GLuint texture, std::function<void()> f) {
    glBindTexture(target, texture);

    f();

    glBindTexture(target, 0);
}