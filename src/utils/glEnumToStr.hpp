#pragma once

#include <glad/glad.h>

#include <unordered_map>

#define ENUM_TEXT_PAIR(x) { x, #x }

namespace glEnumToStr {

    template< template <typename...> class mapContainer, typename ... Ts >
    const char* find(const mapContainer<GLenum, const char*, Ts...>& map, GLenum value) {
        const auto it = map.find(value);
        if (it == end(map)) return "UNDEFINED";
        return (*it).second;
    };

    static const std::unordered_map<GLenum, const char*> source = {
        ENUM_TEXT_PAIR(GL_DEBUG_SOURCE_API),
        ENUM_TEXT_PAIR(GL_DEBUG_SOURCE_WINDOW_SYSTEM), 
        ENUM_TEXT_PAIR(GL_DEBUG_SOURCE_SHADER_COMPILER),
        ENUM_TEXT_PAIR(GL_DEBUG_SOURCE_THIRD_PARTY),
        ENUM_TEXT_PAIR(GL_DEBUG_SOURCE_APPLICATION),
        ENUM_TEXT_PAIR(GL_DEBUG_SOURCE_OTHER)
    };

    static const std::unordered_map<GLenum, const char*> type = {
        ENUM_TEXT_PAIR(GL_DEBUG_TYPE_ERROR),
        ENUM_TEXT_PAIR(GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR),
        ENUM_TEXT_PAIR(GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR),
        ENUM_TEXT_PAIR(GL_DEBUG_TYPE_PORTABILITY),
        ENUM_TEXT_PAIR(GL_DEBUG_TYPE_PERFORMANCE),
		ENUM_TEXT_PAIR(GL_DEBUG_TYPE_MARKER),
		ENUM_TEXT_PAIR(GL_DEBUG_TYPE_PUSH_GROUP),
		ENUM_TEXT_PAIR(GL_DEBUG_TYPE_POP_GROUP),
		ENUM_TEXT_PAIR(GL_DEBUG_TYPE_OTHER)
    };

    static const std::unordered_map<GLenum, const char*> severity = {
        ENUM_TEXT_PAIR(GL_DEBUG_SEVERITY_HIGH),
		ENUM_TEXT_PAIR(GL_DEBUG_SEVERITY_MEDIUM),
        ENUM_TEXT_PAIR(GL_DEBUG_SEVERITY_LOW),
        ENUM_TEXT_PAIR(GL_DEBUG_SEVERITY_NOTIFICATION)
    };

    static const std::unordered_map<GLenum, const char*> error = {
        ENUM_TEXT_PAIR(GL_NO_ERROR),
        ENUM_TEXT_PAIR(GL_INVALID_ENUM),
        ENUM_TEXT_PAIR(GL_INVALID_VALUE),
        ENUM_TEXT_PAIR(GL_INVALID_OPERATION),
		ENUM_TEXT_PAIR(GL_STACK_OVERFLOW),
		ENUM_TEXT_PAIR(GL_STACK_UNDERFLOW),
		ENUM_TEXT_PAIR(GL_OUT_OF_MEMORY),
        ENUM_TEXT_PAIR(GL_INVALID_FRAMEBUFFER_OPERATION),
    };

    static const std::unordered_map<GLenum, const char*> framebufferStatus = {
        ENUM_TEXT_PAIR(GL_FRAMEBUFFER_UNDEFINED),
        ENUM_TEXT_PAIR(GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT),
        ENUM_TEXT_PAIR(GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT),
        ENUM_TEXT_PAIR(GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER),
		ENUM_TEXT_PAIR(GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER),
		ENUM_TEXT_PAIR(GL_FRAMEBUFFER_UNSUPPORTED),
		ENUM_TEXT_PAIR(GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE),
        ENUM_TEXT_PAIR(GL_FRAMEBUFFER_INCOMPLETE_LAYER_TARGETS),
    };

    static const std::unordered_map<GLenum, const char*> shaderType = {
        ENUM_TEXT_PAIR(GL_COMPUTE_SHADER),
        ENUM_TEXT_PAIR(GL_VERTEX_SHADER),
        ENUM_TEXT_PAIR(GL_TESS_CONTROL_SHADER),
        ENUM_TEXT_PAIR(GL_TESS_EVALUATION_SHADER),
		ENUM_TEXT_PAIR(GL_GEOMETRY_SHADER),
		ENUM_TEXT_PAIR(GL_FRAGMENT_SHADER)
    };

}
