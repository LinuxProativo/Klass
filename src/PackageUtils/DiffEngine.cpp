/**
 * @file DiffEngine.cpp
 * @brief Implementation of the Myers diff algorithm with line hashing and boundary trimming.
 */

#include <QHashFunctions>
#include <QStringTokenizer>
#include <QVector>

#include <algorithm>

#include <DiffEngine.hpp>

namespace DiffEngine {
    /**
     * @brief Internal representation of a hashed text line for scalar-speed comparisons.
     */
    struct HashedLine {
        quint64 hash{0};
        QString text{};
    };

    /**
     * @brief Splits a text buffer into lines and pre-computes 64-bit hashes for every line.
     * @param text The input raw string content.
     * @return A list of HashedLine structures containing both hash and text reference.
     */
    static QVector<HashedLine> tokenizeAndHash(const QString &text) {
        QVector<HashedLine> result;
        for (const auto token: QStringTokenizer(text, u'\n')) {
            const QString lineStr = token.toString();
            const quint64 h = qHash(lineStr);
            result.append({h, lineStr});
        }
        return result;
    }

    /**
     * @brief Executes the core Myers greedy graph search on the reduced slice.
     * @param a Sliced old file lines.
     * @param b Sliced new file lines.
     * @param offsetOld Starting zero-based index of the slice within the original file.
     * @param offsetNew Starting zero-based index of the slice within the new file.
     * @return Ordered list of DiffLine operations resolving the difference between slices.
     */
    static QList<DiffLine> myersCore(const QVector<HashedLine> &a, const QVector<HashedLine> &b,
                                     const int offsetOld, const int offsetNew) {
        const int n = static_cast<int>(a.size());
        const int m = static_cast<int>(b.size());
        const int max = n + m;

        if (n == 0 && m == 0)
            return {};

        if (n == 0) {
            QList<DiffLine> res;
            res.reserve(m);
            for (int j = 0; j < m; ++j)
                res.append({LineType::Added, -1, offsetNew + j + 1, b[j].text});
            return res;
        }

        if (m == 0) {
            QList<DiffLine> res;
            res.reserve(n);
            for (int i = 0; i < n; ++i)
                res.append({LineType::Removed, offsetOld + i + 1, -1, a[i].text});
            return res;
        }

        const int vSize = 2 * max + 1;
        QVector<int> v(vSize, 0);
        QVector<QVector<int> > trace;

        const int kOffset = max;

        bool found = false;
        for (int d = 0; d <= max; ++d) {
            trace.append(v);

            for (int k = -d; k <= d; k += 2) {
                int x = 0;
                if (k == -d || (k != d && v[k - 1 + kOffset] < v[k + 1 + kOffset])) {
                    x = v[k + 1 + kOffset];
                } else {
                    x = v[k - 1 + kOffset] + 1;
                }

                int y = x - k;

                while (x < n && y < m && a[x].hash == b[y].hash && a[x].text == b[y].text) {
                    ++x;
                    ++y;
                }

                v[k + kOffset] = x;

                if (x >= n && y >= m) {
                    found = true;
                    break;
                }
            }

            if (found)
                break;
        }

        QList<DiffLine> script;
        int currentX = n;
        int currentY = m;

        for (int d = static_cast<int>(trace.size()) - 1; d >= 0; --d) {
            const QVector<int> &vPrev = trace[d];
            const int k = currentX - currentY;

            int prevK = 0;
            if (k == -d || (k != d && vPrev[k - 1 + kOffset] < vPrev[k + 1 + kOffset])) {
                prevK = k + 1;
            } else {
                prevK = k - 1;
            }

            const int prevX = vPrev[prevK + kOffset];
            const int prevY = prevX - prevK;

            while (currentX > prevX && currentY > prevY) {
                --currentX;
                --currentY;
                script.prepend(
                    {LineType::Context, offsetOld + currentX + 1, offsetNew + currentY + 1, a[currentX].text});
            }

            if (d > 0) {
                if (currentX == prevX) {
                    --currentY;
                    script.prepend({LineType::Added, -1, offsetNew + currentY + 1, b[currentY].text});
                } else if (currentY == prevY) {
                    --currentX;
                    script.prepend({LineType::Removed, offsetOld + currentX + 1, -1, a[currentX].text});
                }
            }
        }

        return script;
    }

    /**
     * @brief Computes line-by-line differences with prefix/suffix trimming and line hashing.
     * @param oldText Content of baseline file.
     * @param newText Content of updated .new file.
     * @return Complete ordered list of DiffLine records.
     */
    QList<DiffLine> computeDiff(const QString &oldText, const QString &newText) {
        const QVector<HashedLine> oldLines = tokenizeAndHash(oldText);
        const QVector<HashedLine> newLines = tokenizeAndHash(newText);

        const int oldSize = static_cast<int>(oldLines.size());
        const int newSize = static_cast<int>(newLines.size());

        int start = 0;
        while (start < oldSize && start < newSize &&
               oldLines[start].hash == newLines[start].hash &&
               oldLines[start].text == newLines[start].text) {
            ++start;
        }

        int oldEnd = oldSize - 1;
        int newEnd = newSize - 1;

        while (oldEnd >= start && newEnd >= start &&
               oldLines[oldEnd].hash == newLines[newEnd].hash &&
               oldLines[oldEnd].text == newLines[newEnd].text) {
            --oldEnd;
            --newEnd;
        }

        QList<DiffLine> result;
        result.reserve(oldSize + newSize);

        for (int i = 0; i < start; ++i)
            result.append({LineType::Context, i + 1, i + 1, oldLines[i].text});

        QVector<HashedLine> sliceA;
        QVector<HashedLine> sliceB;

        for (int i = start; i <= oldEnd; ++i)
            sliceA.append(oldLines[i]);

        for (int j = start; j <= newEnd; ++j)
            sliceB.append(newLines[j]);

        const QList<DiffLine> middleDiff = myersCore(sliceA, sliceB, start, start);
        result.append(middleDiff);

        const int suffixLen = oldSize - 1 - oldEnd;
        for (int i = 0; i < suffixLen; ++i) {
            const int oIdx = oldEnd + 1 + i;
            const int nIdx = newEnd + 1 + i;
            result.append({LineType::Context, oIdx + 1, nIdx + 1, oldLines[oIdx].text});
        }

        return result;
    }
}
