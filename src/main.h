#ifndef MAIN_H
#define MAIN_H

#include <iostream>
#include <string>

#include "openglwindow.h"
#include <QtGui/QGuiApplication>
#include <QtGui/QScreen>
#include <QtGui/QKeyEvent>
#include <QTime>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include "gpuprogram.h"


class RenderingWindow : public OpenGLWindow
{
public :
    RenderingWindow();
    virtual ~RenderingWindow();

    void initialize();

    void render();

    void update();

private:

    void keyPressEvent(QKeyEvent *);

    void resizeEvent(QResizeEvent *);

    void getUniformLocations();

    void sendUniformsToGPU();

    void updateMatrices();

    GLuint loadTexture(const std::string& path);

    void destroyTextures();

    void createRenderTarget();

    void destroyRenderTarget();

    void bindRenderTarget();

    void unbindRenderTarget();

    void writeFBOToFile( GLuint iFBO, const std::string& _rstrFilePath );


    GPUProgram      m_GPUProgram;
    GPUProgram      m_GPUProgramLight;
    GPUProgram      m_GPUProgramBlur;
    GPUProgram      m_GPUProgramBloomFinal;

    glm::mat4       m_mtxObjectWorld;
    glm::mat4       m_mtxCameraView;
    glm::mat4       m_mtxCameraProjection;

    GLuint          m_iUniformWorld;
    GLuint          m_iUniformView;
    GLuint          m_iUniformProjection;

    QTime           m_timer;

    glm::vec3       m_vCameraPosition;
    GLuint          m_iUniformCameraPosition;

    GLuint          m_iWoodTexture;
    GLuint          m_iContainerTexture;

    GLuint          m_iUniformWoodTextureUnit;
    GLuint          m_iUniformContainerTextureUnit;

    GLuint          m_iHDRFBO;
    GLuint          m_iColorBuffersFBO[2];

    GLuint          m_iDepthBufferFBO;


    GLuint          m_iPingPongFBO[2];
    GLuint          m_iPingPongColorBuffersFBO[2];

    GLuint          m_iQuadVAO;
    GLuint          m_iQuadVBO;

    GLuint          m_iCubeVAO;
    GLuint          m_iCubeVBO;

    GLuint          m_iUniformSamplerScene;
    GLuint          m_iUniformSamplerBloomBlur;

    std::vector<glm::vec3> lightPositions;
    std::vector<glm::vec3> lightColors;

    GLsizei         SCR_WIDTH;
    GLsizei         SCR_HEIGHT;

    void            RenderCube();
    void            RenderQuad();

    GLboolean       bloom; // Change with 'Space'
    GLfloat         exposure; // Change with Q and D
};



#endif // MAIN_H
