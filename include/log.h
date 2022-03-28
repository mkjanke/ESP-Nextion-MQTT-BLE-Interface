#ifndef LOG_H
#define LOG_H

#include "settings.h"
#include "TLog.h"

class Logger : public TLog {
  public:
    Logger() : TLog(){}

    void init();
    void send(const char *, bool=false);
    void send(String &, bool=false);

};

extern Logger sLog;

#endif // LOG_H