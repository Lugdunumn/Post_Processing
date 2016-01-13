#include "openglwindow.h"

#include <QtCore/QCoreApplication>

#include <QtGui/QOpenGLContext>
#include <QtGui/QOpenGLPaintDevice>
#include <QtGui/QPainter>


//====================================================================================================================================
OpenGLWindow::OpenGLWindow( QWindow* _pParent )
    : QWindow           ( _pParent )
    , m_bUpdatePending  ( false )
    , m_bAnimating       ( false )
    , m_pContext         ( 0 )
    , m_pDevice          ( 0 )
{
    setSurfaceType(QWindow::OpenGLSurface);
}

//====================================================================================================================================
OpenGLWindow::~OpenGLWindow()
{
    delete m_pDevice;
}
//====================================================================================================================================
void OpenGLWindow::setAnimating( bool _bAnimating )
{
    m_bAnimating = _bAnimating;

    if( _bAnimating )
        renderLater();
}
//====================================================================================================================================
void OpenGLWindow::render( QPainter* _pPainter )
{
    Q_UNUSED( _pPainter );
}

//====================================================================================================================================
void OpenGLWindow::initialize()
{
}

//====================================================================================================================================
void OpenGLWindow::render()
{
    if ( ! m_pDevice )
        m_pDevice = new QOpenGLPaintDevice;

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

    m_pDevice->setSize(size());

    QPainter painter( m_pDevice );
    render(&painter);
}

//====================================================================================================================================
void OpenGLWindow::renderLater()
{
    if ( ! m_bUpdatePending )
    {
        m_bUpdatePending = true;
        QCoreApplication::postEvent(this, new QEvent(QEvent::UpdateRequest));
    }
}

//====================================================================================================================================
bool OpenGLWindow::event( QEvent* _pEvent )
{
    switch( _pEvent->type() )
    {
    case QEvent::UpdateRequest:
        m_bUpdatePending = false;
        renderNow();
        return true;

    default:
        return QWindow::event( _pEvent );
    }
}

//====================================================================================================================================
void OpenGLWindow::exposeEvent( QExposeEvent* _pEvent )
{
    Q_UNUSED( _pEvent );

    if( isExposed() )
        renderNow();
}

//====================================================================================================================================
void OpenGLWindow::renderNow()
{
    if( ! isExposed() )
        return;

    bool bNeedsInitialize = false;

    if( ! isInitialized() )
    {
        m_pContext = new QOpenGLContext(this);
        m_pContext->setFormat(requestedFormat());
        m_pContext->create();

        bNeedsInitialize = true;
    }

    m_pContext->makeCurrent(this);

    if( bNeedsInitialize )
    {
        initializeOpenGLFunctions();
        initialize();
    }


    update();
    render();

    m_pContext->swapBuffers(this);


    if( m_bAnimating )
    {
        renderLater();
    }
}

