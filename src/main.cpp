#include "main.h"

RenderingWindow::RenderingWindow()
    :   m_GPUProgram                ( )
    ,   m_GPUProgramLight           ( )
    ,   m_GPUProgramBlur            ( )
    ,   m_GPUProgramBloomFinal      ( )

    ,   m_mtxObjectWorld            (0)
    ,   m_mtxCameraView             (0)
    ,   m_mtxCameraProjection       (0)
    ,   m_iUniformWorld             (0)
    ,   m_iUniformView              (0)
    ,   m_iUniformProjection        (0)

    ,   m_iUniformCameraPosition    (0)

    ,   m_iHDRFBO                   (0)
    ,   m_iDepthBufferFBO           (0)

    ,   m_iWoodTexture              (0)
    ,   m_iContainerTexture         (0)
    ,   m_iUniformWoodTextureUnit   (0)
    ,   m_iUniformContainerTextureUnit  (0)
    ,   m_iQuadVAO                  (0)
    ,   m_iQuadVBO                  (0)
    ,   m_iCubeVAO                  (0)
    ,   m_iCubeVBO                  (0)

    ,   m_iUniformSamplerScene      (0)
    ,   m_iUniformSamplerBloomBlur  (0)
{
    m_iColorBuffersFBO[2] = {0};
    m_iPingPongFBO[2] = {0};
    m_iPingPongColorBuffersFBO[2] = {0};
    m_vCameraPosition = glm::vec3(0,0,5);

    m_timer.start();

    SCR_WIDTH = 0;
    SCR_HEIGHT = 0;
    bloom = true; // Change with 'Space'
    exposure = 1.0f; // Change with Q and D

    setAnimating(true);
}
//====================================================================================================================================
RenderingWindow::~RenderingWindow()
{
    m_GPUProgram.destroy();
    m_GPUProgramLight.destroy();
    m_GPUProgramBlur.destroy();
    m_GPUProgramBloomFinal.destroy();

    destroyTextures();
    //destroyRenderTarget();
}
//====================================================================================================================================
void RenderingWindow::createRenderTarget()
{
    const float retinaScale = devicePixelRatio();
    SCR_WIDTH = width() * retinaScale;
    SCR_HEIGHT =  height() * retinaScale;
    // Light sources
    // Positions
    lightPositions.push_back(glm::vec3(0.0f, 0.5f, 1.5f)); // back light
    lightPositions.push_back(glm::vec3(-4.0f, 0.5f, -3.0f));
    lightPositions.push_back(glm::vec3(3.0f, 0.5f, 1.0f));
    lightPositions.push_back(glm::vec3(-.8f, 2.4f, -1.0f));
    // Colors
    lightColors.push_back(glm::vec3(5.0f, 5.0f, 5.0f));
    lightColors.push_back(glm::vec3(5.5f, 0.0f, 0.0f));
    lightColors.push_back(glm::vec3(0.0f, 0.0f, 15.0f));
    lightColors.push_back(glm::vec3(0.0f, 1.5f, 0.0f));

    // Set up floating point framebuffer to render scene to
    glGenFramebuffers(1, &m_iHDRFBO);
    glBindFramebuffer(GL_FRAMEBUFFER, m_iHDRFBO);

    // Create 2 floating point color buffers (1 for normal rendering, other for brightness treshold values)
        glGenTextures(2, m_iColorBuffersFBO);
        for (GLuint i = 0; i < 2; i++)
        {
            glBindTexture(GL_TEXTURE_2D,  m_iColorBuffersFBO[i]);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F, SCR_WIDTH, SCR_HEIGHT, 0, GL_RGB, GL_FLOAT, NULL);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);  // clamp to the edge as the blur filter would otherwise sample repeated texture values,
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);  // for example the light would pass the window edge to another side.
            // attach texture to framebuffer
            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + i, GL_TEXTURE_2D, m_iColorBuffersFBO[i], 0);
        }
        // Create and attach depth buffer (renderbuffer)
        glGenRenderbuffers(1, &m_iDepthBufferFBO);
        glBindRenderbuffer(GL_RENDERBUFFER, m_iDepthBufferFBO);
        glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, SCR_WIDTH, SCR_HEIGHT);
        glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, m_iDepthBufferFBO);
        // Tell OpenGL which color attachments will be used (of this framebuffer) for rendering
        GLuint attachments[2] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1 };
        glDrawBuffers(2, attachments);
        // - Finally check if framebuffer is complete
        if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
            std::cout << "Framebuffer not complete!" << std::endl;
        glBindFramebuffer(GL_FRAMEBUFFER, 0);

        // Ping pong framebuffer for blurring
        glGenFramebuffers(2, m_iPingPongFBO);
        glGenTextures(2, m_iPingPongColorBuffersFBO);
        for (GLuint i = 0; i < 2; i++)
        {
            glBindFramebuffer(GL_FRAMEBUFFER, m_iPingPongFBO[i]);
            glBindTexture(GL_TEXTURE_2D, m_iPingPongColorBuffersFBO[i]);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F, SCR_WIDTH, SCR_HEIGHT, 0, GL_RGB, GL_FLOAT, NULL);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE); // clamp to the edge
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_iPingPongColorBuffersFBO[i], 0);
            // check if framebuffers are complete
            if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
                std::cout << "Framebuffer not complete!" << std::endl;
        }

}
//====================================================================================================================================
GLuint RenderingWindow::loadTexture(const std::string &path)
{
    // pDataImage0 : POINTER on IMAGE RGB (3 bytes per pixel)
    std::string filename= "data/" + path;
    QImage img(filename.c_str());
    QImage img0 = img.convertToFormat(QImage::Format_RGB888, Qt::NoOpaqueDetection);

    unsigned char* pDataImage0 = img0.scanLine(0);
    GLuint textureID;
    glGenTextures(1, &textureID);
    glBindTexture(GL_TEXTURE_2D, textureID);
    {
        glTexImage2D(GL_TEXTURE_2D, 0, GL_SRGB, img0.width(), img0.height(), 0, GL_RGB, GL_UNSIGNED_BYTE, pDataImage0);
        glGenerateMipmap(GL_TEXTURE_2D);

        // Parameters
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    }
    glBindTexture(GL_TEXTURE_2D, 0);

    return textureID;
}
//====================================================================================================================================
void RenderingWindow::destroyTextures()
{
    glDeleteTextures(1, &m_iWoodTexture);
    glDeleteTextures(1, &m_iContainerTexture);
}
//====================================================================================================================================
void RenderingWindow::getUniformLocations()
{
    const GLuint iProgramBloomFinal = m_GPUProgramBloomFinal.getID();

    m_iUniformSamplerScene  = glGetUniformLocation(iProgramBloomFinal, "scene");
    m_iUniformSamplerBloomBlur = glGetUniformLocation(iProgramBloomFinal, "bloomBlur");

    m_iUniformWorld             = glGetUniformLocation( iProgramBloomFinal, "u_mtxWorld" );
    m_iUniformView              = glGetUniformLocation( iProgramBloomFinal, "u_mtxView" );
    m_iUniformProjection        = glGetUniformLocation( iProgramBloomFinal, "u_mtxProjection" );
}
//====================================================================================================================================
void RenderingWindow::initialize()
{
    const float retinaScale = devicePixelRatio();
    glViewport( 0, 0, width() * retinaScale, height() * retinaScale );

    glEnable(GL_DEPTH_TEST);
    m_GPUProgram.createFromFiles("VS_bloom.glsl", "", "FS_bloom.glsl");
    m_GPUProgramLight.createFromFiles("VS_bloom.glsl", "", "FS_light_box.glsl");
    m_GPUProgramBlur.createFromFiles("VS_blur.glsl", "", "FS_blur.glsl");
    m_GPUProgramBloomFinal.createFromFiles("VS_bloom_final.glsl", "", "FS_bloom_final.glsl");

    glUseProgram(m_GPUProgramBloomFinal.getID());
        glUniform1i(glGetUniformLocation(m_GPUProgramBloomFinal.getID(), "scene"), 0);
        glUniform1i(glGetUniformLocation(m_GPUProgramBloomFinal.getID(), "bloomBlur"), 1);
    //getUniformLocations();

    m_iWoodTexture = loadTexture("texture/wood.png");
    m_iContainerTexture = loadTexture("texture/container2.png");

    createRenderTarget();

    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
}
//====================================================================================================================================
void RenderingWindow::bindRenderTarget()
{
    const float retinaScale = devicePixelRatio();
    glViewport( 0, 0, width() * retinaScale, height() * retinaScale );
}

