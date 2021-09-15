/*  Copyright: Christian Feldmann (christian.feldmann@bitmovin.com)
 */

#pragma once

#include <common/ILogger.h>
#include <common/typedef.h>

#include <QString>

class ManifestFile
{
public:
  ManifestFile(ILogger *logger);

  bool openJsonManifestFile(QString filename);

  bool openPredefinedManifest(unsigned predefinedManifestID);

  void gotoSegment(unsigned segmentNumber) { this->currentSegment = segmentNumber; }

  struct Segment
  {
    unsigned segmentNumber{};
    unsigned rendition{};
    QString downloadUrl;
  };
  Segment getNextSegment();

private:
  ILogger *logger{};

  bool openFromData(QByteArray data);

  QString filename;

  QString name{"NoName"};
  unsigned numberSegments{};

  struct Rendition
  {
    QString name;
    Size resolution{};
    double fps{};
    QString url;
  };
  std::vector<Rendition> renditions;

  unsigned currentRendition{};
  unsigned currentSegment{};
};
