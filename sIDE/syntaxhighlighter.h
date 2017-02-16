#ifndef SYNTAXHIGHLIGHTER_H
#define SYNTAXHIGHLIGHTER_H

#include <QSyntaxHighlighter>
#include <QRegExp>
#include <QTextDocument>
#include <QTextCharFormat>

struct HighlightRule {
    HighlightRule() = default;
    HighlightRule(const QRegExp &pattern, const QTextCharFormat &format):
        pattern(pattern), format(format) {}

    QRegExp pattern;
    QTextCharFormat format;
};

class SyntaxHighlighter : public QSyntaxHighlighter
{
public:
    explicit SyntaxHighlighter(QTextDocument *parent);

protected:
    void highlightBlock(const QString &text) override;

private:
    QVector<HighlightRule> rules;
};

#endif // SYNTAXHIGHLIGHTER_H
