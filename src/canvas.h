#pragma once

#include <QQuickFramebufferObject>

struct CanvassyMessage;
class Subcanvassy;

class Canvassy : public QQuickFramebufferObject
{
    Q_OBJECT
    QML_NAMED_ELEMENT(Canvassy)

    Q_PROPERTY(Subcanvassy* subcanvassy READ subcanvassy WRITE setSubcanvassy NOTIFY subcanvassyChanged REQUIRED)

    struct Private;
    QScopedPointer<Private> d;

public:
    Canvassy(QQuickItem* parent = nullptr);
    ~Canvassy();
    Renderer* createRenderer() const override;
    QVarLengthArray<CanvassyMessage, 10>& messages() const;

    Subcanvassy* subcanvassy();
    void setSubcanvassy(Subcanvassy* subcanvas);
    Q_SIGNAL void subcanvassyChanged();

protected:
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
};
