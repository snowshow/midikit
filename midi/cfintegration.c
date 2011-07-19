#ifdef __APPLE__
#include <arpa/inet.h>

#define MIDI_RUNLOOP_INTERNALS
#include "midi.h"
#include "cfintegration.h"
#include "driver.h"

static int _netservice_get_first_addr( CFNetServiceRef netService, socklen_t * size, struct sockaddr ** addr ) {
  CFArrayRef addresses;
  CFDataRef data;
  CFNetServiceCancel( netService );
  CFNetServiceResolveWithTimeout( netService, 0.1, NULL );
  addresses = CFNetServiceGetAddressing( netService );
  if( addresses == NULL || CFArrayGetCount( addresses ) < 1 ) {
    return 1;
  }
  data  = CFArrayGetValueAtIndex( addresses, 0 );
  *size = CFDataGetLength( data );
  *addr = CFDataGetBytePtr( data );
  
  return 0;
}

int MIDIDriverAppleMIDIAddPeerWithCFNetService( struct MIDIDriverAppleMIDI * driver, CFNetServiceRef netService ) {
  socklen_t size;
  struct sockaddr * addr;
  if( _netservice_get_first_addr( netService, &size, &addr ) ) return 1;
  
  return MIDIDriverAppleMIDIAddPeerWithSockaddr( driver, size, addr );
}

int MIDIDriverAppleMIDIRemovePeerWithCFNetService( struct MIDIDriverAppleMIDI * driver, CFNetServiceRef netService ) {
  socklen_t size;
  struct sockaddr * addr;
  if( _netservice_get_first_addr( netService, &size, &addr ) ) return 1;
  
  return MIDIDriverAppleMIDIRemovePeerWithSockaddr( driver, size, addr );
}

/* MARK: -
 * MARK: CFMIDIRunloop */

struct CFMIDIRunloop {
  struct MIDIRunloop    base; 
  CFRunLoopRef          runloop;
  CFRunLoopTimerContext timer_context;
  CFSocketContext       socket_context;
  CFMutableSetRef cf_timers;
  CFMutableSetRef cf_sockets;
  CFMutableSetRef cf_sources;
};

/* MARK: Core Foundation callbacks *//**
 * @name Core Foundation callbacks
 * @cond INTERNAL
 * Methods that connect the core foundation runloop events to the
 * internal queue.
 * @{
 */
 
static const void * _cf_retain_callback( const void * runloop ) {
  /* struct CFMIDIRunloop * cf_runloop = runloop; */
  MIDIRunloopRetain( runloop );
  return runloop;
}

static void _cf_release_callback( const void * runloop ) {
  /*struct CFMIDIRunloop * cf_runloop = runloop;*/
  MIDIRunloopRelease( runloop );
}

static void _cf_socket_callback( CFSocketRef s, CFSocketCallBackType callbackType, CFDataRef address, const void *data, void *info ) {
  struct CFMIDIRunloop * cf_runloop = info;
  struct MIDIRunloopSource * source = &(cf_runloop->base.master);
  int i = CFSocketGetNative( s );
  fd_set fds;
  FD_ZERO(&fds);
  FD_SET(i, &fds);
  switch( callbackType ) {
    case kCFSocketReadCallBack:
      if( FD_ISSET(i, &(source->readfds)) && source->delegate.read != NULL ) {
        (*source->delegate.read)( source->delegate.info, source->nfds, &(fds) );
      }
      break;
    case kCFSocketWriteCallBack:
      if( FD_ISSET(i, &(source->writefds)) && source->delegate.write != NULL ) {
        (*source->delegate.write)( source->delegate.info, source->nfds, &(fds) );
      }
      break;
  }
}

static void _cf_timer_callback( CFRunLoopTimerRef timer, void *info ) {
  struct CFMIDIRunloop * cf_runloop = info;
  struct MIDIRunloopSource * source = &(cf_runloop->base.master);
  if( source->delegate.timeout != NULL ) {
    (*source->delegate.timeout)( source->delegate.info, &(source->timeout_time) );
  }
}

struct CFRunLoopTimerTimespec {
  double timeout;
  CFRunLoopTimerRef timer;
};

