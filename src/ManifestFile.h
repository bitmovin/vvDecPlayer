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

#pragma once

#include <common/ILogger.h>
#include <common/Segment.h>
#include <common/typedef.h>

#include <QString>
#include <optional>

class ManifestFile
{
public:
  ManifestFile(ILogger *logger);

  bool openJsonManifestFile(QString filename);

  bool openPredefinedManifest(unsigned predefinedManifestID);

  void gotoSegment(unsigned segmentNumber) { this->currentSegment = segmentNumber; }
  void increaseRendition();
  void decreaseRendition();

  struct Rendition
  {
    QString name;
    Size    resolution{};
    double  fps{};
    QString url;
  };
  std::optional<Rendition> getCurrentRenditionInfo() const;
  unsigned                 getPlotMaxBitrate() const { return this->plotMaxBitrate; }

  Segment::PlaybackInfo getNextSegment();

private:
  ILogger *logger{};

  bool openFromData(QByteArray data);

  QString filename;

  QString  name{"NoName"};
  unsigned numberSegments{};
  unsigned plotMaxBitrate{};

  std::vector<Rendition> renditions;

  unsigned currentRendition{};
  unsigned currentSegment{};
};
