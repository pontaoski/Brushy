#pragma once

#include <QQuickFramebufferObject>

struct SubcanvassyMessage;

class Subcanvassy : public QQuickFramebufferObject
{
    Q_OBJECT
    QML_NAMED_ELEMENT(Subcanvassy)

    struct Private;
    QScopedPointer<Private> d;

public:
    Subcanvassy(QQuickItem* parent = nullptr);
    ~Subcanvassy();

    Renderer* createRenderer() const override;
    QVarLengthArray<SubcanvassyMessage, 10>& messages() const;
    void attach(Renderer* other);

protected:
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
};
