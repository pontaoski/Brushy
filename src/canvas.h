#pragma once

#include <QQuickFramebufferObject>

struct CanvassyMessage;

class Canvassy : public QQuickFramebufferObject
{
    Q_OBJECT
    QML_NAMED_ELEMENT(Canvassy)

    struct Private;
    QScopedPointer<Private> d;

public:
    Canvassy(QQuickItem* parent = nullptr);
    ~Canvassy();
    Renderer* createRenderer() const override;
    QVarLengthArray<CanvassyMessage, 10>& messages() const;

protected:
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
};
