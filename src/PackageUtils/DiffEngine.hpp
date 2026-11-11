/**
 * @file DiffEngine.hpp
 * @brief Internal diff algorithm for comparing files line by line without external processes.
 */

#ifndef DIFFENGINE_HPP
#define DIFFENGINE_HPP

#include <QList>
#include <QString>

namespace DiffEngine {
    /**
     * @enum LineType
     * @brief Indicates whether a line was added, removed, or kept as context.
     */
    enum class LineType {
        Context,
        Added,
        Removed
    };

    /**
     * @struct DiffLine
     * @brief Represents a single line in a diff sequence.
     */
    struct DiffLine {
        LineType type{LineType::Context};
        int oldLineNumber{-1};
        int newLineNumber{-1};
        QString text{};
    };

    QList<DiffLine> computeDiff(const QString &oldText, const QString &newText);
}

#endif
