#ifndef LOCALEHELPER_H
#define LOCALEHELPER_H

#include <QLocale>
#include <QTranslator>

class LocaleHelper
{
public:
    static void loadBestTranslation(const QString &baseName, const QString &directory);
    static QLocale findBestLocale(const QString &baseName, const QString &directory);
    static QList<QLocale> availableLocales(const QString &baseName, const QString &directory);

private:
    static QTranslator translator;
};

#endif // LOCALEHELPER_H
