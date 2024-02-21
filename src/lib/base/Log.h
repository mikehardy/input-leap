/*
 * InputLeap -- mouse and keyboard sharing utility
 * Copyright (C) 2012-2016 Symless Ltd.
 * Copyright (C) 2002 Chris Schoeneman
 *
 * This package is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * found in the file LICENSE that should have accompanied this file.
 *
 * This package is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#pragma once

#include "Fwd.h"
#include "arch/IArchMultithread.h"
#include "arch/Arch.h"
#include "common/common.h"

#include <stdarg.h>
#include <list>
#include <mutex>

#define CLOG (Log::getInstance())
#define BYE "\nTry `%s --help' for more information."

namespace inputleap {

class Thread;

//! Logging facility
/*!
The logging class;  all console output should go through this class.
It supports multithread safe operation, several message priority levels,
filtering by priority, and output redirection.  The macros LOG() and
LOGC() provide convenient access.
*/
class Log {
public:
    Log();
    Log(Log* src);
    ~Log();

    //! @name manipulators
    //@{

    //! Add an outputter to the head of the list
    /*!
    Inserts an outputter to the head of the outputter list.  When the
    logger writes a message, it goes to the outputter at the head of
    the outputter list.  If that outputter's \c write() method returns
    true then it also goes to the next outputter, as so on until an
    outputter returns false or there are no more outputters.  Outputters
    still in the outputter list when the log is destroyed will be
    deleted.  If \c alwaysAtHead is true then the outputter is always
    called before all outputters with \c alwaysAtHead false and the
    return value of the outputter is ignored.

    By default, the logger has one outputter installed which writes to
    the console.
    */
    void insert(ILogOutputter* adopted, bool alwaysAtHead = false);

    //! Remove an outputter from the list
    /*!
    Removes the first occurrence of the given outputter from the
    outputter list.  It does nothing if the outputter is not in the
    list.  The outputter is not deleted.
    */
    void remove(ILogOutputter* orphaned);

    //! Remove the outputter from the head of the list
    /*!
    Removes and deletes the outputter at the head of the outputter list.
    This does nothing if the outputter list is empty.  Only removes
    outputters that were inserted with the matching \c alwaysAtHead.
    */
    void pop_front(bool alwaysAtHead = false);

    //! Set the minimum priority filter.
    /*!
    Set the filter.  Messages below this priority are discarded.
    The default priority is 4 (INFO) (unless built without NDEBUG
    in which case it's 5 (DEBUG)).   setFilter(const char*) returns
    true if the priority \c name was recognized;  if \c name is nullptr
    then it simply returns true.
    */
    bool setFilter(const char* name);

    //! Set the minimum priority filter (by ordinal).
    void setFilter(int);

    //@}
    //! @name accessors
    //@{

    //! Print a log message
    /*!
    Print a log message using the printf-like \c format and arguments
    preceded by the filename and line number.  If \c file is nullptr then
    neither the file nor the line are printed.
    */
    _X_ATTRIBUTE_PRINTF(4, 5)
    void print(const char* file, int line,
                            const char* format, ...);

    //! Get the minimum priority level.
    int getFilter() const;

    //! Get the filter name of the current filter level.
    const char* getFilterName() const;

    //! Get the filter name of a specified filter level.
    const char* getFilterName(int level) const;

    //! Get the singleton instance of the log
    static Log* getInstance();

    //! Get the console filter level (messages above this are not sent to console).
    int getConsoleMaxLevel() const { return kDEBUG2; }

    //@}

private:
    void output(ELevel priority, const char* msg);

private:
    typedef std::list<ILogOutputter*> OutputterList;

    static Log*        s_log;

    mutable std::mutex m_mutex;
    OutputterList m_outputters;
    OutputterList m_alwaysOutputters;
    int m_maxPriority;
};

/*!
\def LOG(arg)
Write to the log. This should be invoked like so:
\code
LOG(CLOG_XXX "%d and %d are %s", x, y, x == y ? "equal" : "not equal");
\endcode
Note that there is no comma after the \c CLOG_XXX.  The \c XXX should be
replaced by one of enumerants in \c Log::ELevel without the leading
\c k.  For example, \c CLOG_INFO.  The special \c CLOG_PRINT level will
not be filtered and is never prefixed by the filename and line number.

If \c NOLOGGING is defined during the build then this macro expands to
nothing.  If \c NDEBUG is defined during the build then it expands to a
call to Log::print.  Otherwise it expands to a call to Log::printt,
which includes the filename and line number.
*/

/*!
\def LOGC(expr, arg)
Write to the log if and only if expr is true. This should be invoked like so:
\code
LOGC(x == y, CLOG_XXX "%d and %d are equal", x, y);
\endcode
Note that there is no comma after the \c CLOG_XXX.
The \c XXX should be replaced by one of enumerants in \c Log::ELevel
without the leading \c k.  For example, \c CLOG_INFO.  The special
\c CLOG_PRINT level will not be filtered and is never prefixed by the
filename and line number.

If \c NOLOGGING is defined during the build then this macro expands to
nothing.  If \c NDEBUG is not defined during the build then it expands
to a call to Log::print that prints the filename and line number,
otherwise it expands to a call that doesn't.
*/

#if defined(NOLOGGING)
#define LOG(...)
#define LOGC(_a1, ...)
#elif defined(NDEBUG)
#define LOG(...)        CLOG->print(nullptr, 0, __VA_ARGS__)
#define LOGC(_a1, ...)    if (_a1) CLOG->print(nullptr, 0, __VA_ARGS__)
#else
#define LOG(...)        CLOG->print(__FILE__, __LINE__, __VA_ARGS__)
#define LOGC(_a1, ...)    if (_a1) CLOG->print(__FILE__, __LINE__, __VA_ARGS__)
#endif

// the CLOG_* defines %z and an octal number (060=0, 071=9),
// but the limitation is that once we run out of numbers at either
// end, then we resort to using non-numerical chars. this still works (since
// to deduce the number we subtract octal \060, so '/' is -1, and ':' is 10

#define CLOG_PRINT        "@z\057" // char is '/'
#define CLOG_CRIT        "@z\060" // char is '0'
#define CLOG_ERR        "@z\061"
#define CLOG_WARN        "@z\062"
#define CLOG_NOTE        "@z\063"
#define CLOG_INFO        "@z\064"
#define CLOG_DEBUG        "@z\065"
#define CLOG_DEBUG1        "@z\066"
#define CLOG_DEBUG2        "@z\067"
#define CLOG_DEBUG3        "@z\070"
#define CLOG_DEBUG4        "@z\071" // char is '9'
#define CLOG_DEBUG5        "@z\072" // char is ':'

} // namespace inputleap
