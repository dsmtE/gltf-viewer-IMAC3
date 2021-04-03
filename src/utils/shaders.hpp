#pragma once

#include "filesystem.hpp"
#include "glDebug.hpp"

#include <glad/glad.h>
#include <glm/gtc/type_ptr.hpp>

#include <fstream>
#include <iostream>
#include <memory>
#include <sstream>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <cassert>

class GLShader {
  GLuint id_ = 0;
  typedef std::unique_ptr<char[]> CharBuffer;

public:
  GLShader(const GLenum type) : id_(GLCALL(glCreateShader(type))) {}

  ~GLShader() { GLCALL(glDeleteShader(id_)); }

  GLShader(const GLShader &) = delete;

  GLShader &operator=(const GLShader &) = delete;

  GLShader(GLShader &&rvalue) : id_(rvalue.id_) { rvalue.id_ = 0; }

  GLShader &operator=(GLShader &&rvalue) {
    this->~GLShader();
    id_ = rvalue.id_;
    rvalue.id_ = 0;
    return *this;
  }

  inline GLuint id() const { return id_; }

  void setSource(const GLchar *src) { GLCALL(glShaderSource(id_, 1, &src, 0)); }

  void setSource(const std::string &src) { setSource(src.c_str()); }

  bool compile() {
    GLCALL(glCompileShader(id_));
    return getCompileStatus();
  }

  bool getCompileStatus() const {
    GLint status;
    GLCALL(glGetShaderiv(id_, GL_COMPILE_STATUS, &status));
    return status == GL_TRUE;
  }

  std::string getInfoLog() const {
    GLint logLength;
    GLCALL(glGetShaderiv(id_, GL_INFO_LOG_LENGTH, &logLength));

    CharBuffer buffer(new char[logLength]);
    GLCALL(glGetShaderInfoLog(id_, logLength, 0, buffer.get()));

    return std::string(buffer.get());
  }
};

inline std::string loadShaderSource(const fs::path &filepath) {
  std::ifstream input(filepath.string());
  if (!input) {
    std::stringstream ss;
    ss << "Unable to open file " << filepath;
    throw std::runtime_error(ss.str());
  }

  std::stringstream buffer;
  buffer << input.rdbuf();

  return buffer.str();
}

template <typename StringType>
GLShader compileShader(GLenum type, StringType &&src) {
  GLShader shader(type);
  shader.setSource(std::forward<StringType>(src));
  if (!shader.compile()) throw std::runtime_error(shader.getInfoLog());
  return shader;
}

// Load and compile a shader according to the following naming convention:
// *.vs.glsl -> vertex shader
// *.fs.glsl -> fragment shader
// *.gs.glsl -> geometry shader
// *.cs.glsl -> compute shader
inline GLShader loadShader(const fs::path& shaderPath) {
  static std::unordered_map<std::string, GLenum> extToShaderType = {
    {".vs", GL_VERTEX_SHADER},
    {".fs", GL_FRAGMENT_SHADER},
    {".gs", GL_GEOMETRY_SHADER},
    {".cs", GL_COMPUTE_SHADER}
  };

  const auto ext = shaderPath.stem().extension();
  const auto it = extToShaderType.find(ext.string());
  if (it == end(extToShaderType)) {
    std::cerr << "Unrecognized shader extension " << ext << std::endl;
    throw std::runtime_error("Unrecognized shader extension " + ext.string());
  }

  const GLenum shaderType = (*it).second;
  std::clog << "Compiling " << glEnumToStr::find(glEnumToStr::shaderType, shaderType) << " (" << shaderPath.string() << ")" << std::endl;

  GLShader shader(shaderType);
  shader.setSource(loadShaderSource(shaderPath));
  shader.compile();
  if (!shader.getCompileStatus())
    throw std::runtime_error("Shader compilation error:" + shader.getInfoLog());
  
  return shader;
}

