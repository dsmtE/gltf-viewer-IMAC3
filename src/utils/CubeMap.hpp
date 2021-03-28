#pragma once

// #include "TextureFB.hpp"
#include "Texture.hpp"

#include "stb_image.h"

#include <array>

class CubeMap {

public:

    inline GLuint id() const { return textureId_; }

	CubeMap() {
		GLCALL(glGenTextures(1, &textureId_));
		GLCALL(glBindTexture(GL_TEXTURE_CUBE_MAP, textureId_));

        GLCALL(glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR));
        GLCALL(glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR));
        GLCALL(glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE));
        GLCALL(glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE));
        GLCALL(glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE));

		GLCALL(glBindTexture(GL_TEXTURE_CUBE_MAP, 0));

        initCubeVao();
    }

	~CubeMap() {
        GLCALL(glDeleteTextures(1, &textureId_));
        GLCALL(glDeleteBuffers(1, &cubeVbo_));
        GLCALL(glDeleteBuffers(1, &cubeEbo_));
        GLCALL(glDeleteVertexArrays(1, &cubeVao_));
    }

	CubeMap(const Texture&) = delete;
    CubeMap& operator=(const CubeMap&) = delete;

	CubeMap(CubeMap&& o) {
		textureId_ = o.textureId_;
		o.textureId_ = -1;
	}

    inline void bindTexture(const int slot) const {
        GLCALL(glActiveTexture(GL_TEXTURE0 + slot));
		GLCALL(glBindTexture(GL_TEXTURE_CUBE_MAP, textureId_));
    }

    void draw() const {
        GLCALL(glBindVertexArray(cubeVao_));
        GLCALL(glDrawElements(GL_TRIANGLES,36, GL_UNSIGNED_SHORT, 0));
        GLCALL(glBindVertexArray(0));
    }

	void upload(const std::array<std::string, 6>& facesPaths) {
		GLCALL(glBindTexture(GL_TEXTURE_CUBE_MAP, id()));

        int width, height, nrChannels;
        for (unsigned int i = 0; i < 6; ++i) {
            unsigned char *data = stbi_load(facesPaths[i].c_str(), &width, &height, &nrChannels, 0);
            if (data) {
                GLCALL(glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data));
                stbi_image_free(data);
            } else {
                std::cout << "Cubemap tex failed to load at path: " << facesPaths[i] << std::endl;
                stbi_image_free(data);
            }
        }

        // glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        // glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        // glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        // glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        // glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

        GLCALL(glBindTexture(GL_TEXTURE_CUBE_MAP, 0));
    }

    void uploadFromHDREquirectangular(const std::filesystem::path shadersRootPath, const std::string& path) {
        stbi_set_flip_vertically_on_load(true);
        int width, height, nrChannels;
        float *data = stbi_loadf(path.c_str(), &width, &height, &nrChannels, 0);

        Texture hdrTex({width, height}, GL_RGB16F);

        if (!data) {
            std::cerr << "Failed to load HDR image." << std::endl;
            return;
        }

        hdrTex.upload(GL_RGB, GL_FLOAT, data);
        hdrTex.setup();
        stbi_image_free(data);
        stbi_set_flip_vertically_on_load(false);

        GLProgram equirectangularToCubemapShader = compileProgram({shadersRootPath / "cubemap.vs.glsl", shadersRootPath / "equirectangularToCubemap.fs.glsl"});

        unsigned int captureFBO, captureRBO;
        glGenFramebuffers(1, &captureFBO);
        glGenRenderbuffers(1, &captureRBO);

        glBindFramebuffer(GL_FRAMEBUFFER, captureFBO);
        glBindRenderbuffer(GL_RENDERBUFFER, captureRBO);
        glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, 512, 512);
        glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, captureRBO);

        GLCALL(glBindTexture(GL_TEXTURE_CUBE_MAP, id()));
        for (unsigned int i = 0; i < 6; ++i) {
            // note that we store each face with 16 bit floating point values
            glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGB16F,  512, 512, 0, GL_RGB, GL_FLOAT, nullptr);
        }
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

        const glm::mat4 captureProjection = glm::perspective(glm::radians(90.0f), 1.0f, 0.1f, 10.0f);
        const glm::vec3 zero = glm::vec3(0.0f, 0.0f, 0.0f);
        const glm::mat4 captureViews[] = {
            glm::lookAt(zero, glm::vec3( 1.0f,  0.0f,  0.0f), glm::vec3(0.0f, -1.0f,  0.0f)),
            glm::lookAt(zero, glm::vec3(-1.0f,  0.0f,  0.0f), glm::vec3(0.0f, -1.0f,  0.0f)),
            glm::lookAt(zero, glm::vec3( 0.0f,  1.0f,  0.0f), glm::vec3(0.0f,  0.0f,  1.0f)),
            glm::lookAt(zero, glm::vec3( 0.0f, -1.0f,  0.0f), glm::vec3(0.0f,  0.0f, -1.0f)),
            glm::lookAt(zero, glm::vec3( 0.0f,  0.0f,  1.0f), glm::vec3(0.0f, -1.0f,  0.0f)),
            glm::lookAt(zero, glm::vec3( 0.0f,  0.0f, -1.0f), glm::vec3(0.0f, -1.0f,  0.0f))
        };

        equirectangularToCubemapShader.use();

        equirectangularToCubemapShader.setMat4("ProjMatrix", captureProjection);
        hdrTex.attachToShaderSlot(equirectangularToCubemapShader, "hdrEquirectangular", 0);

        int saveViewportDim_[4];
        GLCALL(glGetIntegerv(GL_VIEWPORT, saveViewportDim_)); // Store viewport settings to restore them when unbinding
        glViewport(0, 0, 512, 512); // configure the viewport to the capture dimensions

        glBindFramebuffer(GL_FRAMEBUFFER, captureFBO);
        for (unsigned int i = 0; i < 6; ++i) {
            equirectangularToCubemapShader.setMat4("ViewMatrix", captureViews[i]);
            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, id(), 0);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
            draw(); // renders a cube
        }
        glBindRenderbuffer(GL_RENDERBUFFER, 0);
        glBindFramebuffer(GL_FRAMEBUFFER, 0);  

        GLCALL(glBindTexture(GL_TEXTURE_CUBE_MAP, 0));

        // then before rendering, configure the viewport to the original framebuffer's screen dimensions
        GLCALL(glViewport(saveViewportDim_[0], saveViewportDim_[1], saveViewportDim_[2], saveViewportDim_[3]));
    }

