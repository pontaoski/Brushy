#define protected public
#include <QQuickFramebufferObject>
#include <QOpenGLFunctions>
#include <QOpenGLShaderProgram>
#include <QOpenGLFramebufferObjectFormat>
#include <QQuickWindow>
#undef protected
#include "subcanvas.h"
#include "canvas.h"

struct SubcanvassyMessage {
    enum Type {
        Down,
        Move,
        Up,
        OtherRenderer,
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
        struct {
            QQuickFramebufferObject::Renderer* ptr;
        } other;
    };
    SubcanvassyMessage() { }
    static SubcanvassyMessage CDown(const QPointF& pos) {
        SubcanvassyMessage ret; { ret.tag = Down; ret.down = {pos}; }; return ret;
    }
    static SubcanvassyMessage CMove(const QPointF& pos) {
        SubcanvassyMessage ret; { ret.tag = Move; ret.move = {pos}; }; return ret;
    }
    static SubcanvassyMessage CUp() {
        SubcanvassyMessage ret; { ret.tag = Up; ret.up = {}; }; return ret;
    }
    static SubcanvassyMessage COtherRenderer(QQuickFramebufferObject::Renderer* other) {
        SubcanvassyMessage ret; { ret.tag = OtherRenderer; ret.other = {other}; }; return ret;
    }
};

class SubcanvassyRenderer : public QQuickFramebufferObject::Renderer
{
    // opengl + inputs to opengl
    QOpenGLShaderProgram program;
    int vertexLocation;
    int matrixLocation;
    int colorLocation;
    int viewportSizeLocation;
    int centerLocation;
    int strokeSizeLocation;
    int inputTextureLocation;
    int directionLocation;

    // inputs from item
    QSize m_size;
    float m_dpr;

    // messages
    QPointF m_pos;
    QPointF m_lastPos;
    QVarLengthArray<SubcanvassyMessage, 10> m_messages;
    bool m_initted = false;
    Renderer* m_other = nullptr;
    QOpenGLFramebufferObject* m_pass1 = nullptr;
    /// also the output buffer
    QOpenGLFramebufferObject* m_pass2 = nullptr;
public:
    SubcanvassyRenderer(Subcanvassy*) {
        const char* vsrc =
            R"(
            #version 330
            attribute highp vec4 vertex;
            uniform highp mat4 matrix;
            out mediump vec2 TextureCoordinates;
            void main()
            {
                gl_Position = matrix * vertex;
                TextureCoordinates = 0.5 * gl_Position.xy + vec2(0.5);
            }
            )";
        const char* fsrc =
            R"(
            #version 330
            uniform mediump vec4 color;
            uniform mediump vec2 viewportSize;
            uniform mediump vec2 center;
            uniform mediump float strokeSize;
            uniform sampler2D inputTexture;
            uniform mediump vec2 blurDirection;

            in mediump vec2 TextureCoordinates;

            const int M = 16;
            const int N = 2 * M + 1;

            const float coeffs[N] = float[N](
                0.012318109844189502,
                0.014381474814203989,
                0.016623532195728208,
                0.019024086115486723,
                0.02155484948872149,
                0.02417948052890078,
                0.02685404941667096,
                0.0295279624870386,
                0.03214534135442581,
                0.03464682117793548,
                0.0369716985390341,
                0.039060328279673276,
                0.040856643282313365,
                0.04231065439216247,
                0.043380781642569775,
                0.044035873841196206,
                0.04425662519949865,
                0.044035873841196206,
                0.043380781642569775,
                0.04231065439216247,
                0.040856643282313365,
                0.039060328279673276,
                0.0369716985390341,
                0.03464682117793548,
                0.03214534135442581,
                0.0295279624870386,
                0.02685404941667096,
                0.02417948052890078,
                0.02155484948872149,
                0.019024086115486723,
                0.016623532195728208,
                0.014381474814203989,
                0.012318109844189502
            );

