#include <QOpenGLFramebufferObjectFormat>
#include <QOpenGLFunctions>
#include <QOpenGLShaderProgram>
#include <QQuickWindow>
#include "canvas.h"

struct CanvassyMessage {
    enum Type {
        Down,
        Move,
        Up,
    };
    Type tag;
    union {
        struct {
            QPointF pos;
        } down;
        struct {
            QPointF pos;
        } move;
        struct {
        } up;
    };
    CanvassyMessage() { }
    static CanvassyMessage CDown(const QPointF& pos) {
        CanvassyMessage ret; { ret.tag = Down; ret.down = {pos}; }; return ret;
    }
    static CanvassyMessage CMove(const QPointF& pos) {
        CanvassyMessage ret; { ret.tag = Move; ret.move = {pos}; }; return ret;
    }
    static CanvassyMessage CUp() {
        CanvassyMessage ret; { ret.tag = Up; ret.up = {}; }; return ret;
    }
};

class CanvassyRenderer : public QQuickFramebufferObject::Renderer
{
    // opengl + inputs to opengl
    QOpenGLShaderProgram program;
    int vertexLocation;
    int matrixLocation;
    int colorLocation;
    int viewportSizeLocation;
    int centerLocation;
    int strokeSizeLocation;

    // inputs from item
    QPointF m_pos;
    QPointF m_lastPos;
    QSize m_size;
    float m_dpr;

    // messages
    QVarLengthArray<CanvassyMessage, 10> m_messages;
    bool m_initted = false;
public:
    CanvassyRenderer(Canvassy*) {
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
            uniform mediump vec2 viewportSize;
            uniform mediump vec2 center;
            uniform mediump float strokeSize;
            void main() {
                vec2 cx = (center);
                vec2 px = (gl_FragCoord.xy);
                float dist = distance(cx, px);
                // gl_FragColor = color;
                gl_FragColor.r = 1.0;
                gl_FragColor.g = 0.0;
                gl_FragColor.b = 0.0;
                if (dist < strokeSize) {
                    gl_FragColor.a = 1.0;
                } else {
                    gl_FragColor.a = 0.0;
                }
            }
            )";
        program.addCacheableShaderFromSourceCode(QOpenGLShader::Vertex, vsrc);
        program.addCacheableShaderFromSourceCode(QOpenGLShader::Fragment, fsrc);
        program.link();

        vertexLocation = program.attributeLocation("vertex");
        matrixLocation = program.uniformLocation("matrix");
        colorLocation = program.uniformLocation("color");
        viewportSizeLocation = program.uniformLocation("viewportSize");
        centerLocation = program.uniformLocation("center");
        strokeSizeLocation = program.uniformLocation("strokeSize");
    }
    ~CanvassyRenderer() { }

    void paint(QPointF p) {
        QOpenGLFunctions fns;
        fns.initializeOpenGLFunctions();

        QMatrix4x4 pmvMatrix;
        pmvMatrix.ortho(0, m_size.width(), 0, m_size.height(), -1, 1);

        auto x = static_cast<GLfloat>(p.x());
        auto y = static_cast<GLfloat>(p.y());
        const auto pointSize = 10.0f;

        const GLfloat squareVertices[] = {
            x - pointSize, y - pointSize, 0.0f,
            x - pointSize, y + pointSize, 0.0f,
            x + pointSize, y + pointSize, 0.0f,
            x + pointSize, y - pointSize, 0.0f
        };
        const GLubyte squareIndices[] = {
            0, 1, 2,
            0, 2, 3
        };

        program.enableAttributeArray(vertexLocation);
        program.setAttributeArray(vertexLocation, squareVertices, 3);
        program.setUniformValue(matrixLocation, pmvMatrix);
        program.setUniformValue(colorLocation, QColor(0, 255, 0, 255));
        program.setUniformValue(viewportSizeLocation, m_size);
        program.setUniformValue(centerLocation, p * m_dpr);
        program.setUniformValue(strokeSizeLocation, pointSize * m_dpr);

        fns.glEnable(GL_BLEND);
        fns.glEnable(GL_DEPTH_TEST);
        fns.glDepthFunc(GL_ALWAYS);
        fns.glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        fns.glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_BYTE, squareIndices);
        program.disableAttributeArray(vertexLocation);
    }

    void render() override {
        QOpenGLFunctions fns;
        fns.initializeOpenGLFunctions();
        if (!m_initted) {
            fns.glClearColor(0.3, 0.3, 0.3, 1.0);
            fns.glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
            m_initted = true;
        }

        for (auto& msg : m_messages) {
            switch (msg.tag) {
            case CanvassyMessage::Down: {
                m_lastPos = msg.down.pos;
                m_pos = msg.down.pos;
                program.bind();
                paint(m_pos);
                program.release();
                break;
            }
            case CanvassyMessage::Move: {
                m_lastPos = m_pos;
                m_pos = msg.move.pos;

                auto line = QLineF(m_pos, m_lastPos);
                program.bind();
                if (line.length() > 1.0) {
                    int n = line.length();
                    for (int i = 0; i < n; i++) {
                        auto t = qreal(i) / qreal(n);
                        paint(line.pointAt(t));
                    }
                } else {
                    paint(m_pos);
                }
                program.release();
                break;
            }
            case CanvassyMessage::Up: {
                m_lastPos = QPointF();
                m_pos = QPointF();
                break;
            }
            }
        }

    }

    QOpenGLFramebufferObject *createFramebufferObject(const QSize &size) override {
        QOpenGLFramebufferObjectFormat format;
        format.setAttachment(QOpenGLFramebufferObject::CombinedDepthStencil);
        format.setSamples(4);
        return new QOpenGLFramebufferObject(size, format);
    }

    void synchronize(QQuickFramebufferObject* item) override {
        auto canvas = static_cast<Canvassy*>(item);
        m_messages = std::move(canvas->messages());
        canvas->messages().clear();
        m_size = item->size().toSize();
        m_dpr = item->window()->effectiveDevicePixelRatio();
    }
};

struct Canvassy::Private
{
    QPoint pos;
    QVarLengthArray<CanvassyMessage, 10> messages;
};

Canvassy::Canvassy(QQuickItem* parent) : QQuickFramebufferObject(parent), d(new Private)
{
    setAcceptedMouseButtons(Qt::LeftButton);
}
Canvassy::~Canvassy()
{

}
QQuickFramebufferObject::Renderer* Canvassy::createRenderer() const
{
    return new CanvassyRenderer(const_cast<Canvassy*>(this));
}
void Canvassy::mousePressEvent(QMouseEvent* event)
{
    d->messages << CanvassyMessage::CDown(event->pos());
    update();
}
void Canvassy::mouseMoveEvent(QMouseEvent* event)
{
    d->messages << CanvassyMessage::CMove(event->pos());
    update();
}
void Canvassy::mouseReleaseEvent(QMouseEvent*) {
    d->messages << CanvassyMessage::CUp();
    update();
}
QVarLengthArray<CanvassyMessage, 10>& Canvassy::messages() const
{
    return d->messages;
}