class GLProgram {
  GLuint id_;
  std::unordered_map<std::string, GLint> uniformLocationCache_;
  std::unordered_map<std::string, bool> uniformMissingWarning_;

public:
  GLProgram() : id_(GLCALL(glCreateProgram())) {}

  ~GLProgram() { GLCALL(glDeleteProgram(id_));}

  GLProgram(const GLProgram &) = delete;

  GLProgram &operator=(const GLProgram &) = delete;

  GLProgram(GLProgram &&rvalue) : id_(rvalue.id_) { rvalue.id_ = 0; }

  GLProgram &operator=(GLProgram &&rvalue) {
    this->~GLProgram();
    id_ = rvalue.id_;
    rvalue.id_ = 0;
    return *this;
  }

  GLuint glId() const { return id_; }

  void attachShader(const GLShader &shader) {
    GLCALL(glAttachShader(id_, shader.id()));
  }

  bool link() {
    GLCALL(glLinkProgram(id_));
    return getLinkStatus();
  }

  bool getLinkStatus() const {
    GLint linkStatus;
    GLCALL(glGetProgramiv(id_, GL_LINK_STATUS, &linkStatus));
    return linkStatus == GL_TRUE;
  }

  std::string getInfoLog() const {
    GLint logLength;
    GLCALL(glGetProgramiv(id_, GL_INFO_LOG_LENGTH, &logLength));

    std::string buffer(logLength, '0');
    GLCALL(glGetProgramInfoLog(id_, logLength, 0, buffer.data()));

    return buffer;
  }

  void use() const { GLCALL(glUseProgram(id_));}

  GLint getAttribLocation(const GLchar *name) const {
    GLint location = GLCALL(glGetAttribLocation(id_, name));
    return location;
  }

  inline void bindAttribLocation(GLuint index, const GLchar *name) const {
    GLCALL(glBindAttribLocation(id_, index, name));
  }

  GLint getUniform(const std::string &uniformName) {
    if (uniformLocationCache_.find(uniformName) != uniformLocationCache_.end()) {
      return uniformLocationCache_[uniformName];
    }

    const GLint location = GLCALL(glGetUniformLocation(id_, uniformName.c_str()));

#ifndef NDEBUG
    if(location == -1 && !uniformMissingWarning_[uniformName]) {
      uniformMissingWarning_[uniformName] = true;
      std::cerr << "[Shader] uniform \"" << uniformName << "\" doesn't exist or is not used !" << std::endl;
      return -1;
    }
#endif

    uniformLocationCache_[uniformName] = location;
    return location;
  }

  void setInt(const char* uniformName, const int value) {
    const GLuint loc = getUniform(uniformName);
    if(loc != -1) GLCALL(glUniform1i(loc, value));
  }

  void setFloat(const char* uniformName, const float value) {
    const GLuint loc = getUniform(uniformName);
    if(loc != -1) GLCALL(glUniform1f(loc, value));
  }
  void setFloat(const char* uniformName, const float* values, const size_t& size) {
    const GLuint loc = getUniform(uniformName);
    if(loc != -1) GLCALL(glUniform1fv(loc, static_cast<GLsizei>(size), values));
  }

  void setVec2f(const char* uniformName, const float& x, const float& y) {
    const GLuint loc = getUniform(uniformName);
    if(loc != -1) GLCALL(glUniform2f(loc, x, y));
  }
  inline void setVec2f(const char* uniformName, const glm::vec2& value) { setVec2f(uniformName, value.x, value.y); }
  void setVec2f(const char* uniformName, const glm::vec2* values, const size_t& size) {
    const GLuint loc = getUniform(uniformName);
    if(loc != -1) GLCALL(glUniform2fv(loc, 2 * static_cast<GLsizei>(size), reinterpret_cast<const float*>(values)));
  }

