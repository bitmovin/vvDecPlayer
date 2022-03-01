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

#include <common/EnumMapper.h>
#include <common/Typedef.h>
#include <video/PixelFormatRGB.h>
#include <video/PixelFormatYUV.h>

#include <QLibrary>

namespace decoder
{

// Each decoder is in one of these states
enum class DecoderState
{
  NeedsMoreData,  ///< The decoder needs more data (pushData). When there is no more data,
                  ///< push an empty array to flush out frames.
  RetrieveFrames, ///< Retrieve frames from the decoder (decodeNextFrame)
  EndOfBitstream, ///< Decoding has ended.
  Error
};

enum class DecoderEngine
{
  Invalid,
  // Libde265, // The libde265 decoder
  // HM,       // The HM reference software decoder
  // VTM,      // The VTM reference software decoder
  VVDec, // The VVDec VVC decoder
  // Dav1d,    // The dav1d AV1 decoder
  // FFMpeg    // The FFMpeg decoder
};

const auto DecoderEngineMapper = EnumMapper<DecoderEngine>({{DecoderEngine::VVDec, "VVDec"}});

/* This class is the abstract base class for all decoders. All decoders work like this:
 * 1. Create an instance and configure it (if required)
 * 2. Push data to the decoder until it returns that it can not take any more data.
 *    When you pushed all of the bitstream into the decoder, push an empty QByteArray to indicate
 * the EOF.
 * 3. Read frames until no new frames are coming out. Go back to 2.
 */
class decoderBase
{
public:
  decoderBase();
  virtual ~decoderBase(){};

  // Reset the decoder. Afterwards, the decoder should behave as if you just created a new one
  // (without the overhead of reloading the libraries). This must be used in case of errors or when
  // seeking.
  virtual void resetDecoder();

  // -- The decoding interface
  // If the current frame is valid, the current frame can be retrieved using getRawFrameData.
  // Call decodeNextFrame to advance to the next frame. When the function returns false, more data
  // is probably needed.
  virtual bool               decodeNextFrame() = 0;
  virtual QByteArray         getRawFrameData() = 0;
  RawFormat                  getRawFormat() const { return this->rawFormat; }
  video::yuv::PixelFormatYUV getPixelFormatYUV() const { return this->formatYUV; }
  video::rgb::PixelFormatRGB getRGBPixelFormat() const { return this->formatRGB; }
  Size                       getFrameSize() const { return this->frameSize; }
  // Push data to the decoder (until no more data is needed)
  // In order to make the interface generic, the pushData function accepts data only without start
  // codes
  virtual bool pushData(QByteArray &data) = 0;

  DecoderState state() const { return this->decoderState; }

  // Error handling
  bool    errorInDecoder() const { return decoderState == DecoderState::Error; }
  QString decoderErrorString() const { return errorString; }

  // Get the name, filename and full path to the decoder library(s) that is/are being used.
  // The length of the list must be a multiple of 3 (name, libName, fullPath)
  virtual QStringList getLibraryPaths() const = 0;

  // Get the deocder name (everyting that is needed to identify the deocder library) and the codec
  // that is being decoded. If needed, also version information (like HM 16.4)
  virtual QString getDecoderName() const = 0;
  virtual QString getCodecName() const   = 0;

protected:
  DecoderState decoderState{DecoderState::NeedsMoreData};

  int  decodeSignal{0};  ///< Which signal should be decoded?
  bool isCachingDecoder; ///< Is this the caching or the interactive decoder?

  bool internalsSupported{false}; ///< Enable in the constructor if you support statistics
  Size frameSize;

  // Some decoders are able to handel both YUV and RGB output
  RawFormat                  rawFormat;
  video::yuv::PixelFormatYUV formatYUV;
  video::rgb::PixelFormatRGB formatRGB;

  // Error handling
  void setError(const QString &reason)
  {
    decoderState = DecoderState::Error;
    errorString  = reason;
  }
  bool setErrorB(const QString &reason)
  {
    setError(reason);
    return false;
  }
  QString errorString{};
};

// This abstract base class extends the decoderBase class by the ability to load one single library
// using QLibrary. For decoding, the decoderBase interface is not changed.
class decoderBaseSingleLib : public decoderBase
{
public:
  decoderBaseSingleLib() : decoderBase(){};
  virtual ~decoderBaseSingleLib(){};

  QStringList getLibraryPaths() const override
  {
    return QStringList() << getDecoderName() << library.fileName() << library.fileName();
  }

protected:
  virtual void resolveLibraryFunctionPointers() = 0;
  void         loadDecoderLibrary(QString specificLibrary);

  // Get all possible names of the library that we will load
  virtual QStringList getLibraryNames() const = 0;

  QLibrary library;
  QString  libraryPath{};
};

} // namespace decoder
