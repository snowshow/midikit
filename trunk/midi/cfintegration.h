#ifndef MIDIKIT_MIDI_CFINTEGRATION_H
#define MIDIKIT_MIDI_CFINTEGRATION_H
#include <CoreFoundation/CoreFoundation.h>
#if TARGET_OS_IPHONE
#include <CFNetwork/CFNetServices.h>
#else
#if TARGET_OS_MAC
#include <CoreServices/CoreServices.h>
#endif
#endif

#include "driver/applemidi/applemidi.h"

int MIDIDriverAppleMIDIAddPeerWithCFNetService( struct MIDIDriverAppleMIDI * driver,
                                                CFNetServiceRef netService );
int MIDIDriverAppleMIDIRemovePeerWithCFNetService( struct MIDIDriverAppleMIDI * driver,
                                                   CFNetServiceRef netService );

#include "midi/runloop.h"

#ifndef MIDI_RUNLOOP_INTERNALS
/**
 * When used as an opaque pointer type, an instance of
 * CFMIDIRunloop can be used as a MIDIRunloop.
 */
#define CFMIDIRunloop MIDIRunloop
#endif

struct CFMIDIRunloop;

struct CFMIDIRunloop * CFMIDIRunloopCreate( CFRunLoopRef runloop );

#endif
