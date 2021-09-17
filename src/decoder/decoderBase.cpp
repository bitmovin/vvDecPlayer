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

#include "decoderBase.h"

#include <QCoreApplication>
#include <QDir>
#include <QSettings>

namespace decoder
{

decoderBase::decoderBase() { this->resetDecoder(); }

void decoderBase::resetDecoder()
{
  this->decoderState = DecoderState::NeedsMoreData;
  this->frameSize    = {};
  this->formatYUV    = {};
  this->rawFormat    = RawFormat::Invalid;
}

void decoderBaseSingleLib::loadDecoderLibrary(QString specificLibrary)
{
  // Try to load the HM library from the current working directory
  // Unfortunately relative paths like this do not work: (at least on windows)
  // library.setFileName(".\\libde265");

  bool libLoaded = false;

  // Try the specific library first
  library.setFileName(specificLibrary);
  libraryPath = specificLibrary;
  libLoaded   = library.load();

  if (!libLoaded)
  {
    // Try various paths/names next
    // If the file name is not set explicitly, QLibrary will try to open
    // the decLibXXX.so file first. Since this has been compiled for linux
    // it will fail and not even try to open the decLixXXX.dylib.
    // On windows and linux ommitting the extension works
    auto libNames = getLibraryNames();

    // Get the additional search path from the settings
    QSettings settings;
    settings.beginGroup("Decoders");
    QString searchPath = settings.value("SearchPath", "").toString();
    if (!searchPath.endsWith("/"))
      searchPath.append("/");
    searchPath.append("%1");
    settings.endGroup();

    QStringList const libPaths =
        QStringList() << searchPath << QDir::currentPath() + "/%1"
                      << QDir::currentPath() + "/decoder/%1"
                      << QDir::currentPath() +
                             "/libde265/%1" // for legacy installations before the decoders were
                                            // moved to the "decoders" folder
                      << QCoreApplication::applicationDirPath() + "/%1"
                      << QCoreApplication::applicationDirPath() + "/decoder/%1"
                      << QCoreApplication::applicationDirPath() + "/libde265/%1"
                      << "%1"; // Try the system directories.

    for (auto &libName : libNames)
    {
      for (auto &libPath : libPaths)
      {
        library.setFileName(libPath.arg(libName));
        libraryPath = libPath.arg(libName);
        libLoaded   = library.load();
        if (libLoaded)
          break;
      }
      if (libLoaded)
        break;
    }
  }

  if (!libLoaded)
  {
    libraryPath.clear();
    QString error = "Error loading library: " + library.errorString() + "\n";
    error += "We could not load one of the supported decoder library (";
    auto libNames = getLibraryNames();
    for (int i = 0; i < libNames.count(); i++)
    {
      if (i == 0)
        error += libNames[0];
      else
        error += ", " + libNames[i];
    }
    error +=
        ") Please download the decoder library and put it in a known path or set up the path.\n";
    return setError(error);
  }

  resolveLibraryFunctionPointers();
}

} // namespace decoder
