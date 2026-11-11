/**
 * @file DiffEngine.hpp
 * @brief High-performance Myers line diff algorithm with line hashing and prefix/suffix pruning.
 */

#ifndef DIFFENGINE_HPP
#define DIFFENGINE_HPP

#include <QList>
#include <QString>

/**
 * @namespace DiffEngine
 * @brief Provides data structures and algorithms for computing line-by-line text differences.
 */
namespace DiffEngine {
    /**
     * @enum LineType
     * @brief Indicates whether a line was added, removed, or retained as context.
     */
    enum class LineType {
        Context,
        Added,
        Removed
    };

    /**
     * @struct DiffLine
     * @brief Represents a single line in a calculated diff sequence.
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
