#include "images.hpp"

#include "glDebug.hpp"

#include <glad/glad.h>

#include <cassert>
#include <iostream>

void renderToImage(size_t width, size_t height, size_t numComponents, unsigned char *outPixels, std::function<void()> drawScene) {
  GLint previousTextureObject = 0;
  GLint previousFramebufferObject = 0;

  // Save previous GL state that we will change in order to put it back after
  GLCALL(glGetIntegerv(GL_TEXTURE_BINDING_2D, &previousTextureObject));
  GLCALL(glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING, &previousFramebufferObject));

  GLuint textureObject = 0;

  GLCALL(glGenTextures(1, &textureObject));

  GLCALL(glBindTexture(GL_TEXTURE_2D, textureObject));

  // Lets avoid warnings
  const auto w = GLsizei(width);
  const auto h = GLsizei(height);

  // if we want better quality, we can use multisampling, but for testing
  // purpose it is useless todo replace with glTexStorage2DMultisample (in that
  // case need to todo glBlitFramebuffer in another one in order to be able to
  // glGetTexImage)
  // https://stackoverflow.com/questions/14019910/how-does-glteximage2dmultisample-work
  GLCALL(glTexStorage2D(GL_TEXTURE_2D, 1, GL_RGBA32F, w, h));

  GLuint depthTexture;
  GLCALL(glGenTextures(1, &depthTexture));

  GLCALL(glBindTexture(GL_TEXTURE_2D, depthTexture));

  GLCALL(glTexStorage2D(GL_TEXTURE_2D, 1, GL_DEPTH_COMPONENT32F, w, h));

  GLCALL(glBindTexture(GL_TEXTURE_2D, previousTextureObject));

  GLuint framebufferObject = 0;
  GLCALL(glGenFramebuffers(1, &framebufferObject));
  GLCALL(glBindFramebuffer(GL_DRAW_FRAMEBUFFER, framebufferObject));

  GLCALL(glFramebufferTexture(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, textureObject, 0));
  GLCALL(glFramebufferTexture(GL_DRAW_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, depthTexture, 0));

  GLenum drawBuffers[1] = {GL_COLOR_ATTACHMENT0};
  GLCALL(glDrawBuffers(1, drawBuffers));

  const auto framebufferStatus = glCheckFramebufferStatus(GL_DRAW_FRAMEBUFFER);
  assert(framebufferStatus == GL_FRAMEBUFFER_COMPLETE);

  drawScene();

  GLint currentlyBoundFBO = 0;
  GLCALL(glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING, &currentlyBoundFBO));
  if (currentlyBoundFBO != framebufferObject) {
    // Display a warning on clog
    // It may not be an error because the drawScene() function might have render
    // to the framebuffer but unbound it after.
    std::clog
        << "Warning: renderToImage - GL_DRAW_FRAMEBUFFER_BINDING has "
           "changed during drawScene. It might lead to unexpected behavior."
        << std::endl;
  }

  GLCALL(glBindTexture(GL_TEXTURE_2D, textureObject));
  GLCALL(glGetTexImage(GL_TEXTURE_2D, 0, numComponents == 3 ? GL_RGB : GL_RGBA, GL_UNSIGNED_BYTE, outPixels));

  GLCALL(glBindTexture(GL_TEXTURE_2D, previousTextureObject));
  GLCALL(glBindFramebuffer(GL_DRAW_FRAMEBUFFER, previousFramebufferObject));
}