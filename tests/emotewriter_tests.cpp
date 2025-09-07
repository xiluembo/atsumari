#include <QFontDatabase>
#include <QImage>
#include <QSettings>
#include <QTemporaryDir>
#include <QtTest>

#include "../emotewriter.h"
#include "../settings_defaults.h"

class EmoteWriterFontTests : public QObject {
  Q_OBJECT
private slots:
  void testFontAffectsRendering();
};

void EmoteWriterFontTests::testFontAffectsRendering() {
  QTemporaryDir tempDir;
  QVERIFY(tempDir.isValid());

  QSettings::setDefaultFormat(QSettings::IniFormat);
  QSettings::setPath(QSettings::IniFormat, QSettings::UserScope,
                     tempDir.path());
  QCoreApplication::setOrganizationName("Push X!");
  QCoreApplication::setApplicationName("Atsumari");

  QFont generalFont = QFontDatabase::systemFont(QFontDatabase::GeneralFont);
  QFont fixedFont = QFontDatabase::systemFont(QFontDatabase::FixedFont);
  if (generalFont.family() == fixedFont.family())
    QSKIP("System fonts are identical; cannot compare rendering.");

  QSettings settings;
  settings.beginWriteArray(CFG_PROFILES, 1);
  settings.setArrayIndex(0);
  settings.setValue(CFG_EMOJI_FONT, generalFont.family());
  settings.endArray();
  settings.setValue(CFG_CURRENT_PROFILE, 0);
  settings.sync();

  settings.beginReadArray(CFG_PROFILES);
  settings.setArrayIndex(0);
  QString firstFont =
      settings.value(CFG_EMOJI_FONT, DEFAULT_EMOJI_FONT).toString();
  settings.endArray();

  EmoteWriter writer;
  const QString glyph = "g";
  writer.saveEmoji("first", glyph, firstFont);
  QImage firstImage = writer.pixmapFor("first").toImage();

  settings.beginWriteArray(CFG_PROFILES, 1);
  settings.setArrayIndex(0);
  settings.setValue(CFG_EMOJI_FONT, fixedFont.family());
  settings.endArray();
  settings.sync();

  settings.beginReadArray(CFG_PROFILES);
  settings.setArrayIndex(0);
  QString secondFont =
      settings.value(CFG_EMOJI_FONT, DEFAULT_EMOJI_FONT).toString();
  settings.endArray();

  writer.saveEmoji("second", glyph, secondFont);
  QImage secondImage = writer.pixmapFor("second").toImage();

  QVERIFY(firstImage != secondImage);
}

QTEST_MAIN(EmoteWriterFontTests)
#include "emotewriter_tests.moc"
