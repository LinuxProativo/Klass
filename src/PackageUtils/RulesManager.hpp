/**
 * @file RulesManager.hpp
 * @brief Header for RulesManager.
 */

#ifndef RULESMANAGER_HPP
#define RULESMANAGER_HPP

#include <QHash>
#include <QList>
#include <QStringList>
#include <QRegularExpression>

#include <Packages.hpp>

/**
 * @struct RuleEntry
 * @brief Represents a single exception or priority rule entry.
 */
struct RuleEntry {
    QString type{};
    QString repo{};
    QString scope{};
    QString rule{};

    /**
     * @brief Converts entry to formatted configuration line.
     */
    [[nodiscard]] QString toConfigLine() const {
        return QString("RULES['%1']=(%2,%3,%4)").arg(type, repo, scope, rule);
    }
};

/**
 * @enum RuleSt
 * @brief The resolved outcome of applying exception/priority rules to one specific package instance.
 */
enum class RuleSt {
    Normal,
    Excluded,
    Prioritized
};

/**
 * @struct PkgKey
 * @brief Memory-efficient composite key used for hash map lookups.
 * Eliminates heavy string concatenations.
 */
struct PkgKey {
    QString name;
    QString version;
    QString repo;

    bool operator==(const PkgKey &other) const {
        return name == other.name && version == other.version && repo == other.repo;
    }
};

inline size_t qHash(const PkgKey &key, const size_t seed = 0) {
    return qHashMulti(seed, key.name, key.version, key.repo);
}

/**
 * @class RulesManager
 * @brief Handles reading, writing, and parsing rules in /var/lib/klass/klass.conf.
 */
class RulesManager {
public:
    static QList<RuleEntry> loadRules();

    static bool addRule(const RuleEntry &rule);

    static bool removeRules(const QList<RuleEntry> &rules);

    static bool editRule(const RuleEntry &oldRule, const RuleEntry &newRule);

    static QHash<PkgKey, RuleSt> resolveStatuses(const QList<PkgInfo> &allPackages, const QList<RuleEntry> &r);
};

#endif