#define CF_INTEGRATION_TIMEOUT_DELTA 0.001
static void _cf_runloop_timer_timeout_applier( const void * value, void * context ) {
  CFRunLoopTimerRef timer = value;
  struct CFRunLoopTimerTimespec * t = context;
  double timeout = CFRunLoopTimerGetInterval( timer );
  if( timeout + CF_INTEGRATION_TIMEOUT_DELTA > t->timeout
   && timeout - CF_INTEGRATION_TIMEOUT_DELTA < t->timeout ) {
    t->timer = timer;
  }
}
#undef CF_INTEGRATION_TIMEOUT_DELTA

struct CFSocketNative {
  int fd;
  CFSocketRef socket;
};

static void _cf_socket_native_applier( const void * value, void * context ) {
  CFSocketRef socket = value;
  struct CFSocketNative * s = context;
  int native = CFSocketGetNative( socket );
  if( s->fd == native ) {
    s->socket = socket;
  }
}

static void _cf_invalidate_timer_applier( const void * value, void * context ) {
  struct CFMIDIRunloop * cf_runloop = context;
  CFRunLoopTimerRef timer = value;
  CFRunLoopTimerInvalidate( timer );
  if( CFRunLoopContainsTimer( cf_runloop->runloop, timer, kCFRunLoopCommonModes ) ) {
    CFRunLoopRemoveTimer( cf_runloop->runloop, timer, kCFRunLoopCommonModes );
  }
}

static void _cf_disable_socket_callbacks_applier( const void * value, void * context ) {
  CFSocketDisableCallBacks( value, kCFSocketReadCallBack | kCFSocketWriteCallBack );
}

static void _cf_invalidate_source_applier( const void * value, void * context ) {
  struct CFMIDIRunloop * cf_runloop = context;
  CFRunLoopSourceRef source = value;
  CFRunLoopSourceInvalidate( source );
  if( CFRunLoopContainsSource( cf_runloop->runloop, source, kCFRunLoopCommonModes ) ) {
    CFRunLoopRemoveSource( cf_runloop->runloop, source, kCFRunLoopCommonModes );
  }
}

/**
 * @}
 * @endcond
 */

/* MARK: Helper functions *//**
 * @name Helper functions
 * @cond INTERNAL
 * @{
 */

/**
 * Given the runloop and a timeout find the CFRunLoopTimerRef
 * with the interval close to timeout.
 * @private @memberof CFMIDIRunloop
 * @param cf_runloop the runloop.
 * @param timeout    the timeout.
 * @return a CFRunLoopTimerRef that is close to @c timeout or @c NULL if no
 *         interval close to the given timeout was created.
 */
static CFRunLoopTimerRef _cf_runloop_find_cf_runloop_timer( struct CFMIDIRunloop * cf_runloop, struct timespec * timeout ) {
  struct CFRunLoopTimerTimespec t;
  t.timeout = (double) timeout->tv_sec + 0.000000001 * (double) timeout->tv_nsec;
  t.timer   = NULL;
  CFSetApplyFunction( cf_runloop->cf_timers, &_cf_runloop_timer_timeout_applier, &t );
  if( t.timer != NULL ) CFRetain( t.timer );
  return t.timer;
}

/**
 * Provide a runloop timer with closely the given timeout.
 * @private @memberof CFMIDIRunloop
 * @param cf_runloop the runloop.
 * @param fd         the filedescriptor.
 * @return a CFRunLoopTimerRef that is close to @c timeout.
 */
static CFRunLoopTimerRef _cf_runloop_provide_cf_runloop_timer( struct CFMIDIRunloop * cf_runloop, struct timespec * timeout ) {
  CFRunLoopTimerRef timer;
  
  timer = _cf_runloop_find_cf_runloop_timer( cf_runloop, timeout );
  if( timer == NULL ) {
    timer = CFRunLoopTimerCreate( NULL, CFAbsoluteTimeGetCurrent(),
      (double) timeout->tv_sec + 0.000000001 * (double) timeout->tv_nsec,
      0, 1, &_cf_timer_callback, &(cf_runloop->timer_context) );
    CFRunLoopAddTimer( cf_runloop->runloop, timer, kCFRunLoopCommonModes );
    CFSetAddValue( cf_runloop->cf_timers, timer );
  }
  return timer;
}

