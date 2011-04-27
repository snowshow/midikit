#include <stdio.h>
#include <stdarg.h>
#include "midi.h"

/**
 * @defgroup MIDI MIDI
 */

int MIDIErrorNumber = 0;

#ifdef DEBUG

static int _midi_err_log( const char * fmt, ... ) {
   int result;
   va_list ap;
   va_start( ap, fmt );
   result = vfprintf( stderr, fmt, ap );
   va_end( ap );
   fflush( stderr );
   return result;
}

MIDILogFunction MIDILogger = &_midi_err_log;
#else
MIDILogFunction MIDILogger = NULL;
#endif

/**
 * @def MIDIPrecond
 * @brief Check preconditions of a method.
 * Use this macro to check your preconditions inside a function.
 * Those preconditions may be specific values of arguments, or
 * checks if a malloc succeeded. Preconditions may also be used
 * to check if some piece of code should continue it's operation.
 * A failed precondition is a program error that was triggered by
 * misuse of the API or by the an external error. When the precondition
 * fails the function returns immediately.
 * @param expr The expression to test.
 * @param kind The symbolic error number.
 */

/**
 * @def MIDIPrecondReturn
 * @brief Check preconditions of a method and return a specified value.
 * Use this macro to check your preconditions inside a function.
 * Unlike @c MIDIPrecond this macro allows you to specify the return
 * value if the precondition was violated.
 * @param expr   The expression to test.
 * @param kind   The symbolic error number.
 * @param retval The value that should be returned on error.
 */

/**
 * @def MIDIAssert
 * @brief Check for programming errors.
 * Use this macro to check for programming errors. Inside a
 * function you may assume various conditions that should be
 * fulfilled if you programmed your code correctly. For example
 * if you have numerous static functions in you C code and use
 * them to solve recurring tasks, these functions are only
 * called by you, never by a client. To check if you use those
 * functions correctly you can use assertions inside those
 * functions. Assertions check for errors that should never
 * happen. Failed assertions indicate a programming error and
 * therefore result in the program exiting immediately.
 * Use many assertions, cover all implicit knowledge with them,
 * there is no penalty, because they will be removed in
 * release mode.
 * @param expr The expression to test.
 */

/**
 * @def MIDIError
 * @brief Indicate that an error occured.
 * Normal code execution will continue, but the MIDI error
 * number will be set and a message will be logged.
 * @param kind The symbolic error number.
 * @param msg  The log message.
 */

/**
 * @def MIDILog
 * @brief Send a message to the MIDI logger.
 * @param channels The log channels to use for logging.
 * @param ...      The log message format and variable arguments.
 */
