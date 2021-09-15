/*  Copyright: Christian Feldmann (christian.feldmann@bitmovin.com)
 */

#include "ManifestFile.h"

#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>

namespace
{

Size parseResolutionString(QString resolutionString)
{
  if (resolutionString.count("x") != 1)
    throw std::logic_error("Resolution string does not contain 'x'");

  auto parts = resolutionString.split("x");
  if (parts.count() != 2)
    throw std::logic_error("Resolution string splitting failed");

  auto width  = parts[0].toInt();
  auto height = parts[1].toInt();

  if (width <= 0 || height <= 0)
    throw std::logic_error("Resolution string has invalid numbers");

  return {width, height};
}

const auto PREDEFINED_MANIFEST_0 = QByteArray(
  R"--json--({
    "Name": "Sintel 720p only",
    "NrSegments": 888,
    "Renditions": [
      {
        "Name": "Remote Sintel AWS S3 720p",
        "Resolution": "1696x720",
        "Fps": 24,
        "Url": "https://bitmovin-api-eu-west1-ci-input.s3.amazonaws.com/feldmann/VVCDemo/Sintel720p/segment-%i.vvc"
      }
    ]
  })--json--"
);

} // namespace

ManifestFile::ManifestFile(ILogger *logger) { this->logger = logger; }

bool ManifestFile::openPredefinedManifest(unsigned predefinedManifestID)
{
  if (predefinedManifestID == 0)
  {
    return this->openFromData(PREDEFINED_MANIFEST_0);
  }
  return false;
}

bool ManifestFile::openJsonManifestFile(QString filename)
{
  this->filename = filename;

  QFile file;
  file.setFileName(filename);
  file.open(QIODevice::ReadOnly | QIODevice::Text);

  return this->openFromData(file.readAll());
}

bool ManifestFile::openFromData(QByteArray data)
{
  if (data.isEmpty())
  {
    this->logger->addMessage("No data to open", LoggingPriority::Error);
    return false;
  }

  try
  {
    auto doc = QJsonDocument::fromJson(data);

    if (!doc.isObject())
      throw std::logic_error("Document should be an object");

    auto mainObject = doc.object();

    if (mainObject.contains("Name"))
      this->name = mainObject["Name"].toString();

    if (!mainObject.contains("NrSegments"))
      throw std::logic_error("NrSegments not found");
    this->numberSegments = unsigned(mainObject["NrSegments"].toInt());

    if (!mainObject.contains("Renditions"))
      throw std::logic_error("No renditions found");
    auto renditions = mainObject["Renditions"].toArray();

    for (auto renditionValue : renditions)
    {
      if (!renditionValue.isObject())
        throw std::logic_error("Rendition is not an object");
      auto renditionObject = renditionValue.toObject();

      Rendition rendition;

      if (!renditionObject.contains("Name"))
        throw std::logic_error("Rendition has no name");
      rendition.name = renditionObject["Name"].toString();

      if (!renditionObject.contains("Resolution"))
        throw std::logic_error("Rendition has no resolution");
      rendition.resolution = parseResolutionString(renditionObject["Resolution"].toString());

      if (!renditionObject.contains("Fps"))
        throw std::logic_error("Rendition has no Fps");
      rendition.fps = renditionObject["Fps"].toDouble();

      if (!renditionObject.contains("Url"))
        throw std::logic_error("Rendition has no Url");
      rendition.url = renditionObject["Url"].toString();
      if (rendition.url.count("%i") != 1)
        throw std::logic_error("Rendition URL must have exactly one %i indicator");

      this->renditions.push_back(rendition);
    }
  }
  catch (const std::exception &e)
  {
    this->logger->addMessage("Error parsing document: " + QString::fromLatin1(e.what()),
                             LoggingPriority::Error);
    return false;
  }
  return true;
}

ManifestFile::Segment ManifestFile::getNextSegment()
{
  Segment segment;
  segment.segmentNumber = this->currentSegment;
  segment.rendition     = this->currentRendition;

  auto url = this->renditions.at(this->currentRendition).url;
  url.replace("%i", QString("%1").arg(this->currentSegment));
  segment.downloadUrl = url;

  this->currentSegment++;
  if (this->currentSegment >= this->numberSegments)
    this->currentSegment = 0;

  return segment;
}