/**
 * Given the runloop and a socket number find the CFSocketRef
 * with the native socket number @c fd.
 * @private @memberof CFMIDIRunloop
 * @param cf_runloop the runloop.
 * @param fd         the filedescriptor.
 * @return the CFSocketRef that matches @c fd or @c NULL if no
 *         socket with the given number was created.
 */
static CFSocketRef _cf_runloop_find_cf_socket( struct CFMIDIRunloop * cf_runloop, int fd ) {
  struct CFSocketNative s;
  s.fd     = fd;
  s.socket = NULL;
  CFSetApplyFunction( cf_runloop->cf_sockets, &_cf_socket_native_applier, &s );
  if( s.socket != NULL ) CFRetain( s.socket );
  return s.socket;
}

/**
 * Provide a socket with the given native socket number @c fd.
 * If the socket was not yet created (within this runloop) create it
 * and schedule it.
 * @private @memberof CFMIDIRunloop
 * @param cf_runloop the runloop.
 * @param fd         the filedescriptor.
 * @return a CFSocketRef thet matches @c fd.
 */
static CFSocketRef _cf_runloop_provide_cf_socket( struct CFMIDIRunloop * cf_runloop, int fd ) {
  CFSocketRef        socket;
  CFRunLoopSourceRef source;
  
  socket = _cf_runloop_find_cf_socket( cf_runloop, fd );
  if( socket == NULL ) {
    socket = CFSocketCreateWithNative( NULL, fd, kCFSocketReadCallBack | kCFSocketWriteCallBack,
                                       &_cf_socket_callback, &(cf_runloop->socket_context) );
    source = CFSocketCreateRunLoopSource( NULL, socket, 1 );
    if( !CFSetContainsValue( cf_runloop->cf_sources, source ) ) {
      CFSetAddValue( cf_runloop->cf_sources, source );
      CFRunLoopAddSource( cf_runloop->runloop, source, kCFRunLoopCommonModes );
    }
    CFRelease( source );
    CFSetAddValue( cf_runloop->cf_sockets, socket );
  }

  return socket;
}

/**
 * Enable a given callback for the socket @c fd.
 * Create and schedule the socket if it is not yet scheduled.
 * @param cf_runloop the runloop.
 * @param fd         the filedescriptor.
 */
static int _cf_runloop_socket_enable_callback( struct CFMIDIRunloop * cf_runloop, int fd, CFSocketCallBackType type ) {
  CFSocketRef socket;
  MIDIAssert( cf_runloop != NULL );

  socket = _cf_runloop_provide_cf_socket( cf_runloop, fd );
  CFSocketEnableCallBacks( socket, type );  
  CFRelease( socket );
  return 0;
}  

/**
 * Disable a given callback for the socket @c fd.
 * Do nothing if the socket is not scheduled in this runloop.
 * @param cf_runloop the runloop.
 * @param fd         the filedescriptor.
 */
static int _cf_runloop_socket_disable_callback( struct CFMIDIRunloop * cf_runloop, int fd, CFSocketCallBackType type ) {
  CFSocketRef socket;
  MIDIAssert( cf_runloop != NULL );

  socket = _cf_runloop_find_cf_socket( cf_runloop, fd );
  if( socket ) {
    CFSocketDisableCallBacks( socket, type );  
    CFRelease( socket );
  }
  return 0;
}  

/**
 * @}
 * @endcond
 */

/* MARK: Runloop callbacks *//**
 * @name Runloop callbacks
 * @cond INTERNAL
 * Methods that are part of the runloop delegate to schedule and unschedule (clear)
 * actions.
 * @{
 */

static int _cf_runloop_schedule_read( void * info, int fd ) {
  MIDILog( DEBUG, "CoreFoundation integration: Schedule read (%i)\n", fd );
  return _cf_runloop_socket_enable_callback( info, fd, kCFSocketReadCallBack );
}

static int _cf_runloop_clear_read( void * info, int fd ) {
  MIDILog( DEBUG, "CoreFoundation integration: Clear read (%i)\n", fd );
  return _cf_runloop_socket_disable_callback( info, fd, kCFSocketReadCallBack );
}

