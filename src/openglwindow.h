#ifndef __OPENGLWINDOW_H__
#define __OPENGLWINDOW_H__

#include <QtGui/QWindow>
#include <QOpenGLFunctions_3_3_Core>

QT_BEGIN_NAMESPACE
class QPainter;
class QOpenGLContext;
class QOpenGLPaintDevice;
QT_END_NAMESPACE


class OpenGLWindow : public QWindow, protected QOpenGLFunctions_3_3_Core
{
    Q_OBJECT
public:
    explicit OpenGLWindow( QWindow* _pParent = 0);
    ~OpenGLWindow();

    virtual void render( QPainter* _pPainter );
    virtual void render();
    virtual void update() = 0;

    virtual void initialize();

    void setAnimating( bool _bAnimating );

//    QOpenGLContext* getGLContext() { return m_context;}

public slots:
    void renderLater();
    void renderNow();

protected:

    inline bool isInitialized() const;

    bool event( QEvent* _pEvent ) Q_DECL_OVERRIDE;

    void exposeEvent( QExposeEvent* _pEvent ) Q_DECL_OVERRIDE;

private:
    bool                    m_bUpdatePending;
    bool                    m_bAnimating;

    QOpenGLContext*         m_pContext;
    QOpenGLPaintDevice*     m_pDevice;
};

//====================================================================================================================================
bool OpenGLWindow::isInitialized() const
{
    return 0 != m_pContext;
}

#endif //__OPENGLWINDOW_H__
