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

#include <QLibrary>

#include "decoderBase.h"
#include "vvdec/vvdec.h"

namespace decoder
{

struct LibraryFunctionsVVDec
{
  // General functions
  const char *(*vvdec_get_version)(void){};

  vvdecAccessUnit *(*vvdec_accessUnit_alloc)(){};
  void (*vvdec_accessUnit_free)(vvdecAccessUnit *accessUnit){};
  void (*vvdec_accessUnit_alloc_payload)(vvdecAccessUnit *accessUnit, int payload_size){};
  void (*vvdec_accessUnit_free_payload)(vvdecAccessUnit *accessUnit){};
  void (*vvdec_accessUnit_default)(vvdecAccessUnit *accessUnit){};

  void (*vvdec_params_default)(vvdecParams *param){};
  vvdecParams *(*vvdec_params_alloc)(){};
  void (*vvdec_params_free)(vvdecParams *params){};

  vvdecDecoder *(*vvdec_decoder_open)(vvdecParams *){};
  int (*vvdec_decoder_close)(vvdecDecoder *){};

  int (*vvdec_set_logging_callback)(vvdecDecoder *, vvdecLoggingCallback callback){};
  int (*vvdec_decode)(vvdecDecoder *, vvdecAccessUnit *accessUnit, vvdecFrame **frame){};
  int (*vvdec_flush)(vvdecDecoder *, vvdecFrame **frame){};
  int (*vvdec_frame_unref)(vvdecDecoder *, vvdecFrame *frame){};

  int (*vvdec_get_hash_error_count)(vvdecDecoder *){};
  const char *(*vvdec_get_dec_information)(vvdecDecoder *){};
  const char *(*vvdec_get_last_error)(vvdecDecoder *){};
  const char *(*vvdec_get_last_additional_error)(vvdecDecoder *){};
  const char *(*vvdec_get_error_msg)(int nRet){};
};

// This class wraps the decoder library in a demand-load fashion.
class decoderVVDec : public decoderBaseSingleLib
{
public:
  decoderVVDec();
  decoderVVDec(bool loadLibrary);
  ~decoderVVDec();

  void resetDecoder() override;

  // Decoding / pushing data
  bool       decodeNextFrame() override;
  QByteArray getRawFrameData() override;
  bool       pushData(QByteArray &data) override;

  // Check if the given library file is an existing libde265 decoder that we can use.
  static bool checkLibraryFile(QString libFilePath, QString &error);

  QString getDecoderName() const override;
  QString getCodecName() const override { return "hevc"; }

private:
  void loadLibrary();

  // Return the possible names of the HM library
  QStringList getLibraryNames() const override;

  // Try to resolve all the required function pointers from the library
  void resolveLibraryFunctionPointers() override;

  // The function template for resolving the functions.
  // This can not go into the base class because then the template
  // generation does not work.
  template <typename T> T resolve(T &ptr, const char *symbol, bool optional = false);

  void allocateNewDecoder();

  vvdecDecoder *   decoder{nullptr};
  vvdecAccessUnit *accessUnit{nullptr};
  vvdecFrame *     currentFrame{nullptr};

  // Try to get the next picture from the decoder and save it in currentHMPic
  bool getNextFrameFromDecoder();

  int  nrSignals{0};
  bool flushing{false};

  // We buffer the current image as a QByteArray so you can call getYUVFrameData as often as
  // necessary without invoking the copy operation from the hm image buffer to the QByteArray again.
  QByteArray currentOutputBuffer;
  void       copyImgToByteArray(QByteArray &dst);

  bool currentFrameReadyForRetrieval{};

  LibraryFunctionsVVDec lib{};
};

} // namespace decoder
