#include "syntaxhighlighter.h"

// Setup the rules table;
// We use the molokai-like theme.
SyntaxHighlighter::SyntaxHighlighter(QTextDocument *parent) : QSyntaxHighlighter(parent)
{
    QTextCharFormat constFormat;
    constFormat.setForeground(QColor("#5FD7FF"));
    rules.push_back(HighlightRule(QRegExp("\\b\\d+\\b", Qt::CaseInsensitive),
                    constFormat));

    QTextCharFormat labelFormat;
    labelFormat.setForeground(QColor("#87D700"));
    labelFormat.setFontWeight(QFont::Bold);
    rules.push_back(HighlightRule(QRegExp("^\\s*\\w+\\s*:"), labelFormat));

    QTextCharFormat keywordFormat;
    keywordFormat.setForeground(QColor("#5FFFFF"));
    keywordFormat.setFontWeight(QFont::Bold);

    QStringList keywords;
    keywords<< "\\bhlt\\b"<< "\\bjmp\\b"<< "\\bcjmp\\b"<< "\\bojmp\\b"<< "\\bcall\\b"
            << "\\bret\\b"<< "\\bpush\\b"<< "\\bpop\\b"<< "\\bloadb\\b"<< "\\bloadw\\b"
            << "\\bstoreb\\b"<< "\\bstorew\\b"<< "\\bloadi\\b"<< "\\bnop\\b"<< "\\bin\\b"
            << "\\bout\\b"<< "\\badd\\b"<< "\\baddi\\b"<< "\\bsub\\b"<< "\\bsubi\\b"
            << "\\bmul\\b"<< "\\bdiv\\b"<< "\\band\\b"<< "\\bor\\b"<< "\\bnor\\b"
            << "\\bnotb\\b"<< "\\bsal\\b"<< "\\bsar\\b"<< "\\bequ\\b"<< "\\blt\\b"
            << "\\blte\\b"<< "\\bnotc\\b";
    for (auto &word : keywords) {
        rules.push_back(HighlightRule(QRegExp(word, Qt::CaseInsensitive),
                                        keywordFormat));
    }

    QTextCharFormat typeFormat;
    typeFormat.setForeground(QColor("#5FD7FF"));
    typeFormat.setFontWeight(QFont::Bold);
    rules.push_back(HighlightRule(QRegExp("\\bbyte\\b", Qt::CaseInsensitive),
                    typeFormat));
    rules.push_back(HighlightRule(QRegExp("\\bword\\b", Qt::CaseInsensitive),
                    typeFormat));

    QTextCharFormat commentFormat;
    commentFormat.setForeground(QColor("#8B8878"));
    rules.push_back(HighlightRule(QRegExp("#.*"), commentFormat));
}

void SyntaxHighlighter::highlightBlock(const QString &text)
{
    for (const auto &rule : rules) {
        int index = rule.pattern.indexIn(text);
        while (index >= 0) {
            int length = rule.pattern.matchedLength();
            setFormat(index, length, rule.format);
            index = rule.pattern.indexIn(text, index + length);
        }
    }
}
