/**
 * @file DiffEngine.cpp
 * @brief Implementation of internal Longest Common Subsequence (LCS) diff calculation.
 */

#include <QVector>
#include <QStringTokenizer>
#include <algorithm>

#include <DiffEngine.hpp>

namespace DiffEngine {
    /**
     * @brief Computes line-by-line differences using the Longest Common Subsequence (LCS) algorithm.
     * @param oldText Content of the existing configuration file.
     * @param newText Content of the .new configuration file.
     * @return Ordered list of DiffLine records.
     */
    QList<DiffLine> computeDiff(const QString &oldText, const QString &newText) {
        QStringList oldLines;
        for (const auto line: QStringTokenizer(oldText, u'\n'))
            oldLines.append(line.toString());

        QStringList newLines;
        for (const auto line: QStringTokenizer(newText, u'\n'))
            newLines.append(line.toString());

        const int n = static_cast<int>(oldLines.size());
        const int m = static_cast<int>(newLines.size());

        QVector<int> dp((n + 1) * (m + 1), 0);
        auto at = [&](const int i, const int j) -> int & {
            return dp[i * (m + 1) + j];
        };

        for (int i = 0; i < n; ++i) {
            for (int j = 0; j < m; ++j) {
                if (oldLines[i] == newLines[j]) {
                    at(i + 1, j + 1) = at(i, j) + 1;
                } else {
                    at(i + 1, j + 1) = std::max(at(i + 1, j), at(i, j + 1));
                }
            }
        }

        QList<DiffLine> result;
        int i = n;
        int j = m;

        while (i > 0 || j > 0) {
            if (i > 0 && j > 0 && oldLines[i - 1] == newLines[j - 1]) {
                result.prepend({LineType::Context, i, j, oldLines[i - 1]});
                --i;
                --j;
            } else if (j > 0 && (i == 0 || at(i, j - 1) >= at(i - 1, j))) {
                result.prepend({LineType::Added, -1, j, newLines[j - 1]});
                --j;
            } else if (i > 0 && (j == 0 || at(i, j - 1) < at(i - 1, j))) {
                result.prepend({LineType::Removed, i, -1, oldLines[i - 1]});
                --i;
            }
        }

        return result;
    }
}