  void setVec3f(const char* uniformName, const float& x, const float& y, const float& z) {
    const GLuint loc = getUniform(uniformName);
    if(loc != -1) GLCALL(glUniform3f(loc, x, y, z));
  }
  inline void setVec3f(const char* uniformName, const glm::vec3& value) { setVec3f(uniformName, value.x, value.y, value.z); }
  void setVec3f(const char* uniformName, const glm::vec3* values, const size_t& size) {
    const GLuint loc = getUniform(uniformName);
    if(loc != -1) GLCALL(glUniform3fv(loc, 3 * static_cast<GLsizei>(size), reinterpret_cast<const float*>(values)));
  }

  void setVec4f(const char* uniformName, const float& x, const float& y, const float& z, const float& w) {
    const GLuint loc = getUniform(uniformName);
    if(loc != -1) GLCALL(glUniform4f(loc, x, y, z, w));
  }
  inline void setVec4f(const char* uniformName, const glm::vec4& value) { setVec4f(uniformName, value.x, value.y, value.z, value.w); }
  void setVec4f(const char* uniformName, const glm::vec4* values, const size_t& size) {
    const GLuint loc = getUniform(uniformName);
    if(loc != -1) GLCALL(glUniform4fv(loc, 4 * static_cast<GLsizei>(size), reinterpret_cast<const float*>(values)));
  }

  void setMat3(const char* uniformName, const glm::mat3& m) {
    const GLuint loc = getUniform(uniformName);
    if(loc != -1) GLCALL(glUniformMatrix3fv(loc, 1, GL_FALSE, glm::value_ptr(m)));
  }
  void setMat4(const char* uniformName, const glm::mat4& m) {
    const GLuint loc = getUniform(uniformName);
    if(loc != -1) GLCALL(glUniformMatrix4fv(loc, 1, GL_FALSE, glm::value_ptr(m)));
  }

};

inline GLProgram buildProgram(std::initializer_list<GLShader> shaders)
{
  GLProgram program;
  for (const auto &shader : shaders) {
    program.attachShader(shader);
  }

  if (!program.link()) {
    std::cerr << program.getInfoLog() << std::endl;
    throw std::runtime_error(program.getInfoLog());
  }

  return program;
}

template <typename VSrc, typename FSrc>
GLProgram buildProgram(VSrc &&vsrc, FSrc &&fsrc)
{
  GLShader vs = compileShader(GL_VERTEX_SHADER, std::forward<VSrc>(vsrc));
  GLShader fs = compileShader(GL_FRAGMENT_SHADER, std::forward<FSrc>(fsrc));

  return buildProgram({std::move(vs), std::move(fs)});
}

template <typename VSrc, typename GSrc, typename FSrc>
GLProgram buildProgram(VSrc &&vsrc, GSrc &&gsrc, FSrc &&fsrc)
{
  GLShader vs = compileShader(GL_VERTEX_SHADER, std::forward<VSrc>(vsrc));
  GLShader gs = compileShader(GL_GEOMETRY_SHADER, std::forward<GSrc>(gsrc));
  GLShader fs = compileShader(GL_FRAGMENT_SHADER, std::forward<FSrc>(fsrc));

  return buildProgram({std::move(vs), std::move(gs), std::move(fs)});
}

template <typename CSrc>
GLProgram buildComputeProgram(CSrc &&src)
{
  GLShader cs = compileShader(GL_COMPUTE_SHADER, std::forward<CSrc>(src));
  return buildProgram({std::move(cs)});
  ;
}

inline GLProgram compileProgram(std::vector<fs::path> shaderPaths) {
  GLProgram program;
  for (const auto &path : shaderPaths) {
    auto shader = loadShader(path);
    program.attachShader(shader);
  }
  program.link();
  if (!program.getLinkStatus()) {
    std::cerr << "Program link error:" << program.getInfoLog() << std::endl;
    throw std::runtime_error("Program link error:" + program.getInfoLog());
  }
  return program;
}
