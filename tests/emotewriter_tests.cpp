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

  QFontDatabase db;
  QVERIFY(db.families().contains("DejaVu Sans"));
  QVERIFY(db.families().contains("DejaVu Serif"));

  QSettings settings;
  settings.beginWriteArray(CFG_PROFILES, 1);
  settings.setArrayIndex(0);
  settings.setValue(CFG_EMOJI_FONT, "DejaVu Sans");
  settings.endArray();
  settings.setValue(CFG_CURRENT_PROFILE, 0);
  settings.sync();

  settings.beginReadArray(CFG_PROFILES);
  settings.setArrayIndex(0);
  QString sansFont =
      settings.value(CFG_EMOJI_FONT, DEFAULT_EMOJI_FONT).toString();
  settings.endArray();

  EmoteWriter writer;
  writer.saveEmoji("sans", "A", sansFont);
  QImage sansImage = writer.pixmapFor("sans").toImage();

  settings.beginWriteArray(CFG_PROFILES, 1);
  settings.setArrayIndex(0);
  settings.setValue(CFG_EMOJI_FONT, "DejaVu Serif");
  settings.endArray();
  settings.sync();

  settings.beginReadArray(CFG_PROFILES);
  settings.setArrayIndex(0);
  QString serifFont =
      settings.value(CFG_EMOJI_FONT, DEFAULT_EMOJI_FONT).toString();
  settings.endArray();

  writer.saveEmoji("serif", "A", serifFont);
  QImage serifImage = writer.pixmapFor("serif").toImage();

  QVERIFY(sansImage != serifImage);
}

QTEST_MAIN(EmoteWriterFontTests)
#include "emotewriter_tests.moc"
