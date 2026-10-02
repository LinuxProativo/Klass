/**
 * @file RulesManager.cpp
 * @brief Implementation of RulesManager for parsing and serializing exception/priority rules.
 */

#include <QRegularExpression>

#include <RepoManager.hpp>
#include <RulesManager.hpp>
#include <SlackwareDefines.hpp>

/**
 * @brief Checks if a specific package, version, repository, and category match a compiled rule.
 * @param  cRule The compiled rule containing evaluation flags, the raw rule entry, and the pre-compiled regex.
 * @param  pkg The name of the package to be evaluated.
 * @param  ver The version of the package (can be empty).
 * @param  cat The category or scope of the package.
 * @param  repo The repository where the package originates.
 * @return True if the package details satisfy the rule criteria; otherwise false.
 */
bool RulesManager::matchesCompiledRule(const CompiledRule &cRule, const QString &pkg, const QString &ver,
                                       const QString &cat, const QString &repo) {
    const RuleEntry &rule = cRule.entry;

    if (!cRule.isAllRepo && !rule.repo.isEmpty() && rule.repo != repo)
        return false;

    if (!cRule.isPackageScope)
        return rule.rule.contains(cat) || cat.contains(rule.rule);

    const QString fullPkgVersion = ver.isEmpty() ? pkg : QStringLiteral("%1-%2").arg(pkg, ver);

    if (rule.rule == pkg || rule.rule == fullPkgVersion)
        return true;

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
    const QStringList lines = RepoManager::readConf();
    QList<RuleEntry> rules{};

    for (const QString &line: lines) {
        const QString trimmed = line.trimmed();
        if (QRegularExpressionMatch match = RULE_REGEX.match(trimmed); match.hasMatch()) {
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
 */
bool RulesManager::addRule(const RuleEntry &rule) {
    QStringList lines = RepoManager::readConf();
    const QString targetLine = rule.toConfigLine();

    for (const QString &line: lines) {
        if (line.trimmed() == targetLine)
            return true;
    }

    const QString header = rule.type == EXCEPT ? EXCEPTION_HEADER : PRIORITY_HEADER;

    if (const int index = static_cast<int>(lines.indexOf(header)); index != -1) {
        lines.insert(index + 1, targetLine);
    } else {
        lines.append(targetLine);
    }

    return RepoManager::writeConf(lines);
}

/**
 * @brief Remove a rules lines completely from klass.conf.
 */
bool RulesManager::removeRules(const QList<RuleEntry> &rules) {
    const QStringList currentLines = RepoManager::readConf();

    QSet<QString> targets;
    for (const RuleEntry &r: rules)
        targets.insert(r.toConfigLine());

    QStringList updatedLines;
    for (const QString &line: currentLines) {
        if (!targets.contains(line.trimmed()))
            updatedLines.append(line);
    }

    return RepoManager::writeConf(updatedLines);
}

/**
 * @brief Replaces an existing rule line with a new one in a single read/write pass.
 * @param oldRule The rule as it currently exists in klass.conf, used to locate its line.
 * @param newRule The rule's new values, written in place of the old line.
 * @return True if the old line was found and the file was written successfully, or if the old line was already absent.
 */
bool RulesManager::editRule(const RuleEntry &oldRule, const RuleEntry &newRule) {
    QStringList lines = RepoManager::readConf();
    const QString oldLine = oldRule.toConfigLine();
    const QString newLine = newRule.toConfigLine();

    bool found = false;
    for (QString &line: lines) {
        if (line.trimmed() == oldLine) {
            line = newLine;
            found = true;
            break;
        }
    }

    if (!found)
        return true;

    return RepoManager::writeConf(lines);
}

/**
 * @brief Resolves exception and priority rules across the full cross-repository package universe at once.
 */
QHash<PkgKey, RuleSt> RulesManager::resolveStatuses(const QList<PkgInfo> &allPackages, const QList<RuleEntry> &r) {
    QHash<PkgKey, RuleSt> result;
    result.reserve(allPackages.size());

    QList<CompiledRule> compiledRules;
    compiledRules.reserve(r.size());
    for (const RuleEntry &rule: r) {
        CompiledRule cRule;
        cRule.entry = rule;
        cRule.isPackageScope = rule.scope == BY_PKG;
        cRule.isAllRepo = rule.repo == ALL_REPOSITORIES;
        if (cRule.isPackageScope) {
            cRule.regex = QRegularExpression(rule.rule);
            cRule.isRegexValid = cRule.regex.isValid();
        } else {
            cRule.isRegexValid = false;
        }
        compiledRules.append(cRule);
    }

    for (const auto &pkg: allPackages)
        result.insert({pkg.name, pkg.version, pkg.repoName}, RuleSt::Normal);

    QSet<QString> excludedNames;
    for (const CompiledRule &cRule: compiledRules) {
        if (cRule.entry.type != EXCEPT || !cRule.isPackageScope || !cRule.isAllRepo)
            continue;

        for (const auto &pkg: allPackages) {
            if (matchesCompiledRule(cRule, pkg.name, pkg.version, pkg.category, pkg.repoName))
                excludedNames.insert(pkg.name);
        }
    }

    for (const auto &pkg: allPackages) {
        bool isExcludedLocal = false;
        if (!excludedNames.contains(pkg.name)) {
            for (const CompiledRule &cRule: compiledRules) {
                if (cRule.entry.type == EXCEPT &&
                    matchesCompiledRule(cRule, pkg.name, pkg.version, pkg.category, pkg.repoName)) {
                    isExcludedLocal = true;
                    break;
                }
            }
        }

        if (excludedNames.contains(pkg.name) || isExcludedLocal)
            result[{pkg.name, pkg.version, pkg.repoName}] = RuleSt::Excluded;
    }

    QHash<QString, QList<const PkgInfo *> > byName;
    byName.reserve(allPackages.size());
    for (const auto &pkg: allPackages)
        byName[pkg.name].append(&pkg);

    for (auto it = byName.constBegin(); it != byName.constEnd(); ++it) {
        const QList<const PkgInfo *> &instances = it.value();

        const PkgInfo *winner = nullptr;
        for (const auto *inst: instances) {
            for (const CompiledRule &cRule: compiledRules) {
                if (cRule.entry.type == PRIORITY &&
                    matchesCompiledRule(cRule, inst->name, inst->version, inst->category, inst->repoName)) {
                    winner = inst;
                    break;
                }
            }
            if (winner) break;
        }

        if (!winner)
            continue;

        for (const auto *inst: instances) {
            if (inst->repoName != winner->repoName) {
                result[{inst->name, inst->version, inst->repoName}] = RuleSt::Excluded;
            } else if (inst == winner) {
                result[{inst->name, inst->version, inst->repoName}] = RuleSt::Prioritized;
            }
        }
    }

    QSet<QPair<QString, QString> > excludedNameVersions;
    for (const auto &pkg: allPackages) {
        if (result.value({pkg.name, pkg.version, pkg.repoName}) == RuleSt::Excluded)
            excludedNameVersions.insert({pkg.name, pkg.version});
    }

    for (const auto &pkg: allPackages) {
        const PkgKey key = {pkg.name, pkg.version, pkg.repoName};
        if (result.value(key) == RuleSt::Prioritized)
            continue;
        if (excludedNameVersions.contains({pkg.name, pkg.version}))
            result[key] = RuleSt::Excluded;
    }

    return result;
}
