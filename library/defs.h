/* 
 * File:   defs.h
 * Author: adrian
 *
 * Created on 13 July 2013, 16:31
 */

#ifndef DEFS_H
#define	DEFS_H

#include <string>

namespace Choonz
{
    extern const char * const   COMMAND_NONE;
    extern const char * const   COMMAND_LIBRARY;        // A command requesting the entire library, follwed by a second parameter to determine FIL, ALBUM, or ARTIST view
    extern const char * const   COMMAND_PLAY_FILE;      // Adds a file to the play list
    extern const char * const   COMMAND_LOCATION;       // Add a location to be scanned for tracks
    extern const char * const   COMMAND_IDENTIFY;       // Send a track for identification at the Echo Nest
    
    extern const char * const   PARAM_FILE_VIEW;
    extern const char * const   PARAM_ALBUM_VIEW;
    extern const char * const   PARAM_ARTIST_VIEW;
    extern const char * const   PARAM_SUCCESS;
    
    extern const char * const   RESPONSE_BEGIN;
    extern const char * const   RESPONSE_END;
    
    const unsigned TAG_TITLE(0);
    const unsigned TAG_ARTIST(1);
    const unsigned TAG_ALBUM(2);
    const unsigned TAG_GENRE(3);
    const unsigned TAG_YEAR(4);
    const unsigned TAG_TRACK(5);
    const unsigned TAG_LENGTH(6);
    const unsigned TAG_BITRATE(7);
    const unsigned TAG_SAMPLERATE(8);
    const unsigned TAG_CHANNELS(9);
    const unsigned TAG_COMMENT(10);
    const unsigned TAG_MAX(11);
    
    const unsigned SAVE_TITLE(0);
    const unsigned SAVE_ARTIST(1);
    const unsigned SAVE_ALBUM(2);
    const unsigned SAVE_GENRE(3);
    const unsigned SAVE_YEAR(4);
    const unsigned SAVE_TRACK(5);
    const unsigned SAVE_COMMENT(6);
    
    const unsigned      PG_PORT         (5432);
    const unsigned      SERVER_PORT     (1234);
    const unsigned      PLAYER_PORT     (1235);
    const unsigned      ECHONEST_PORT   (1236);
    
}

#endif	/* DEFS_H */