private:

    void initCubeVao() {
        GLCALL(glGenVertexArrays(1, &cubeVao_));
        GLCALL(glGenBuffers(1, &cubeVbo_));
        GLCALL(glGenBuffers(1, &cubeEbo_));

        GLCALL(glBindVertexArray(cubeVao_));

        GLCALL(glBindBuffer(GL_ARRAY_BUFFER, cubeVbo_));
        GLCALL(glBufferData(GL_ARRAY_BUFFER, sizeof(cubeVertices), cubeVertices, GL_STATIC_DRAW));

        GLCALL(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, cubeEbo_));
        GLCALL(glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(cubeIndices), cubeIndices, GL_STATIC_DRAW));

        // 4. then set the vertex attributes pointers
        GLCALL(glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0));
        GLCALL(glEnableVertexAttribArray(0));

        GLCALL(glBindVertexArray(0));
        GLCALL(glDisableVertexAttribArray(0));
        GLCALL(glBindBuffer(GL_ARRAY_BUFFER, 0));
        GLCALL(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0));
    }

	GLuint textureId_ = -1;

    GLuint cubeVao_ = -1;
    GLuint cubeVbo_ = -1;
    GLuint cubeEbo_ = -1;

    static constexpr const GLfloat cubeVertices[] = {
        // front
        -1.0, -1.0,  1.0,
        1.0, -1.0,  1.0,
        1.0,  1.0,  1.0,
        -1.0,  1.0,  1.0,
        // back
        -1.0, -1.0, -1.0,
        1.0, -1.0, -1.0,
        1.0,  1.0, -1.0,
        -1.0,  1.0, -1.0,
    };
    
    static constexpr const GLushort cubeIndices[] = {
        // front
        0, 1, 2,
        2, 3, 0,
        // top
        3, 2, 6,
        6, 7, 3,
        // back
        7, 6, 5,
        5, 4, 7,
        // bottom
        4, 5, 1,
        1, 0, 4,
        // left
        4, 0, 3,
        3, 7, 4,
        // right
        1, 5, 6,
        6, 2, 1,
    };
};