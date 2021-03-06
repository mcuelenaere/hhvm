/*
   +----------------------------------------------------------------------+
   | HipHop for PHP                                                       |
   +----------------------------------------------------------------------+
   | Copyright (c) 2010-2014 Facebook, Inc. (http://www.facebook.com)     |
   | Copyright (c) 1997-2010 The PHP Group                                |
   +----------------------------------------------------------------------+
   | This source file is subject to version 3.01 of the PHP license,      |
   | that is bundled with this package in the file LICENSE, and is        |
   | available through the world-wide-web at the following url:           |
   | http://www.php.net/license/3_01.txt                                  |
   | If you did not receive a copy of the PHP license and are unable to   |
   | obtain it through the world-wide-web, please send a note to          |
   | license@php.net so we can mail you a copy immediately.               |
   +----------------------------------------------------------------------+
*/

#ifndef incl_HPHP_EXT_XENON_H_
#define incl_HPHP_EXT_XENON_H_

#include "hphp/runtime/base/base-includes.h"
#include <semaphore.h>

/*
                        Project Xenon
  What is it?
  The name Xenon is from the type of bulb used in strobelights.  The idea is
  that a timer goes off and the code takes a snapshot of the php and async
  stack at that time (ie it flashes the code).

  How does it work?
  There are two ways for Xenon to work: 1) always on, 2) via timer.
  For the timer mode:  Xenon appends a timer to the already existing SIGVTALRM
  handler.  When that timer fires it sets a semaphore so that others may
  know.  We'd like to be able to record the status of every stack for every
  thread at this point, but during a timer handler is not a reasonable place
  to do this.
  Instead, Xenon has a pthread waiting for the semaphore.  When the semaphore
  is set in the handler, it wakes, sets the Xenon Surprise flag for every
  thread - this is the flash.
  There are is a mechanism that hooks the enter/exit of every PHP function,
  using the Surprise flags leverages that mechanism and calls logging.
  At this point, if the flag is set for this thread, the PHP and async stack
  will be logged for that thread.

  The for the always on mode:  when execute_command_line_begin is called,
  all of the threads are Surprised.  When the Xenon methods are invoked in
  this mode, the Xenon Surprise flags are not cleared, so functions will always
  be Surprised on enter and exit.  In this mode, no threads or semaphores or
  timers are created, since they are not needed.

  How do I use it?
  Enabling it via Xenon Period runtime option will start Xenon snapping and
  gathering data.  There is a PHP extension, xenon_get_data, that returns
  all of the data gather for that request/thread.  Period is a double that
  represents seconds (ie 0.1 is a ten of a second).
  Once can alternatively set Xenon ForceAlways=true, which will then
  gather PHP and async stacks at every function enter/exit.  This will
  gather potentially large amounts of data - mostly used for small
  script debugging.
*/

namespace HPHP {
class Xenon {
  public:

    enum SampleType {
      IOWaitSample,  // sample was taken during a function that is wait time
      EnterSample,   // sample was during CPU time at the start of a function
      ExitSample,    // sample was during CPU time at the end of a function
    };

    static Xenon& getInstance(void);

    Xenon();
    ~Xenon();
    Xenon(Xenon const&) = delete;
    void operator=(Xenon const&) = delete;

    void start(uint64_t msec);
    void stop();
    void log(SampleType t);
    void surpriseAll();
    void onTimer();

    bool      m_stopping;
  private:
    sem_t     m_timerTriggered;
    pthread_t m_triggerThread;
#ifndef __APPLE__
    timer_t   m_timerid;
#endif
};
}

#endif // incl_HPHP_EXT_XENON_H_
