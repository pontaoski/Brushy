#include <QOpenGLFramebufferObjectFormat>
#include <QOpenGLFunctions>
#include <QOpenGLShaderProgram>
#include "canvas.h"

class CanvassyRenderer : public QQuickFramebufferObject::Renderer
{
    QPoint pos;
    QSize size;
    QOpenGLShaderProgram program;
    int vertexLocation;
    int matrixLocation;
    int colorLocation;
public:
    CanvassyRenderer() {
        const char* vsrc =
            R"(
            attribute highp vec4 vertex;
            uniform highp mat4 matrix;
            void main()
            {
                gl_Position = matrix * vertex;
            }
            )";
        const char* fsrc =
            R"(
            uniform mediump vec4 color;
            void main() {
                gl_FragColor = color;
            }
            )";
        program.addCacheableShaderFromSourceCode(QOpenGLShader::Vertex, vsrc);
        program.addCacheableShaderFromSourceCode(QOpenGLShader::Fragment, fsrc);
        program.link();

        vertexLocation = program.attributeLocation("vertex");
        matrixLocation = program.uniformLocation("matrix");
        colorLocation = program.uniformLocation("color");
    }
    ~CanvassyRenderer() { }

    void paint() {
        QOpenGLFunctions fns;
        fns.initializeOpenGLFunctions();

        QMatrix4x4 pmvMatrix;
        pmvMatrix.ortho(0, size.width(), 0, size.height(), -1, 1);

        auto x = static_cast<GLfloat>(pos.x());
        auto y = static_cast<GLfloat>(pos.y());

        const GLfloat squareVertices[] = {
            x - 10.0f, y - 10.0f, 0.0f,
            x - 10.0f, y + 10.0f, 0.0f,
            x + 10.0f, y + 10.0f, 0.0f,
            x + 10.0f, y - 10.0f, 0.0f
        };
        const GLubyte squareIndices[] = {
            0, 1, 2,
            0, 2, 3
        };

        program.enableAttributeArray(vertexLocation);
        program.setAttributeArray(vertexLocation, squareVertices, 3);
        program.setUniformValue(matrixLocation, pmvMatrix);
        program.setUniformValue(colorLocation, QColor(0, 255, 0, 255));

        fns.glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_BYTE, squareIndices);
        program.disableAttributeArray(vertexLocation);
    }

    void render() override {
        QOpenGLFunctions fns;
        fns.initializeOpenGLFunctions();
        if (pos.isNull()) {
            fns.glClearColor(1.0, 1.0, 0.0, 1.0);
            fns.glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        } else {
            program.bind();
            paint();
            program.release();
        }
        update();
    }

    QOpenGLFramebufferObject *createFramebufferObject(const QSize &size) override {
        QOpenGLFramebufferObjectFormat format;
        format.setAttachment(QOpenGLFramebufferObject::CombinedDepthStencil);
        format.setSamples(4);
        return new QOpenGLFramebufferObject(size, format);
    }

    void synchronize(QQuickFramebufferObject* item) override {
        auto canvas = static_cast<Canvassy*>(item);
        pos = canvas->pos;
        size = canvas->size().toSize();
    }
};

Canvassy::Canvassy(QQuickItem* parent) : QQuickFramebufferObject(parent)
{
    setAcceptedMouseButtons(Qt::LeftButton);
}
Canvassy::~Canvassy()
{

}
QQuickFramebufferObject::Renderer* Canvassy::createRenderer() const
{
    return new CanvassyRenderer();
}
void Canvassy::mousePressEvent(QMouseEvent* event)
{
    pos = event->pos();
    update();
}
void Canvassy::mouseMoveEvent(QMouseEvent* event)
{
    pos = event->pos();
    update();
}