static int _cf_runloop_schedule_write( void * info, int fd ) {
  MIDILog( DEBUG, "CoreFoundation integration: Schedule write (%i)\n", fd );
  return _cf_runloop_socket_enable_callback( info, fd, kCFSocketWriteCallBack );
}

static int _cf_runloop_clear_write( void * info, int fd ) {
  MIDILog( DEBUG, "CoreFoundation integration: Clear write (%i)\n", fd );
  return _cf_runloop_socket_disable_callback( info, fd, kCFSocketWriteCallBack );
}

static int _cf_runloop_schedule_timeout( void * info, struct timespec * timeout ) {
  struct CFMIDIRunloop * cf_runloop = info;
  CFRunLoopTimerRef timer;
  MIDIAssert( cf_runloop != NULL );
  MIDILog( DEBUG, "CoreFoundation integration: Schedule timeout\n" );
  timer = _cf_runloop_provide_cf_runloop_timer( cf_runloop, timeout );
  return (timer != NULL);
}

static int _cf_runloop_clear_timeout( void * info ) {
  struct CFMIDIRunloop * runloop = info;
  MIDIAssert( runloop != NULL );
  MIDILog( DEBUG, "CoreFoundation integration: Clear timeout\n" );
  return 0;
}

/**
 * @}
 * @endcond
 */

/* MARK: -
 * MARK: Creation and destruction *//**
 * @name Creation and destruction
 * Creating, destroying and reference counting of MIDIClock objects.
 * @{
 */

void CFMIDIRunloopDestroy( struct CFMIDIRunloop * cf_runloop );

struct CFMIDIRunloop * CFMIDIRunloopCreate( CFRunLoopRef runloop ) {
  struct CFMIDIRunloop * cf_runloop = malloc( sizeof( struct CFMIDIRunloop ) );
  MIDIPrecondReturn( cf_runloop != NULL, ENOMEM, NULL );

  MIDIRunloopInit( &(cf_runloop->base) );  
  cf_runloop->runloop = runloop;
  
  cf_runloop->timer_context.version = 0;
  cf_runloop->timer_context.info = cf_runloop;
  cf_runloop->timer_context.release = &_cf_release_callback;
  cf_runloop->timer_context.retain  = &_cf_retain_callback;
  cf_runloop->timer_context.copyDescription = NULL;

  cf_runloop->socket_context.version = 0;
  cf_runloop->socket_context.info = cf_runloop;
  cf_runloop->socket_context.release = &_cf_release_callback;
  cf_runloop->socket_context.retain  = &_cf_retain_callback;
  cf_runloop->socket_context.copyDescription = NULL;
  
  cf_runloop->cf_timers  = CFSetCreateMutable( NULL, 0, &kCFTypeSetCallBacks );
  cf_runloop->cf_sockets = CFSetCreateMutable( NULL, 0, &kCFTypeSetCallBacks );
  cf_runloop->cf_sources = CFSetCreateMutable( NULL, 0, &kCFTypeSetCallBacks );

  cf_runloop->base.schedule_read    = &_cf_runloop_schedule_read;
  cf_runloop->base.schedule_write   = &_cf_runloop_schedule_write;
  cf_runloop->base.schedule_timeout = &_cf_runloop_schedule_timeout;
  cf_runloop->base.clear_read    = &_cf_runloop_clear_read;
  cf_runloop->base.clear_write   = &_cf_runloop_clear_write;
  cf_runloop->base.clear_timeout = &_cf_runloop_clear_timeout;
  cf_runloop->base.destroy = &CFMIDIRunloopDestroy;
  return cf_runloop;
}

void CFMIDIRunloopDestroy( struct CFMIDIRunloop * cf_runloop ) {
  MIDIPrecondReturn( cf_runloop, EFAULT, (void)0 );
  
  CFSetApplyFunction( cf_runloop->cf_timers, &_cf_invalidate_timer_applier, NULL );
  CFSetApplyFunction( cf_runloop->cf_sockets, &_cf_disable_socket_callbacks_applier, NULL );
  CFSetApplyFunction( cf_runloop->cf_sources, &_cf_invalidate_source_applier, NULL );

  CFRelease( cf_runloop->cf_timers );
  CFRelease( cf_runloop->cf_sockets );
  CFRelease( cf_runloop->cf_sources );
  free( cf_runloop );
}

/** @} */

#endif