#pragma once

#include <glad/glad.h>
#include "glEnumToStr.hpp"

#include <iostream>

#ifdef _MSC_VER
	#define DEBUG_BREAK  __debugbreak
#else
	#define DEBUG_BREAK __builtin_trap
#endif

// Assertion and logger handling for opengl functions before OpenGL 4.3 and support of glDebugMessageCallback.
#ifndef NDEBUG
    #define GLCALL(x) [&](){glDebug::ExitScopeGLCheck exitChecker{#x, __FILE__, __LINE__, glGetError()}; return x;}()
#else
    #define GLCALL(x) x
#endif

namespace glDebug {

    struct ExitScopeGLCheck {
        const char* fName;
        const char* file;
        long line;
        GLenum preError;
        ~ExitScopeGLCheck() {
            GLenum postError;

            while (preError || (postError = glGetError()) != GL_NO_ERROR) {
                GLenum error = preError != GL_NO_ERROR ? preError : postError;
                std::cerr << "OpenGL Error[" << error << "] " << glEnumToStr::find(glEnumToStr::error, error) <<
                " encountered " << (error == preError ? "before" : "while") << " executing \"" << fName << "\" [" << file << "(" << line <<  ")]" << std::endl;
                DEBUG_BREAK();
            }
        }
    };

    void logGLDebugInfo(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar *message, GLvoid *userParam);
    void initGLDebugOutput();
}
