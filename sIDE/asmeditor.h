#ifndef ASMEDITOR_H
#define ASMEDITOR_H

#include <QPlainTextEdit>
#include <QWidget>
#include <QObject>

class LineNumberWidget;

class AsmEditor : public QPlainTextEdit
{
    Q_OBJECT

public:
    AsmEditor(QWidget *parent = 0);

    void lineNumberWidgetPaintEvent(QPaintEvent *event);
    int lineNumberWidgetWidth();

protected:
    void resizeEvent(QResizeEvent *event) override;

private slots:
    void updateLineNumberWidgetWidth(int newBlockCount);
    void highlightCurrentLine();
    void updateLineNumberWidget(const QRect &, int);

private:
    LineNumberWidget *lineNumber;
};

// Display line number.
class LineNumberWidget : public QWidget
{
public:
    LineNumberWidget(AsmEditor *editor) :
        QWidget(editor), editor(editor){}

    QSize sizeHint() const override {
        return QSize(editor->lineNumberWidgetWidth(), 0);
    }

protected:
    void paintEvent(QPaintEvent *event) override {
        editor->lineNumberWidgetPaintEvent(event);
    }

private:
    AsmEditor *editor;
};

#endif // ASMEDITOR_H
