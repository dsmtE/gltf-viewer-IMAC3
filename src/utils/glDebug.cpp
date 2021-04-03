#include "glDebug.hpp"

#include "glEnumToStr.hpp"

namespace glDebug {

    void logGLDebugInfo(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar *message, GLvoid *userParam) {
		const char* sourceStr = glEnumToStr::find(glEnumToStr::source, source);
		const char* typeStr = glEnumToStr::find(glEnumToStr::type, type);
		const char* severityStr = glEnumToStr::find(glEnumToStr::severity, severity);
		std::cerr << "OpenGL: " << message << " [source=" << sourceStr << " type=" << typeStr << " severity=" << severityStr << " id=" << id << "]" << std::endl;
		DEBUG_BREAK();
	}
    
    void initGLDebugOutput() {
		int flags;
    	glGetIntegerv(GL_CONTEXT_FLAGS, &flags);
		if (flags & GL_CONTEXT_FLAG_DEBUG_BIT) {
			glEnable(GL_DEBUG_OUTPUT);
			glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS); // to ensure that errors are thrown in the scope of the function that produces them
			glDebugMessageCallback((GLDEBUGPROC)logGLDebugInfo, nullptr);
			glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DONT_CARE, 0, nullptr, GL_TRUE); // enable all
			// Ignore all low severity
			glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DEBUG_SEVERITY_LOW, 0, nullptr, GL_FALSE);
			// Ignore all PERFORMANCE medium severity
			glDebugMessageControl(GL_DONT_CARE, GL_DEBUG_TYPE_PERFORMANCE, GL_DEBUG_SEVERITY_MEDIUM, 0, nullptr, GL_FALSE);
			// Ignore all notifications
			glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DEBUG_SEVERITY_NOTIFICATION, 0, nullptr, GL_FALSE);
		}else {
			std::clog << "Debug context for OpenGL not supported by your system." << std::endl;
		}
	}
}
