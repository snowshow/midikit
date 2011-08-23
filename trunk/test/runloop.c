#include "test.h"
#include "midi/runloop.h"
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <string.h>

struct MIDIRunloopTestSource {
  int send_sock;
  int recv_sock;
  struct sockaddr_in send_addr;
  struct sockaddr_in recv_addr;
  struct timespec time;
  struct MIDIRunloopSource * source;
  int send_data;
  int recv_data;
};

struct MIDIRunloopTestSource _rls_info = {
  0, 0, {}, {}, {}, NULL, 0, 0
};


int _rls_read_fds( void * info, int nfds, fd_set * fds ) {
  struct MIDIRunloopTestSource * rls_info = info;
  ssize_t r;
  
  ASSERT_EQUAL( rls_info, &_rls_info, "Received wrong info pointer." );
  ASSERT_GREATER( nfds, rls_info->recv_sock, "Set is too short." );
  ASSERT_GREATER( FD_ISSET(rls_info->recv_sock, fds), 0, "Expected socket missing from set." );
  
  r = recv( rls_info->recv_sock, &(rls_info->recv_data), sizeof(rls_info->recv_data), 0 );
  
  printf( "Scheduled read, received %i.\n", rls_info->recv_data );
  
  ASSERT_EQUAL( r, sizeof(rls_info->recv_data), "Could not recv data." );
  ASSERT_EQUAL( rls_info->recv_data, rls_info->send_data, "Received wrong data." );
  
  ASSERT_NO_ERROR( MIDIRunloopSourceClearRead( rls_info->source, rls_info->recv_sock ), 
                   "Could not clear read." );  
  ASSERT_NO_ERROR( MIDIRunloopSourceScheduleTimeout( rls_info->source, &(rls_info->time) ), 
                   "Could not schedule timeout." );
  return 0;
}

int _rls_write_fds( void * info, int nfds, fd_set * fds ) {
  struct MIDIRunloopTestSource * rls_info = info;
  ssize_t s;
  
  ASSERT_EQUAL( rls_info, &_rls_info, "Received wrong info pointer." );
  ASSERT_GREATER( nfds, rls_info->send_sock, "Set is too short." );
  ASSERT_GREATER( FD_ISSET(rls_info->send_sock, fds), 0, "Expected socket missing from set." );
  
  printf( "Scheduled write, sending %i.\n", rls_info->send_data );
  
  s = sendto( rls_info->send_sock, &(rls_info->send_data), sizeof(rls_info->send_data), 0,
              (struct sockaddr *) &(rls_info->send_addr), sizeof(rls_info->send_addr) );

  ASSERT_EQUAL( s, sizeof(rls_info->send_data), "Could not send data." );
  ASSERT_NO_ERROR( MIDIRunloopSourceClearWrite( rls_info->source, rls_info->send_sock ), 
                   "Could not clear write." );  
  ASSERT_NO_ERROR( MIDIRunloopSourceScheduleRead( rls_info->source, rls_info->recv_sock ), 
                   "Could not schedule read." );

  return 0;
}

int _rls_timeout( void * info, struct timespec * ts ) {
  struct MIDIRunloopTestSource * rls_info = info;

  rls_info->send_data++;
  
  ASSERT_EQUAL( rls_info, &_rls_info, "Received wrong info pointer." );

  printf( "Timeout, schedule write of %i.\n", rls_info->send_data );
  
  ASSERT_NO_ERROR( MIDIRunloopSourceClearTimeout( rls_info->source ), 
                   "Could not clear timeout." );
 
  ASSERT_NO_ERROR( MIDIRunloopSourceScheduleWrite( rls_info->source, rls_info->send_sock ),
                   "Could not schedule write." );
  return 0;
}
 
struct MIDIRunloopSourceDelegate _rls_delegate = {
  &_rls_info,
  &_rls_read_fds,
  &_rls_write_fds,
  &_rls_timeout
};

/**
 * Test that the runloop works.
 */
int test001_runloop( void ) {
  struct MIDIRunloop * runloop = MIDIRunloopCreate();
  struct MIDIRunloopSource * source = MIDIRunloopSourceCreate( &_rls_delegate );
  
  ASSERT_NOT_EQUAL( runloop, NULL, "Could not create runloop." );
  ASSERT_NOT_EQUAL( source,  NULL, "Could not create runloop source." );
  
  ASSERT_NO_ERROR( MIDIRunloopAddSource( runloop, source ),
                   "Could not schedule source in runloop." );

  memset( &(_rls_info.send_addr), 0, sizeof(_rls_info.send_addr) );
  memset( &(_rls_info.recv_addr), 0, sizeof(_rls_info.recv_addr) );

  _rls_info.send_addr.sin_family      = AF_INET;
  _rls_info.send_addr.sin_addr.s_addr = INADDR_LOOPBACK;
  _rls_info.send_addr.sin_port        = 9500;
  
  inet_aton( "127.0.0.1", &(_rls_info.send_addr.sin_addr) );
  
  _rls_info.recv_addr.sin_family      = AF_INET;
  _rls_info.recv_addr.sin_addr.s_addr = INADDR_ANY;
  _rls_info.recv_addr.sin_port        = 9500;
  
  _rls_info.time.tv_sec  = 0;
  _rls_info.time.tv_nsec = 500000;
  
  _rls_info.send_sock = socket( PF_INET, SOCK_DGRAM, 0 );
  _rls_info.recv_sock = socket( PF_INET, SOCK_DGRAM, 0 );
  _rls_info.source = source;
  
  ASSERT_NOT_EQUAL( _rls_info.send_sock, 0, "Could not create test send socket." );
  ASSERT_NOT_EQUAL( _rls_info.recv_sock, 0, "Could not create test recv socket." );
  ASSERT_NO_ERROR( bind( _rls_info.recv_sock, (struct sockaddr *) &(_rls_info.recv_addr),
                         sizeof(_rls_info.recv_addr) ), "Could not bind test recv socket." );

  ASSERT_NO_ERROR( MIDIRunloopSourceScheduleTimeout( source, &(_rls_info.time) ), 
                   "Could not schedule timeout." );

  while( _rls_info.recv_data < 20 ) {
    printf( "Step\n" );
    fflush( stdout );
    ASSERT_NO_ERROR( MIDIRunloopStep( runloop ), "Could not step through runloop." );
  }
  
  MIDIRunloopSourceRelease( source );
  MIDIRunloopRelease( runloop );
  close( _rls_info.send_sock );
  close( _rls_info.recv_sock );
  return 0;
}
