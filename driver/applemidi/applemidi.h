#ifndef MIDIKIT_DRIVER_APPLEMIDI_H
#define MIDIKIT_DRIVER_APPLEMIDI_H
#include <stdlib.h>
#include <sys/socket.h>
#include "midi/driver.h"
#include "midi/message.h"
#include "midi/util.h"

struct MIDIDriverAppleMIDI;

struct MIDIDriverAppleMIDI * MIDIDriverAppleMIDICreate( struct MIDIDriverDelegate * delegate, char * name, unsigned short port );
void MIDIDriverAppleMIDIDestroy( struct MIDIDriverAppleMIDI * driver );
void MIDIDriverAppleMIDIRetain( struct MIDIDriverAppleMIDI * driver );
void MIDIDriverAppleMIDIRelease( struct MIDIDriverAppleMIDI * driver );

int MIDIDriverAppleMIDISetPort( struct MIDIDriverAppleMIDI * driver, unsigned short port ); 
int MIDIDriverAppleMIDIGetPort( struct MIDIDriverAppleMIDI * driver, unsigned short * port ); 

int MIDIDriverAppleMIDIAcceptFromNone( struct MIDIDriverAppleMIDI * driver );
int MIDIDriverAppleMIDIAcceptFromAny( struct MIDIDriverAppleMIDI * driver );
int MIDIDriverAppleMIDIAcceptFromPeer( struct MIDIDriverAppleMIDI * driver, char * address, unsigned short port );


int MIDIDriverAppleMIDIAddPeerWithSockaddr( struct MIDIDriverAppleMIDI * driver, socklen_t size, struct sockaddr * addr );
int MIDIDriverAppleMIDIRemovePeerWithSockaddr( struct MIDIDriverAppleMIDI * driver, socklen_t size, struct sockaddr * addr );
int MIDIDriverAppleMIDIAddPeer( struct MIDIDriverAppleMIDI * driver, char * address, unsigned short port );
int MIDIDriverAppleMIDIRemovePeer( struct MIDIDriverAppleMIDI * driver, char * address, unsigned short port );

int MIDIDriverAppleMIDISetRTPSocket( struct MIDIDriverAppleMIDI * driver, int socket );
int MIDIDriverAppleMIDIGetRTPSocket( struct MIDIDriverAppleMIDI * driver, int * socket );
int MIDIDriverAppleMIDISetControlSocket( struct MIDIDriverAppleMIDI * driver, int socket );
int MIDIDriverAppleMIDIGetControlSocket( struct MIDIDriverAppleMIDI * driver, int * socket );

int MIDIDriverAppleMIDIConnect( struct MIDIDriverAppleMIDI * driver );
int MIDIDriverAppleMIDIDisconnect( struct MIDIDriverAppleMIDI * driver );

int MIDIDriverAppleMIDIReceiveMessage( struct MIDIDriverAppleMIDI * driver, struct MIDIMessage * message );
int MIDIDriverAppleMIDISendMessage( struct MIDIDriverAppleMIDI * driver, struct MIDIMessage * message );

int MIDIDriverAppleMIDIReceive( struct MIDIDriverAppleMIDI * driver );
int MIDIDriverAppleMIDISend( struct MIDIDriverAppleMIDI * driver );
int MIDIDriverAppleMIDIIdle( struct MIDIDriverAppleMIDI * driver );

int MIDIDriverAppleMIDIGetRunloopSource( struct MIDIDriverAppleMIDI * driver, struct MIDIRunloopSource ** source );

#endif