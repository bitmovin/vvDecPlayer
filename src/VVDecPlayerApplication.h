/*  Copyright: Christian Feldmann (christian.feldmann@bitmovin.com)
*/

#pragma once

#include <QApplication>

class VVDecPlayerApplication : public QApplication
{
  Q_OBJECT
public:
  VVDecPlayerApplication(int argc, char *argv[]);
  int returnCode{0};
};
