/* MIT License

Copyright (c) 2021 Christian Feldmann <christian.feldmann@gmx.de>
                                      <christian.feldmann@bitmovin.com>

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE. */

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

Segment::PlaybackInfo createPlaybackInfoForRendition(const ManifestFile::Rendition &rendition,
                                                     unsigned                       currentSegment,
                                                     unsigned currentRendition)
{
  Segment::PlaybackInfo segmentInfo;
  segmentInfo.segmentNumber = currentSegment;
  segmentInfo.rendition     = currentRendition;
  segmentInfo.fps           = rendition.fps;

  auto url = rendition.url;
  url.replace("%i", QString("%1").arg(currentSegment));
  segmentInfo.downloadUrl = url;

  return segmentInfo;
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
  })--json--");

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

    if (mainObject.contains("PlotMaxBitrate"))
      this->plotMaxBitrate = unsigned(mainObject["PlotMaxBitrate"].toInt());

    if (!mainObject.contains("Renditions"))
      throw std::logic_error("No renditions found");
    auto renditions = mainObject["Renditions"].toArray();

    if (mainObject.contains("OpenGOPAdaptiveResolutionChange"))
      this->openGopAdaptiveResolutionChange =
          mainObject["OpenGOPAdaptiveResolutionChange"].toBool();

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

void ManifestFile::increaseRendition()
{
  if (this->currentRendition < this->renditions.size() - 1)
    this->currentRendition++;
}

void ManifestFile::decreaseRendition()
{
  if (this->currentRendition > 0)
    this->currentRendition--;
}

std::optional<ManifestFile::Rendition> ManifestFile::getCurrentRenditionInfo() const
{
  auto id = this->currentRendition;
  if (id >= this->renditions.size())
    return {};
  return this->renditions.at(id);
}

Segment::PlaybackInfo ManifestFile::getNextSegment()
{
  auto &rendition = this->renditions.at(this->currentRendition);

  auto segmentInfo =
      createPlaybackInfoForRendition(rendition, this->currentSegment, this->currentRendition);

  this->currentSegment++;
  if (this->currentSegment >= this->numberSegments)
    this->currentSegment = 0;

  return segmentInfo;
}

Segment::PlaybackInfo ManifestFile::getSegmentSPSHighestRendition()
{
  auto &rendition = this->renditions.back();
  return createPlaybackInfoForRendition(rendition, 0, unsigned(this->renditions.size()) - 1);
}
