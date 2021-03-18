#pragma once

#include <assert.h>
#include <glad/glad.h>

#ifdef _MSC_VER
	#define debug_break __debugbreak
#else
	#define debug_break __builtin_trap
#endif

// Assertion and logger handling for opengl functions
#if defined(NDEBUG)
    #define GLCall(x) x
#else
    #define break_assert(x) if (!x) { debug_break(); assert(false); }
    #define GLCall(x) glexp::clear(); x; break_assert(glexp::doesFunctionWorks(#x, __FILE__, __LINE__))
#endif

namespace glexp {

    // Empty the OpenGl error buffer
    void clear();

    /**
     * @brief Print OpenGl errors to the console if any
     * 
     * @param functionName
     * @param filename
     * @param line
     */
    bool doesFunctionWorks(const char* functionName, const char* filename, int line);

    /**
     * @brief Transform error enum to text for OpenGL
     * 
     * @param err 
     * @return char const* - Text corresponding to OpenGl error
     */
    char const* glErrorString(const GLenum err);

}
