#include "saiga/opengl/uniformBuffer.h"


UniformBuffer::UniformBuffer() : Buffer(GL_UNIFORM_BUFFER)
{

}

UniformBuffer::~UniformBuffer()
{
}




void UniformBuffer::bind(GLuint bindingPoint) const
{
    Buffer::bind();
    glBindBufferBase(target, bindingPoint, buffer);
    assert_no_glerror();
}


void UniformBuffer::init(Shader *shader, GLuint location)
{
    size = shader->getUniformBlockSize(location);
    assert_no_glerror();
    createGLBuffer(nullptr,size,GL_DYNAMIC_DRAW);

}


std::ostream &operator<<(std::ostream &os, const UniformBuffer &ub){
    os<<"UniformBuffer "<<"size="<<ub.size;
    return os;
}

GLint UniformBuffer::getMaxUniformBlockSize()
{
    GLint ret;
    glGetIntegerv(GL_MAX_UNIFORM_BLOCK_SIZE,&ret);
    return ret;
}

GLint UniformBuffer::getMaxUniformBufferBindings()
{
    GLint ret;
    glGetIntegerv(GL_MAX_UNIFORM_BUFFER_BINDINGS,&ret);
    return ret;
}
