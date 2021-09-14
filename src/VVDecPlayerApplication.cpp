/*  Copyright: Christian Feldmann (christian.feldmann@bitmovin.com)
 */

#include "VVDecPlayerApplication.h"

#include <ui/MainWindow.h>

#include <QApplication>
#include <QSettings>

#define APPLICATION_DEBUG 0
#if APPLICATION_DEBUG && !NDEBUG
#include <QDebug>
#define DEBUG_APP(msg) qDebug() << msg
#else
#define DEBUG_APP(msg) ((void)0)
#endif

VVDecPlayerApplication::VVDecPlayerApplication(int argc, char *argv[]) : QApplication(argc, argv)
{
  setApplicationName("vvDecPlayer");
  setApplicationVersion("0.1");
  setOrganizationName("Bitmovin");
  setOrganizationDomain("bitmovin.com");
#ifdef Q_OS_LINUX
#if QT_VERSION >= QT_VERSION_CHECK(5, 7, 0)
  QGuiApplication::setDesktopFileName("vvDecPlayer");
#endif
#endif

  auto args = arguments();
  DEBUG_APP("vvDecPlayer args" << args);

  MainWindow w;
  this->installEventFilter(&w);

  w.show();
  returnCode = exec();
}
