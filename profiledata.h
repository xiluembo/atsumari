#ifndef PROFILEDATA_H
#define PROFILEDATA_H

#include <QString>

class ProfileData
{
public:
    ProfileData();

    QString profileName() const;
    void setProfileName(const QString &newProfileName);
    QString diffuseColor() const;
    void setDiffuseColor(const QString &newDiffuseColor);
    QString specularColor() const;
    void setSpecularColor(const QString &newSpecularColor);
    QString ambientColor() const;
    void setAmbientColor(const QString &newAmbientColor);
    QString lightColor() const;
    void setLightColor(const QString &newLightColor);
    QString decorationPath() const;
    void setDecorationPath(const QString &newDecorationPath);
    int lightIntensity() const;
    void setLightIntensity(int newLightIntensity);
    int slices() const;
    void setSlices(int newSlices);
    int rings() const;
    void setRings(int newRings);
    int shininess() const;
    void setShininess(int newShininess);
    double iteration() const;
    void setIteration(double newIteration);
    QString font() const;
    void setFont(const QString &newFont);

private:
    QString m_profileName;
    QString m_diffuseColor;
    QString m_specularColor;
    QString m_ambientColor;
    QString m_lightColor;
    QString m_decorationPath;
    int m_lightIntensity;
    int m_slices;
    int m_rings;
    int m_shininess;
    double m_iteration;
    QString m_font;
};

#endif // PROFILEDATA_H
