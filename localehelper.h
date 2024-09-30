#ifndef LOCALEHELPER_H
#define LOCALEHELPER_H

#include <QLocale>
#include <QTranslator>

class LocaleHelper
{
public:
    static void loadBestTranslation(const QString &baseName = "atsumari", const QString &directory = ":/i18n");
    static void loadTranslation(const QLocale &locale, const QString &baseName = "atsumari", const QString &directory = ":/i18n");
    static QLocale findBestLocale(const QLocale &locale = QLocale::system(), const QString &baseName = "atsumari", const QString &directory = ":/i18n");
    static QList<QLocale> availableLocales(const QString &baseName = "atsumari", const QString &directory = ":/i18n");

private:
    static QTranslator translator;
};

#endif // LOCALEHELPER_H
