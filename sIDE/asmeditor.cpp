#include <QTextBlock>
#include <QPainter>
#include <QPlainTextEdit>
#include "asmeditor.h"

AsmEditor::AsmEditor(QWidget *parent): QPlainTextEdit(parent)
{
    lineNumber = new LineNumberWidget(this);

    connect(this, SIGNAL(blockCountChanged(int)), this, SLOT(updateLineNumberWidgetWidth(int)));
    connect(this, SIGNAL(updateRequest(QRect,int)), this, SLOT(updateLineNumberWidget(QRect,int)));
    connect(this, SIGNAL(cursorPositionChanged()), this, SLOT(highlightCurrentLine()));

    updateLineNumberWidgetWidth(0);
    highlightCurrentLine();
}


int AsmEditor::lineNumberWidgetWidth()
{
    int digits = 1;
    int max = qMax(1, blockCount());
    while (max >= 10) {
        max /= 10;
        ++digits;
    }

    int space = 5 + fontMetrics().width(QLatin1Char('9'))*digits;
    return space;
}

void AsmEditor::updateLineNumberWidgetWidth(int /*newBlockCount*/)
{
    setViewportMargins(lineNumberWidgetWidth(), 0, 0, 0);
}

void AsmEditor::updateLineNumberWidget(const QRect &rect, int dy)
{
    if (dy)
        lineNumber->scroll(0, dy);
    else
        lineNumber->update(0, rect.y(), lineNumber->width(), rect.height());

    if (rect.contains(viewport()->rect()))
        updateLineNumberWidgetWidth(0);
}

void AsmEditor::resizeEvent(QResizeEvent *e)
{
    QPlainTextEdit::resizeEvent(e);

    QRect cr = contentsRect();
    lineNumber->setGeometry(QRect(cr.left(), cr.top(), lineNumberWidgetWidth(), cr.height()));
}

void AsmEditor::highlightCurrentLine()
{
    QList<QTextEdit::ExtraSelection> extraSelections;

    QTextEdit::ExtraSelection selection;

    QColor lineColor = QColor("#3C3D37");

    selection.format.setBackground(lineColor);
    selection.format.setProperty(QTextFormat::FullWidthSelection, true);
    selection.cursor = textCursor();
    selection.cursor.clearSelection();
    extraSelections.append(selection);

    setExtraSelections(extraSelections);
}

void AsmEditor::lineNumberWidgetPaintEvent(QPaintEvent *event)
{
    QPainter painter(lineNumber);
    painter.fillRect(event->rect(), QColor("#272822"));

    QTextBlock block = firstVisibleBlock();
    int blockNumber = block.blockNumber();
    int top = (int) blockBoundingGeometry(block).translated(contentOffset()).top();
    int bottom = top + (int) blockBoundingRect(block).height();

    while (block.isValid() && top <= event->rect().bottom()) {
        if (block.isVisible() && bottom >= event->rect().top()) {
            QString number = QString::number(blockNumber + 1);
            painter.setPen(QColor("#8F908A"));
            painter.drawText(0, top, lineNumber->width(), fontMetrics().height(),
                             Qt::AlignRight, number);
        }

        block = block.next();
        top = bottom;
        bottom = top + (int) blockBoundingRect(block).height();
        ++blockNumber;
    }
}



