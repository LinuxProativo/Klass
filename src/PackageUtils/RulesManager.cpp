/**
 * @file RulesManager.cpp
 * @brief Implementation of RulesManager for parsing and serializing exception/priority rules.
 */

#include <QRegularExpression>

#include <RepoUtils.hpp>
#include <RulesManager.hpp>
#include <SlackwareDefines.hpp>

/**
 * @brief Checks if a package, version, repository, and category match a compiled rule.
 * @param cRule The compiled rule containing evaluation flags and regular expressions.
 * @param pkg The name of the package to be evaluated.
 * @param ver The version of the package.
 * @param cat The category or scope of the package.
 * @param repo The repository where the package originates.
 * @return True if the package details satisfy the rule criteria; otherwise false.
 */
bool RulesManager::matchesCompiledRule(const CompiledRule &cRule, const QString &pkg, const QString &ver,
                                       const QString &cat, const QString &repo) {
    const RuleEntry &rule = cRule.entry;

    if (!cRule.isAllRepo && !rule.repo.isEmpty() && rule.repo != repo)
        return false;

    if (!cRule.isPackageScope) {
        if (cat.isEmpty())
            return false;

        if (cat.compare(rule.rule, Qt::CaseInsensitive) == 0)
            return true;

        return cRule.isRegexValid && cRule.regex.match(cat).hasMatch();
    }

    const QString fullPkgVersion = ver.isEmpty() ? pkg : QStringLiteral("%1-%2").arg(pkg, ver);

    if (rule.rule.compare(pkg, Qt::CaseInsensitive) == 0 ||
        rule.rule.compare(fullPkgVersion, Qt::CaseInsensitive) == 0) {
        return true;
    }

    if (cRule.isRegexValid) {
        if (cRule.regex.match(pkg).hasMatch() || cRule.regex.match(fullPkgVersion).hasMatch())
            return true;
    }

    return false;
}

/**
 * @brief Parses raw klass.conf lines into structured RuleEntry objects.
 * @return List of parsed RuleEntry items.
 */
QList<RuleEntry> RulesManager::loadRules() {
    const QStringList lines = RepoUtils::readConf();
    QList<RuleEntry> rules;

    for (const QString &line: lines) {
        if (const QRegularExpressionMatch match = RULE_REGEX.match(line.trimmed()); match.hasMatch()) {
            RuleEntry entry;
            entry.type = match.captured(1);
            entry.repo = match.captured(2);
            entry.scope = match.captured(3);
            entry.rule = match.captured(4);
            rules.append(entry);
        }
    }
    return rules;
}

/**
 * @brief Appends a single rule line under its section (# Exceptions or # Priorities).
 * @param rule RuleEntry to persist.
 * @return True on success.
 */
bool RulesManager::addRule(const RuleEntry &rule) {
    const QString targetLine = rule.toConfigLine();
    const QString header = (rule.type == EXCEPT) ? EXCEPTION_HEADER : PRIORITY_HEADER;
    return RepoUtils::appendConfigLine(header, targetLine);
}

/**
 * @brief Removes rule lines completely from klass.conf.
 * @param rules List of RuleEntry items to remove.
 * @return True on success.
 */
bool RulesManager::removeRules(const QList<RuleEntry> &rules) {
    QSet<QString> targets;
    targets.reserve(rules.size());
    for (const RuleEntry &r: rules)
        targets.insert(r.toConfigLine());

    return RepoUtils::removeConfigLines(targets);
}

/**
 * @brief Replaces an existing rule line with a new one in place.
 * @param oldRule The rule currently stored in klass.conf.
 * @param newRule The rule's new replacement values.
 * @return True on success.
 */
bool RulesManager::editRule(const RuleEntry &oldRule, const RuleEntry &newRule) {
    return RepoUtils::replaceConfigLine(oldRule.toConfigLine(), newRule.toConfigLine());
}

/**
 * @brief Resolves exception and priority rules across the cross-repository package universe.
 * @param allPkgs List of pointers to package metadata across all loaded repositories.
 * @param r List of user-defined exception and priority rules.
 * @return QHash mapping each composite package key to its resolved RuleSt status.
 */
QHash<PkgKey, RuleSt> RulesManager::resolveStatuses(const QList<const PkgInfo *> &allPkgs, const QList<RuleEntry> &r) {
    QHash<PkgKey, RuleSt> result;
    result.reserve(allPkgs.size());

    QList<CompiledRule> exceptionRules;
    QList<CompiledRule> priorityRules;

    for (const RuleEntry &rule: r) {
        CompiledRule cRule;
        cRule.entry = rule;
        cRule.isPackageScope = (rule.scope == BY_PKG);
        cRule.isAllRepo = (rule.repo == ALL_REPOSITORIES);
        cRule.regex = QRegularExpression(rule.rule, QRegularExpression::CaseInsensitiveOption);
        cRule.isRegexValid = cRule.regex.isValid() && !rule.rule.trimmed().isEmpty();

        if (rule.type == EXCEPT)
            exceptionRules.append(cRule);
        else if (rule.type == PRIORITY)
            priorityRules.append(cRule);
    }

    for (const auto *pkg: allPkgs)
        result.insert({pkg->name, pkg->version, pkg->repoName}, RuleSt::Normal);

    QSet<QString> excludedGlobalNames;
    for (const CompiledRule &cRule: exceptionRules) {
        if (!cRule.isPackageScope || !cRule.isAllRepo)
            continue;

        for (const auto *pkg: allPkgs) {
            if (matchesCompiledRule(cRule, pkg->name, pkg->version, pkg->category, pkg->repoName))
                excludedGlobalNames.insert(pkg->name);
        }
    }

    for (const auto *pkg: allPkgs) {
        bool isExcluded = excludedGlobalNames.contains(pkg->name);

        if (!isExcluded) {
            for (const CompiledRule &cRule: exceptionRules) {
                if (matchesCompiledRule(cRule, pkg->name, pkg->version, pkg->category, pkg->repoName)) {
                    isExcluded = true;
                    break;
                }
            }
        }

        if (isExcluded)
            result[{pkg->name, pkg->version, pkg->repoName}] = RuleSt::Excluded;
    }

    if (priorityRules.isEmpty())
        return result;

    QHash<QString, QList<const PkgInfo *> > byName;
    byName.reserve(allPkgs.size());
    for (const auto *pkg: allPkgs)
        byName[pkg->name].append(pkg);

    for (auto it = byName.constBegin(); it != byName.constEnd(); ++it) {
        const QList<const PkgInfo *> &instances = it.value();

        const PkgInfo *winner = nullptr;
        for (const auto *inst: instances) {
            for (const CompiledRule &cRule: priorityRules) {
                if (matchesCompiledRule(cRule, inst->name, inst->version, inst->category, inst->repoName)) {
                    winner = inst;
                    break;
                }
            }
            if (winner) break;
        }

        if (!winner)
            continue;

        for (const auto *inst: instances) {
            if (inst->repoName != winner->repoName)
                result[{inst->name, inst->version, inst->repoName}] = RuleSt::Excluded;
            else if (inst == winner)
                result[{inst->name, inst->version, inst->repoName}] = RuleSt::Prioritized;
        }
    }

    return result;
}
