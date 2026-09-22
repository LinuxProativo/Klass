/**
 * @file RuleUtils.cpp
 * @brief Implementation of RuleUtils helper functions.
 */

#include <QObject>

#include <RulesManager.hpp>
#include <RuleUtils.hpp>
#include <SlackwareDefines.hpp>

namespace RuleUtils {
    /**
     * @brief Converts an internal repository identifier into a user-visible display name.
     * @param repo Internal repository identifier.
     * @return Localized display name.
     */
    QString displayRepositoryName(const QString &repo) {
        if (repo == QStringLiteral("all"))
            return QObject::tr("Any Repository");

        if (repo == SLACK_OTHERS)
            return QObject::tr("Others");

        QString text = repo;
        text[0] = text[0].toUpper();
        return text;
    }

    /**
     * @brief Populates a repository QComboBox with formatted names and standard fallback items.
     * @param combo The QComboBox to populate.
     * @param repos List of repository identifiers.
     * @param includeAllOption Whether to include the "Any Repository" (all) entry at top.
     */
    void populateRepoCombo(QComboBox *combo, const QStringList &repos, const bool includeAllOption) {
        combo->blockSignals(true);
        combo->clear();

        if (includeAllOption)
            combo->addItem(QObject::tr("Any Repository"), QStringLiteral("all"));

        for (const QString &repo: repos)
            combo->addItem(displayRepositoryName(repo), repo);

        combo->addItem(QObject::tr("Others"), SLACK_OTHERS);
        combo->blockSignals(false);
    }

    /**
     * @brief Searches for a rule entry within the specified table model.
     * @param model Pointer to the item model to search within.
     * @param rule The rule entry criteria used for matching.
     * @return Zero-based row index if found, or -1 if no match exists.
     */
    int findRuleRow(const QStandardItemModel *model, const RuleEntry &rule) {
        for (int row = 0; row < model->rowCount(); ++row) {
            if (rule.type == QLatin1String("exception")) {
                if (model->item(row, 0)->data(Qt::UserRole).toString() == rule.repo &&
                    model->item(row, 1)->text() == rule.scope &&
                    model->item(row, 2)->text() == rule.rule)
                    return row;
            } else if (rule.type == QLatin1String("priority")) {
                if (model->item(row, 0)->text() == rule.rule &&
                    model->item(row, 1)->data(Qt::UserRole).toString() == rule.repo)
                    return row;
            }
        }
        return -1;
    }
}
