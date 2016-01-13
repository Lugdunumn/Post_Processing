#include "gpuprogram.h"

//====================================================================================================================================
GPUProgram::GPUProgram()
    :   m_iGPUProgramID       ( 0 )
    ,   m_iGPUVertexShaderID  ( 0 )
    ,   m_iGPUFragmentShaderID( 0 )
    ,   m_iGPUGeometryShaderID( 0 )
{
}
//====================================================================================================================================
GPUProgram::~GPUProgram()
{
}
 //====================================================================================================================================
void GPUProgram::bind()
{
    glUseProgram( m_iGPUProgramID );
}
//====================================================================================================================================
void GPUProgram::unbind()
{

    glUseProgram( 0 );
}
//====================================================================================================================================
bool GPUProgram::createFromFiles( const std::string& _rstrVertexShaderPath, const std::string& _rstrGeometryShaderPath , const std::string& _rstrFragmentShaderPath )
{

    // required to have the OpenGL functions working - this is because of our use of OpenGL with Qt
    initializeOpenGLFunctions();


    const bool bUseGeometryShader = "" != _rstrGeometryShaderPath;

    // Creates the requested Shaders - or assert on error !
    m_iGPUVertexShaderID        = createShader( _rstrVertexShaderPath, GL_VERTEX_SHADER );
    m_iGPUFragmentShaderID      = createShader( _rstrFragmentShaderPath, GL_FRAGMENT_SHADER );



    if( bUseGeometryShader )
    {
        m_iGPUGeometryShaderID    = createShader( _rstrGeometryShaderPath, GL_GEOMETRY_SHADER );
    }

    // Creates the GPU Program
    m_iGPUProgramID = glCreateProgram();

    // Attaches a Vertex and a Fragmen Shader
    glAttachShader( m_iGPUProgramID, m_iGPUVertexShaderID );
    glAttachShader( m_iGPUProgramID, m_iGPUFragmentShaderID );

    if( bUseGeometryShader )
    {
        glAttachShader( m_iGPUProgramID, m_iGPUGeometryShaderID );
    }

    // Links the GPU Program to finally make it effective
    // NB : this will "copy" the attached Shaders into the Program, kinda like static C library linking
    glLinkProgram( m_iGPUProgramID );

    // Now you can detach the Shaders !
    // NB : You can even delete the Shaders
    //      if you don't want to use them for other GPU Programs
    //      => they are not needed, since linking "copied" them into the Program !
    // NB2: on some mobile device, the driver may not behave properly,
    // and won't like to have the Shaders to be detached !... But this is out of scope.
    glDetachShader( m_iGPUProgramID, m_iGPUVertexShaderID );
    glDetachShader( m_iGPUProgramID, m_iGPUFragmentShaderID );

    if( bUseGeometryShader )
    {
        glDetachShader( m_iGPUProgramID, m_iGPUGeometryShaderID );
    }

    destroyShader( m_iGPUVertexShaderID );
    destroyShader( m_iGPUFragmentShaderID );

    if( bUseGeometryShader )
    {
        destroyShader( m_iGPUGeometryShaderID );
    }

}
//====================================================================================================================================
void GPUProgram::destroy()
{
    glDeleteProgram( m_iGPUProgramID );
    m_iGPUProgramID = 0;
}
//====================================================================================================================================
GLuint GPUProgram::createShader( const std::string& _rstrShaderPath, GLenum _eShaderType )
{
    std::string strFragmentShader = readFileSrc( "data/" + _rstrShaderPath );
    const char* strSrc = strFragmentShader.c_str();

    GLuint iShader = glCreateShader( _eShaderType );
    glShaderSource( iShader, 1, & strSrc, 0 );
    glCompileShader( iShader );

    printShaderCompileInfo( iShader, "data/" + _rstrShaderPath );

    return iShader;
}
//====================================================================================================================================
void GPUProgram::destroyShader( GLuint& _rOutShaderID )
{

    glDeleteShader( _rOutShaderID );
    _rOutShaderID = 0;
}
//====================================================================================================================================
bool GPUProgram::printShaderCompileInfo( GLuint _iShaderID, const std::string& _strMsg )
{
    int iInfologLength = 0;
    int iCharsWritten  = 0;
    char* strInfoLog;

    glGetShaderiv( _iShaderID, GL_INFO_LOG_LENGTH, & iInfologLength );

    if( iInfologLength > 1 )
    {
        strInfoLog = new char[ iInfologLength + 1 ];
        glGetShaderInfoLog( _iShaderID, iInfologLength, & iCharsWritten, strInfoLog );

        std::cerr << "--------------"<< std::endl << "compilation of " << _strMsg <<  " : "<<std::endl << strInfoLog <<std::endl<< "--------------"<< std::endl;

        delete [] strInfoLog;
    }

    return ( 1 == iInfologLength );
}