            void main() {
                vec2 cx = (center);
                vec2 px = (gl_FragCoord.xy);
                float dist = distance(cx, px);
                if (dist >= strokeSize) {
                    gl_FragColor.a = 0.0;
                    gl_FragDepth = 0.0;
                } else {
                    vec4 sum = vec4(0.0);
                    for (int i = 0; i < N; ++i) {
                        vec2 tc = TextureCoordinates + blurDirection * float(i - M);
                        sum += coeffs[i] * texture(inputTexture, tc);
                    }
                    gl_FragColor = sum;
                    gl_FragDepth = 1.0;
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
        inputTextureLocation = program.uniformLocation("inputTexture");
        directionLocation = program.uniformLocation("blurDirection");
    }
    ~SubcanvassyRenderer() { }

    void paint(QPointF p) {
        QOpenGLFunctions fns;
        fns.initializeOpenGLFunctions();

        QMatrix4x4 pmvMatrix;
        pmvMatrix.ortho(0, m_size.width(), 0, m_size.height(), -1, 1);

        auto x = static_cast<GLfloat>(p.x());
        auto y = static_cast<GLfloat>(p.y());
        const float pointSize = 50.0f;

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

        fns.glEnable(GL_BLEND);
        fns.glEnable(GL_DEPTH_TEST);
        fns.glDepthFunc(GL_GREATER);
        fns.glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

        program.enableAttributeArray(vertexLocation);
        program.setAttributeArray(vertexLocation, squareVertices, 3);
        program.setUniformValue(matrixLocation, pmvMatrix);
        program.setUniformValue(colorLocation, QColor(0, 255, 0, 255));
        program.setUniformValue(viewportSizeLocation, m_size);
        program.setUniformValue(centerLocation, p * m_dpr);
        program.setUniformValue(strokeSizeLocation, pointSize * m_dpr);

        fns.glActiveTexture(GL_TEXTURE0);
        program.setUniformValue(inputTextureLocation, 0);

        program.setUniformValue(directionLocation, QVector2D(1.0f / m_size.width(), 0.0f));
        m_pass1->bind();
        fns.glBindTexture(GL_TEXTURE_2D, m_other->framebufferObject()->texture());
        fns.glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_BYTE, squareIndices);

        program.setUniformValue(directionLocation, QVector2D(0.0f, 1.0f / m_size.height()));
        m_pass2->bind();
        fns.glBindTexture(GL_TEXTURE_2D, m_pass1->texture());
        fns.glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_BYTE, squareIndices);

        program.disableAttributeArray(vertexLocation);
    }

    void render() override {
        QOpenGLFunctions fns;
        fns.initializeOpenGLFunctions();
        if (!m_initted) {
            fns.glClearColor(0.0, 0.0, 0.0, 0.00);
            fns.glClearDepthf(0.0);
            fns.glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
            m_initted = true;
        }

        for (auto& msg : m_messages) {
            switch (msg.tag) {
            case SubcanvassyMessage::Down: {
                m_lastPos = msg.down.pos;
                m_pos = msg.down.pos;
                program.bind();
                paint(m_pos);
                program.release();
                break;
            }
            case SubcanvassyMessage::Move: {
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
            case SubcanvassyMessage::Up: {
                m_lastPos = QPointF();
                m_pos = QPointF();
                fns.glClearColor(0.0, 0.0, 0.0, 0.00);
                fns.glClearDepthf(0.0);
                fns.glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
                break;
            }
            case SubcanvassyMessage::OtherRenderer: {
                m_other = msg.other.ptr;
            }
            }
        }
    }

    QOpenGLFramebufferObject *createFramebufferObject(const QSize &size) override {
        if (m_pass1 != nullptr) {
            delete m_pass1;
            m_pass1 = nullptr;
            m_pass2 = nullptr;
        }
        QOpenGLFramebufferObjectFormat format;
        format.setAttachment(QOpenGLFramebufferObject::CombinedDepthStencil);
        format.setSamples(4);
        auto pass1 = new QOpenGLFramebufferObject(size, format);
        auto pass2 = new QOpenGLFramebufferObject(size, format);
        m_pass1 = pass1;
        m_pass2 = pass2;
        return m_pass2;
    }

    void synchronize(QQuickFramebufferObject* item) override {
        auto canvas = static_cast<Subcanvassy*>(item);
        m_messages = std::move(canvas->messages());
        canvas->messages().clear();
        m_size = item->size().toSize();
        m_dpr = item->window()->effectiveDevicePixelRatio();
    }
};

struct Subcanvassy::Private
{
    QVarLengthArray<SubcanvassyMessage, 10> messages;
};

Subcanvassy::Subcanvassy(QQuickItem* parent) : QQuickFramebufferObject(parent), d(new Private)
{
    setAcceptedMouseButtons(Qt::RightButton);
}

Subcanvassy::~Subcanvassy()
{
}

Subcanvassy::Renderer* Subcanvassy::createRenderer() const
{
    return new SubcanvassyRenderer(const_cast<Subcanvassy*>(this));
}
QVarLengthArray<SubcanvassyMessage, 10>& Subcanvassy::messages() const
{
    return d->messages;
}
void Subcanvassy::attach(Renderer* other)
{
    d->messages << SubcanvassyMessage::COtherRenderer(other);
    update();
}
void Subcanvassy::mousePressEvent(QMouseEvent* event)
{
    d->messages << SubcanvassyMessage::CDown(event->pos());
    update();
}
void Subcanvassy::mouseMoveEvent(QMouseEvent* event)
{
    d->messages << SubcanvassyMessage::CMove(event->pos());
    update();
}
void Subcanvassy::mouseReleaseEvent(QMouseEvent*) {
    d->messages << SubcanvassyMessage::CUp();
    update();
}
