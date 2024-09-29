#ifndef LOCALEHELPER_H
#define LOCALEHELPER_H

#include <QLocale>
#include <QTranslator>

class LocaleHelper
{
public:
    static QTranslator* loadBestTranslation(const QString &baseName, const QString &directory);
    static QLocale findBestLocale(const QString &baseName, const QString &directory);
    static QList<QLocale> availableLocales(const QString &baseName, const QString &directory);
};

#endif // LOCALEHELPER_H
