#ifndef MIDIKIT_MIDI_RUNLOOP_H
#define MIDIKIT_MIDI_RUNLOOP_H
#include <sys/select.h>

#define MIDI_RUNLOOP_READ       1
#define MIDI_RUNLOOP_WRITE      2
#define MIDI_RUNLOOP_IDLE       4
#define MIDI_RUNLOOP_INVALIDATE 8

struct MIDIRunloopSource;
struct MIDIRunloop;

struct MIDIRunloopSourceDelegate {
  void *info;
  int (*read)( void * info, int nfds, fd_set * readfds );
  int (*write)( void * info, int nfds, fd_set * readfds );
  int (*timeout)( void * info, struct timespec * elapsed );
};

struct MIDIRunloopDelegate {
  void *info;
  int (*schedule_read)( void * info, int fd );
  int (*schedule_write)( void * info, int fd );
  int (*schedule_timeout)( void * info, struct timespec * );
  int (*clear_read)( void * info, int fd );
  int (*clear_write)( void * info, int fd );
  int (*clear_timeout)( void * info );
};

#ifdef MIDI_RUNLOOP_INTERNALS
#define MAX_RUNLOOP_SOURCES 16

struct MIDIRunloopSource {
  int    refs;
  int    nfds;
  fd_set readfds;
  fd_set writefds;
  struct timespec timeout_start;
  struct timespec timeout_time;
  struct MIDIRunloopSourceDelegate delegate;
  struct MIDIRunloop * runloop;
};

struct MIDIRunloop {
  int    refs;
  int    active;
  struct MIDIRunloopDelegate delegate;
  struct MIDIRunloopSource   master;
  struct MIDIRunloopSource * sources[MAX_RUNLOOP_SOURCES];
  int (*schedule_read)( void * runloop, int fd );
  int (*schedule_write)( void * runloop, int fd );
  int (*schedule_timeout)( void * runloop, struct timespec * );
  int (*clear_read)( void * runloop, int fd );
  int (*clear_write)( void * runloop, int fd );
  int (*clear_timeout)( void * runloop );
  void (*destroy)( void * runloop );
};
#endif

struct MIDIRunloopSource * MIDIRunloopSourceCreate( struct MIDIRunloopSourceDelegate * delegate );
void MIDIRunloopSourceDestroy( struct MIDIRunloopSource * source );
void MIDIRunloopSourceRetain( struct MIDIRunloopSource * source );
void MIDIRunloopSourceRelease( struct MIDIRunloopSource * source );

int MIDIRunloopSourceInvalidate( struct MIDIRunloopSource * source );
int MIDIRunloopSourceWait( struct MIDIRunloopSource * source );

int MIDIRunloopSourceScheduleRead( struct MIDIRunloopSource * source, int fd );
int MIDIRunloopSourceClearRead( struct MIDIRunloopSource * source, int fd );
int MIDIRunloopSourceScheduleWrite( struct MIDIRunloopSource * source, int fd );
int MIDIRunloopSourceClearWrite( struct MIDIRunloopSource * source, int fd );
int MIDIRunloopSourceScheduleTimeout( struct MIDIRunloopSource * source, struct timespec * timeout );
int MIDIRunloopSourceClearTimeout( struct MIDIRunloopSource * source );

int MIDIRunloopSetGlobalRunloop( struct MIDIRunloop * runloop );
int MIDIRunloopGetGlobalRunloop( struct MIDIRunloop ** runloop );

struct MIDIRunloop * MIDIRunloopCreate();
void MIDIRunloopInit( struct MIDIRunloop * runloop );
void MIDIRunloopDestroy( struct MIDIRunloop * runloop );
void MIDIRunloopRetain( struct MIDIRunloop * runloop );
void MIDIRunloopRelease( struct MIDIRunloop * runloop );

int MIDIRunloopAddSource( struct MIDIRunloop * runloop, struct MIDIRunloopSource * source );
int MIDIRunloopRemoveSource( struct MIDIRunloop * runloop, struct MIDIRunloopSource * source );

int MIDIRunloopStart( struct MIDIRunloop * runloop );
int MIDIRunloopStop( struct MIDIRunloop * runloop );
int MIDIRunloopStep( struct MIDIRunloop * runloop );


#endif
