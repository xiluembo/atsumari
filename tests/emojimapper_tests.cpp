#include <QtTest>
#include <QTemporaryFile>
#include <QJsonArray>
#include <QJsonObject>
#include <QJsonDocument>

#include "../emojimapper.h"

class EmojiMapperTests : public QObject
{
    Q_OBJECT

private slots:
    void testFindsLongestMatchWithVariant();
};

void EmojiMapperTests::testFindsLongestMatchWithVariant()
{
    // Build a small emoji dataset with variants
    QJsonArray emojis;

    QJsonObject thumbsUp;
    thumbsUp["slug"] = "thumbs_up";
    thumbsUp["character"] = QString::fromUtf8("👍");
    QJsonArray thumbsVariants;
    QJsonObject thumbsVariant;
    thumbsVariant["slug"] = "thumbs_up_medium_skin_tone";
    thumbsVariant["character"] = QString::fromUtf8("👍🏽");
    thumbsVariants.append(thumbsVariant);
    thumbsUp["variants"] = thumbsVariants;
    emojis.append(thumbsUp);

    QJsonObject grinning;
    grinning["slug"] = "grinning_face";
    grinning["character"] = QString::fromUtf8("😀");
    QJsonArray grinVariants;
    QJsonObject grinVariant;
    grinVariant["slug"] = "grinning_face_light_skin_tone";
    grinVariant["character"] = QString::fromUtf8("😀🏻");
    grinVariants.append(grinVariant);
    grinning["variants"] = grinVariants;
    emojis.append(grinning);

    QJsonDocument doc(emojis);
    QTemporaryFile file;
    QVERIFY(file.open());
    file.write(doc.toJson());
    file.flush();

    EmojiMapper mapper(file.fileName());

    auto result = mapper.findBestMatch(QString::fromUtf8("👍🏽😀"));
    QCOMPARE(result.first, QString::fromUtf8("👍🏽"));
    QCOMPARE(result.second, QString("thumbs_up_medium_skin_tone"));

    result = mapper.findBestMatch(QString::fromUtf8("😀🏻"));
    QCOMPARE(result.first, QString::fromUtf8("😀🏻"));
    QCOMPARE(result.second, QString("grinning_face_light_skin_tone"));
}

QTEST_APPLESS_MAIN(EmojiMapperTests)
#include "emojimapper_tests.moc"
