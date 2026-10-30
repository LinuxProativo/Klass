/**
 * @file Mirrors.hpp
 * @brief Header for manages mirrors.
 */

#ifndef MIRRORS_HPP
#define MIRRORS_HPP

#include <QList>

/**
 * @class Mirrors
 * @brief Manages repository mirror lists and parses configuration files.
 */
class Mirrors {
public:
    /**
     * @struct OfficialBranches
     * @brief Represents the activation status of secondary official Slackware branches.
     */
    struct OfficialBranches {
        bool pasture{false};
        bool testing{false};
        bool extra{false};
    };

    /**
     * @enum Channel
     * @brief Represents the release channel of the distribution.
     */
    enum class Channel {
        Stable,
        Current,
        Unknown
    };

    /**
     * @struct MirrorEntry
     * @brief Represents a package repository mirror server.
     */
    struct MirrorEntry {
        QString url{};
        QString country{};
    };

    /**
     * @struct MirrorPlusEntry
     * @brief Represents a third-party or supplementary repository mirror.
     */
    struct MirrorPlusEntry {
        QString name{};
        QString url{};
    };

    static QList<MirrorEntry> listAll();

    static MirrorEntry activeOne();

    static bool setActive(const QString &url);

    static QList<MirrorPlusEntry> catalogue();

    static QList<MirrorPlusEntry> listActive();

    static bool enable(const MirrorPlusEntry &entry);

    static bool disable(const QString &name);

    static bool isUpdateRequired();

    static OfficialBranches loadOfficialBranches();

    static bool setOfficialBranches(const OfficialBranches &branches);

private:
    static QString detectArch();

    static Channel detectChannel();

    static bool parseMirrorLine(const QString &raw, QString &outUrl);

    static bool accept(const QString &url);

    static QString extractTimestamp(const QString &text);
};

#endif
