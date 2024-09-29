#include "localehelper.h"

#include <QDir>
#include <QRegularExpression>

QTranslator* LocaleHelper::loadBestTranslation(const QString &baseName, const QString &directory)
{
    QTranslator* translator = new QTranslator;
    QLocale bestLocale = findBestLocale(baseName, directory);
    bool result = translator->load(bestLocale, baseName, "_", directory);
    return translator;
}

QLocale LocaleHelper::findBestLocale(const QString &baseName, const QString &directory)
{
    QLocale locale = QLocale::system();
    QList<QLocale> lcls = availableLocales(baseName, directory);

    // Try an exact match
    if (lcls.contains(locale)) {
        return locale;
    }

    // Try matching the generic language
    QLocale genericLocale(locale.language());
    if (lcls.contains(genericLocale)) {
        return genericLocale;
    }

    // Try variants from the same language
    for (const QLocale &variant : lcls) {
        if (variant.language() == locale.language()) {
            return variant;
        }
    }

    // Use English as default, if no match is found
    return QLocale::English;
}

QList<QLocale> LocaleHelper::availableLocales(const QString &baseName, const QString &directory)
{
    QList<QLocale> locales;
    QDir dir(directory);

    if (!dir.exists()) {
        qDebug() << "Translation directory does not exist:" << directory;
        return locales;
    }

    QStringList filters;
    filters << baseName + "_*.qm";  // Filter .qm files
    dir.setNameFilters(filters);

    // Regex para extrair o código de localidade do nome do arquivo
    QRegularExpression re(baseName + "_([a-zA-Z_]+)\\.qm");

    for (const QString &fileName : dir.entryList()) {
        QRegularExpressionMatch match = re.match(fileName);
        if (match.hasMatch()) {
            QString localeCode = match.captured(1);
            QLocale locale(localeCode);
            locales.append(locale);
        }
    }

    return locales;
}