//====================================================================================================================================
void RenderingWindow::unbindRenderTarget()
{
    glBindFramebuffer( GL_FRAMEBUFFER, 0 );
    const float retinaScale = devicePixelRatio();
    glViewport( 0, 0, width() * retinaScale, height() * retinaScale );
}
//====================================================================================================================================
void RenderingWindow::updateMatrices()
{
    const glm::vec3 vRight  (1,0,0);
    const glm::vec3 vUp     (0,1,0);
    const glm::vec3 vFront  (0,0,-1);
    const glm::vec3 vCenter (0,0,-1);



}
//====================================================================================================================================
void RenderingWindow::update()
{
    float   fTimeElapsed        = (float) m_timer.elapsed();

    updateMatrices();
}
//====================================================================================================================================
void RenderingWindow::render()
{
    // Specifies the viewport size
    const float retinaScale = devicePixelRatio();
    glViewport( 0, 0, width() * retinaScale, height() * retinaScale );

//    glEnable( GL_CULL_FACE );
//    glCullFace( GL_BACK );

    // Render scene into floating point framebuffer
    glBindFramebuffer(GL_FRAMEBUFFER, m_iHDRFBO);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    m_mtxCameraProjection = glm::perspective(45.0f, (GLfloat)SCR_WIDTH / (GLfloat)SCR_HEIGHT, 0.1f, 100.0f);
    m_mtxCameraView = glm::lookAt(m_vCameraPosition, m_vCameraPosition + glm::vec3(0.0f, 0.0f, -1.0f), glm::vec3(0.0f, 1.0f, 0.0f));;

    glUseProgram(m_GPUProgram.getID());
    glUniformMatrix4fv(glGetUniformLocation(m_GPUProgram.getID(), "projection"), 1, GL_FALSE, glm::value_ptr(m_mtxCameraProjection));
    glUniformMatrix4fv(glGetUniformLocation(m_GPUProgram.getID(), "view"), 1, GL_FALSE, glm::value_ptr(m_mtxCameraView));
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, m_iWoodTexture);

        // set lighting uniforms
        for (GLuint i = 0; i < lightPositions.size(); i++)
        {
            glUniform3fv(glGetUniformLocation(m_GPUProgram.getID(), ("lights[" + std::to_string(i) + "].Position").c_str()), 1, &lightPositions[i][0]);
            glUniform3fv(glGetUniformLocation(m_GPUProgram.getID(), ("lights[" + std::to_string(i) + "].Color").c_str()), 1, &lightColors[i][0]);
        }
        glUniform3fv(glGetUniformLocation(m_GPUProgram.getID(), "viewPos"), 1, &m_vCameraPosition[0]);

        // create one large cube that acts as the floor
        m_mtxObjectWorld = glm::mat4();
        m_mtxObjectWorld = glm::translate(m_mtxObjectWorld, glm::vec3(0.0f, -1.0f, 0.0));
        m_mtxObjectWorld = glm::scale(m_mtxObjectWorld, glm::vec3(25.0f, 1.0f, 25.0f));
        glUniformMatrix4fv(glGetUniformLocation(m_GPUProgram.getID(), "model"), 1, GL_FALSE, glm::value_ptr(m_mtxObjectWorld));
        RenderCube();
        // create multiple cubes
        glBindTexture(GL_TEXTURE_2D, m_iContainerTexture);
        m_mtxObjectWorld = glm::mat4();
        m_mtxObjectWorld = glm::translate(m_mtxObjectWorld, glm::vec3(0.0f, 1.5f, 0.0));
        glUniformMatrix4fv(glGetUniformLocation(m_GPUProgram.getID(), "model"), 1, GL_FALSE, glm::value_ptr(m_mtxObjectWorld));
        RenderCube();
        m_mtxObjectWorld = glm::mat4();
        m_mtxObjectWorld = glm::translate(m_mtxObjectWorld, glm::vec3(2.0f, 0.0f, 1.0));
        glUniformMatrix4fv(glGetUniformLocation(m_GPUProgram.getID(), "model"), 1, GL_FALSE, glm::value_ptr(m_mtxObjectWorld));
        RenderCube();
        m_mtxObjectWorld = glm::mat4();
        m_mtxObjectWorld = glm::translate(m_mtxObjectWorld, glm::vec3(-1.0f, -1.0f, 2.0));
        m_mtxObjectWorld = glm::rotate(m_mtxObjectWorld, 60.0f, glm::normalize(glm::vec3(1.0, 0.0, 1.0)));
        m_mtxObjectWorld = glm::scale(m_mtxObjectWorld, glm::vec3(2.0));
        glUniformMatrix4fv(glGetUniformLocation(m_GPUProgram.getID(), "model"), 1, GL_FALSE, glm::value_ptr(m_mtxObjectWorld));
        RenderCube();
        m_mtxObjectWorld = glm::mat4();
        m_mtxObjectWorld = glm::translate(m_mtxObjectWorld, glm::vec3(0.0f, 2.7f, 4.0));
        m_mtxObjectWorld = glm::rotate(m_mtxObjectWorld, 23.0f, glm::normalize(glm::vec3(1.0, 0.0, 1.0)));
        m_mtxObjectWorld = glm::scale(m_mtxObjectWorld, glm::vec3(2.5));
        glUniformMatrix4fv(glGetUniformLocation(m_GPUProgram.getID(), "model"), 1, GL_FALSE, glm::value_ptr(m_mtxObjectWorld));
        RenderCube();
        m_mtxObjectWorld = glm::mat4();
        m_mtxObjectWorld = glm::translate(m_mtxObjectWorld, glm::vec3(-2.0f, 1.0f, -3.0));
        m_mtxObjectWorld = glm::rotate(m_mtxObjectWorld, 124.0f, glm::normalize(glm::vec3(1.0, 0.0, 1.0)));
        m_mtxObjectWorld = glm::scale(m_mtxObjectWorld, glm::vec3(2.0));
        glUniformMatrix4fv(glGetUniformLocation(m_GPUProgram.getID(), "model"), 1, GL_FALSE, glm::value_ptr(m_mtxObjectWorld));
        RenderCube();
        RenderCube();
        m_mtxObjectWorld = glm::mat4();
        m_mtxObjectWorld = glm::translate(m_mtxObjectWorld, glm::vec3(-3.0f, 0.0f, 0.0));
        glUniformMatrix4fv(glGetUniformLocation(m_GPUProgram.getID(), "model"), 1, GL_FALSE, glm::value_ptr(m_mtxObjectWorld));
        RenderCube();
        // show all the light sources as bright cubes
        glUseProgram(m_GPUProgramLight.getID());
        glUniformMatrix4fv(glGetUniformLocation(m_GPUProgramLight.getID(), "projection"), 1, GL_FALSE, glm::value_ptr(m_mtxCameraProjection));
        glUniformMatrix4fv(glGetUniformLocation(m_GPUProgramLight.getID(), "view"), 1, GL_FALSE, glm::value_ptr(m_mtxCameraView));

        for (GLuint i = 0; i < lightPositions.size(); i++)
        {
            m_mtxObjectWorld = glm::mat4();
            m_mtxObjectWorld = glm::translate(m_mtxObjectWorld, glm::vec3(lightPositions[i]));
            m_mtxObjectWorld = glm::scale(m_mtxObjectWorld, glm::vec3(0.5f));
            glUniformMatrix4fv(glGetUniformLocation(m_GPUProgramLight.getID(), "model"), 1, GL_FALSE, glm::value_ptr(m_mtxObjectWorld));
            glUniform3fv(glGetUniformLocation(m_GPUProgramLight.getID(), "lightColor"), 1, &lightColors[i][0]);
            RenderCube();
        }
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    // Blur bright fragments with two-pass Gaussian Blur
    GLboolean horizontal = true, first_iteration = true;
    GLuint amount = 10;
    m_GPUProgramBlur.bind();
    for (GLuint i = 0; i < amount; i++)
    {
        glBindFramebuffer(GL_FRAMEBUFFER, m_iPingPongFBO[horizontal]);
        glUniform1i(glGetUniformLocation(m_GPUProgramBlur.getID(), "horizontal"), horizontal);
        glBindTexture(GL_TEXTURE_2D, first_iteration ? m_iColorBuffersFBO[1] : m_iPingPongColorBuffersFBO[!horizontal]);  // bind texture of other framebuffer (or scene if first iteration)
        RenderQuad();
        horizontal = !horizontal;
        if (first_iteration)
            first_iteration = false;
    }
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    // render floating point color buffer to 2D quad and tonemap HDR colors to default framebuffer's (clamped) color range
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    m_GPUProgramBloomFinal.bind();

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, m_iColorBuffersFBO[0]);

    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, m_iPingPongColorBuffersFBO[!horizontal]);


    glUniform1i(glGetUniformLocation(m_GPUProgramBloomFinal.getID(), "bloom"), bloom);
    glUniform1f(glGetUniformLocation(m_GPUProgramBloomFinal.getID(), "exposure"), exposure);
    RenderQuad();

}
//====================================================================================================================================
void RenderingWindow::RenderQuad()
{
    if (m_iQuadVAO == 0)
    {
        GLfloat quadVertices[] = {
            // Positions        // Texture Coords
            -1.0f, 1.0f, 0.0f, 0.0f, 1.0f,
            -1.0f, -1.0f, 0.0f, 0.0f, 0.0f,
            1.0f, 1.0f, 0.0f, 1.0f, 1.0f,
            1.0f, -1.0f, 0.0f, 1.0f, 0.0f,
        };
        // Setup plane VAO
        glGenVertexArrays(1, &m_iQuadVAO);
        glGenBuffers(1, &m_iQuadVBO);
        glBindVertexArray(m_iQuadVAO);
        glBindBuffer(GL_ARRAY_BUFFER, m_iQuadVBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), &quadVertices, GL_STATIC_DRAW);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(GLfloat), (GLvoid*)0);
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(GLfloat), (GLvoid*)(3 * sizeof(GLfloat)));
    }
    glBindVertexArray(m_iQuadVAO);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    glBindVertexArray(0);
}
//====================================================================================================================================
void RenderingWindow::RenderCube()
{
    if (m_iCubeVAO == 0)
    {
        GLfloat vertices[] = {
            // Back face
            -0.5f, -0.5f, -0.5f, 0.0f, 0.0f, -1.0f, 0.0f, 0.0f, // Bottom-left
            0.5f, 0.5f, -0.5f, 0.0f, 0.0f, -1.0f, 1.0f, 1.0f, // top-right
            0.5f, -0.5f, -0.5f, 0.0f, 0.0f, -1.0f, 1.0f, 0.0f, // bottom-right
            0.5f, 0.5f, -0.5f, 0.0f, 0.0f, -1.0f, 1.0f, 1.0f,  // top-right
            -0.5f, -0.5f, -0.5f, 0.0f, 0.0f, -1.0f, 0.0f, 0.0f,  // bottom-left
            -0.5f, 0.5f, -0.5f, 0.0f, 0.0f, -1.0f, 0.0f, 1.0f,// top-left
            // Front face
            -0.5f, -0.5f, 0.5f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, // bottom-left
            0.5f, -0.5f, 0.5f, 0.0f, 0.0f, 1.0f, 1.0f, 0.0f,  // bottom-right
            0.5f, 0.5f, 0.5f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f,  // top-right
            0.5f, 0.5f, 0.5f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f, // top-right
            -0.5f, 0.5f, 0.5f, 0.0f, 0.0f, 1.0f, 0.0f, 1.0f,  // top-left
            -0.5f, -0.5f, 0.5f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f,  // bottom-left
            // Left face
            -0.5f, 0.5f, 0.5f, -1.0f, 0.0f, 0.0f, 1.0f, 0.0f, // top-right
            -0.5f, 0.5f, -0.5f, -1.0f, 0.0f, 0.0f, 1.0f, 1.0f, // top-left
            -0.5f, -0.5f, -0.5f, -1.0f, 0.0f, 0.0f, 0.0f, 1.0f,  // bottom-left
            -0.5f, -0.5f, -0.5f, -1.0f, 0.0f, 0.0f, 0.0f, 1.0f, // bottom-left
            -0.5f, -0.5f, 0.5f, -1.0f, 0.0f, 0.0f, 0.0f, 0.0f,  // bottom-right
            -0.5f, 0.5f, 0.5f, -1.0f, 0.0f, 0.0f, 1.0f, 0.0f, // top-right
            // Right face
            0.5f, 0.5f, 0.5f, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f, // top-left
            0.5f, -0.5f, -0.5f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f, // bottom-right
            0.5f, 0.5f, -0.5f, 1.0f, 0.0f, 0.0f, 1.0f, 1.0f, // top-right
            0.5f, -0.5f, -0.5f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f,  // bottom-right
            0.5f, 0.5f, 0.5f, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f,  // top-left
            0.5f, -0.5f, 0.5f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, // bottom-left
            // Bottom face
            -0.5f, -0.5f, -0.5f, 0.0f, -1.0f, 0.0f, 0.0f, 1.0f, // top-right
            0.5f, -0.5f, -0.5f, 0.0f, -1.0f, 0.0f, 1.0f, 1.0f, // top-left
            0.5f, -0.5f, 0.5f, 0.0f, -1.0f, 0.0f, 1.0f, 0.0f,// bottom-left
            0.5f, -0.5f, 0.5f, 0.0f, -1.0f, 0.0f, 1.0f, 0.0f, // bottom-left
            -0.5f, -0.5f, 0.5f, 0.0f, -1.0f, 0.0f, 0.0f, 0.0f, // bottom-right
            -0.5f, -0.5f, -0.5f, 0.0f, -1.0f, 0.0f, 0.0f, 1.0f, // top-right
            // Top face
            -0.5f, 0.5f, -0.5f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f,// top-left
            0.5f, 0.5f, 0.5f, 0.0f, 1.0f, 0.0f, 1.0f, 0.0f, // bottom-right
            0.5f, 0.5f, -0.5f, 0.0f, 1.0f, 0.0f, 1.0f, 1.0f, // top-right
            0.5f, 0.5f, 0.5f, 0.0f, 1.0f, 0.0f, 1.0f, 0.0f, // bottom-right
            -0.5f, 0.5f, -0.5f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f,// top-left
            -0.5f, 0.5f, 0.5f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f // bottom-left
        };
        glGenVertexArrays(1, &m_iCubeVAO);
        glGenBuffers(1, &m_iCubeVBO);
        // Fill buffer
        glBindBuffer(GL_ARRAY_BUFFER, m_iCubeVBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
        // Link vertex attributes
        glBindVertexArray(m_iCubeVAO);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(GLfloat), (GLvoid*)0);
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(GLfloat), (GLvoid*)(3 * sizeof(GLfloat)));
        glEnableVertexAttribArray(2);
        glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(GLfloat), (GLvoid*)(6 * sizeof(GLfloat)));
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindVertexArray(0);
    }
    // Render Cube
    glBindVertexArray(m_iCubeVAO);
    glDrawArrays(GL_TRIANGLES, 0, 36);
    glBindVertexArray(0);
}
//====================================================================================================================================
void RenderingWindow::resizeEvent(QResizeEvent *)
{
    updateMatrices();
}

