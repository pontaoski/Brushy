#pragma once

#include <QQuickFramebufferObject>

class Canvassy : public QQuickFramebufferObject
{
    Q_OBJECT
    QML_NAMED_ELEMENT(Canvassy)
    friend class CanvassyRenderer;

    QPoint pos;

public:
    Canvassy(QQuickItem* parent = nullptr);
    ~Canvassy();
    Renderer* createRenderer() const override;

protected:
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
};
