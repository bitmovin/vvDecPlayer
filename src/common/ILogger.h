/*  Copyright: Christian Feldmann (christian.feldmann@bitmovin.com)
 */

#pragma once

#include <QString>

enum class LoggingPriority
{
  Error,
  Warning,
  Info
};

class ILogger
{
public:
  virtual void addMessage(QString message, LoggingPriority priority) = 0;
  virtual void clearMessages()                                       = 0;
};