//====================================================================================================================================
void RenderingWindow::keyPressEvent(QKeyEvent* _pEvent)
{
    if(QEvent::KeyPress == _pEvent->type())
    {
        switch(_pEvent->key())
        {
        case Qt::Key_Q:
            exposure -= 0.2;
            break;

        case Qt::Key_D:
            exposure += 0.2;
            break;
        case Qt::Key_Space:
            if (bloom == true)
                bloom = false;
            else
                bloom = true;
            break;
        case Qt::Key_F:
            writeFBOToFile(m_iHDRFBO, "HDRFBO.jpg");
            writeFBOToFile(m_iDepthBufferFBO, "DepthBufferFBO.jpg");
            writeFBOToFile(m_iColorBuffersFBO[0], "ColorBufferFBO1.jpg");
            writeFBOToFile(m_iColorBuffersFBO[1], "ColorBufferFBO2.jpg");
            writeFBOToFile(m_iPingPongFBO[0], "PingPongFBO1.jpg");
            writeFBOToFile(m_iPingPongFBO[1], "PingPongFBO2.jpg");
            writeFBOToFile(m_iPingPongColorBuffersFBO[0], "PingPongColorBuffersFBO1.jpg");
            writeFBOToFile(m_iPingPongColorBuffersFBO[1], "PingPongColorBuffersFBO2.jpg");
            break;
        }
    }
}
//====================================================================================================================================
void RenderingWindow::writeFBOToFile( GLuint iFBO, const std::string& _rstrFilePath )
{
    const float retinaScale = devicePixelRatio();
    GLsizei W = width() * retinaScale;
    GLsizei H = height() * retinaScale;

    // this is called Pixel Transfer operation: http://www.opengl.org/wiki/Pixel_Transfer
    uchar* aPixels = new uchar[ W * H * 4 ];
    memset( aPixels, 0, W * H * 4 );

    glBindFramebuffer( GL_FRAMEBUFFER, iFBO );
    glReadPixels( 0,0,  W, H, GL_RGBA, GL_UNSIGNED_BYTE, aPixels );

    glBindFramebuffer( GL_FRAMEBUFFER,  0 );

    QImage qImage = QImage( aPixels, W, H, QImage::Format_ARGB32 );
    // flip the image vertically, cause OpenGL has them upside down!
    qImage = qImage.mirrored(false, true);
    qImage = qImage.rgbSwapped();

    qImage.save( _rstrFilePath.c_str() );
}

//====================================================================================================================================
int main(int argc, char *argv[])
{
    QGuiApplication app(argc, argv);

    QSurfaceFormat format;
    format.setSamples(16);

    RenderingWindow w;
    w.setFormat(format);
    w.resize(1024,768);
    w.show();

    return app.exec();
}
