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

inline const auto EXCEPT = QStringLiteral("exception");
inline const auto PRIORITY = QStringLiteral("priority");
inline const auto BY_CAT = QStringLiteral("category");
inline const auto BY_PKG = QStringLiteral("package");
inline const QRegularExpression RULE_REGEX(R"(^RULES\['(exception|priority)'\]=\(([^,]*),([^,]*),(.*)\)$)");

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
 * @struct CompiledRule
 * @brief Internal structure for RulesManager to pre-compile Regexes and speed up resolution.
 */
struct CompiledRule {
    RuleEntry entry;
    QRegularExpression regex;
    bool isPackageScope{false};
    bool isAllRepo{false};
    bool isRegexValid{false};
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

/**
 * @brief Generates a hash value for a PkgKey object.
 * @param key The PkgKey instance to be hashed, containing name, version, and repo.
 * @param seed An optional seed value to initialize the hash calculation.
 * @return A size_t hash value combining the seed and the key's fields.
 */
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

private:
    static bool matchesCompiledRule(const CompiledRule &cRule, const QString &pkg, const QString &ver,
                                    const QString &cat, const QString &repo);
};

#endif
