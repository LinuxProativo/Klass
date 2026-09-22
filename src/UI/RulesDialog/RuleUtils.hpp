/**
 * @file RuleUtils.hpp
 * @brief Helper utilities for rule management and repository UI population.
 */

#ifndef RULEUTILS_HPP
#define RULEUTILS_HPP

#include <QComboBox>
#include <QStandardItemModel>

namespace RuleUtils {
    inline const auto RULE_PATTERN_PLACEHOLDER = QObject::tr("Exact name or Regex pattern (e.g., qt6, ^ffmpeg-.*)");

    QString displayRepositoryName(const QString &repo);

    void populateRepoCombo(QComboBox *combo, const QStringList &repos, bool includeAllOption = false);

    int findRuleRow(const QStandardItemModel *model, const RuleEntry &rule);
}

#endif
