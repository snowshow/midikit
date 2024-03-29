#ifndef MIDIKIT_MIDI_DRIVER_H
#define MIDIKIT_MIDI_DRIVER_H
#include "midi.h"

struct MIDIPort;
struct MIDIEvent;
struct MIDIMessage;
struct MIDIRunloop;

struct MIDIDriver;

#define MIDI_DRIVER_WILL_SEND_MESSAGE 0
#define MIDI_DRIVER_WILL_RECEIVE_MESSAGE 1
#define MIDI_DRIVER_NUM_EVENT_TYPES 2

#ifdef MIDI_DRIVER_INTERNALS
struct MIDIRunloopSource;
struct MIDIClock;

struct MIDIDriver {
  size_t refs;
  struct MIDIRunloopSource * rls;
  struct MIDIPort * port;
  struct MIDIClock * clock;
  int (*send)( void * driver, struct MIDIMessage * message );
  void (*destroy)( void * driver );
};
#endif

struct MIDIDriver * MIDIDriverCreate( char * name, MIDISamplingRate rate );
void MIDIDriverInit( struct MIDIDriver * driver, char * name, MIDISamplingRate rate );
void MIDIDriverDestroy( struct MIDIDriver * driver );
void MIDIDriverRetain( struct MIDIDriver * driver );
void MIDIDriverRelease( struct MIDIDriver * driver );

int MIDIDriverMakeLoopback( struct MIDIDriver * driver );

int MIDIDriverGetPort( struct MIDIDriver * driver, struct MIDIPort ** port );

int MIDIDriverSend( struct MIDIDriver * driver, struct MIDIMessage * message );
int MIDIDriverReceive( struct MIDIDriver * driver, struct MIDIMessage * message );
int MIDIDriverTriggerEvent( struct MIDIDriver * driver, struct MIDIEvent * event );

int MIDIDriverStartProfiling( struct MIDIDriver * driver );
int MIDIDriverGetProfilingStats( struct MIDIDriver * driver, void * stats );
int MIDIDriverStopProfiling( struct MIDIDriver * driver );

int MIDIRunloopAddDriver( struct MIDIRunloop * runloop, struct MIDIDriver * driver );
int MIDIRunloopRemoveDriver( struct MIDIRunloop * runloop, struct MIDIDriver * driver );

#endif
