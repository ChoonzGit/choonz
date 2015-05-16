#ifndef NET_PLAYER_H_DEFINED
#define NET_PLAYER_H_DEFINED

#include "../library/runnable.h"
#include <string>
#include <sstream>
#include <stdint.h>
#include <queue> // ActiveMQ

/**
 * @file net_player.h
 * @brief Play tracks across a multicast network socket
 *
 * Play a track across a network socket to which a player can connect at any time.
 * Send the decoded frames as chunks of data, preceded by a sync message which
 * gives track details, allowing a new client to open an audio device for output
 * using the correct sample rate and play the stream.
 *
 * Use the ffmpeg library to decode the media so the client doesn't have to. We
 * assume we'll use this system where bandwidth isn't an issue, e.g. LAN.
 * We play in realtime, in 200ms chunks. This allow the client to buffer.
 *
 * TODO: Have a common header which describes the 512 byte buffer as both
 *       source and player will use it.
 * */
class NetPlayer : public Runnable
{
public:
    explicit NetPlayer (const std::string&);
    ~NetPlayer ();

    void    run ();
    void    play ();
    void    play (const std::string&);
    void    stop ();
    void    pause ();
    bool    checkValid ();

private:
    enum playStatus { STATUS_IDLE, STATUS_START, STATUS_CONTINUE, STATUS_DONE, STATUS_SILENCE };
    enum eState { stateInvalid, stateReady, statePlaying, statePaused, stateStopping, stateStopped, stateExit };
    std::string			m_device;
    uint8_t *           m_outbuf;
    int                 m_stream; //Audio stream index (we don't do video...);
    std::stringstream   m_sync; // Holds track details, send to client before every buffer of frames.
    int                 m_fd; // File descriptor for multicast socket
    playStatus          m_status;
    char *              m_frame_buffer; // Stuff for sending
    int                 m_bytes_per_packet; // Number of bytes per decoded 100msec
    int                 m_buffer_pos; // Pointer to next place in buffer to add frames
    int                 m_encoding;
    int                 m_channels;
    long                m_rate;
    unsigned            m_playback_time; // The number of 200msec packets sent for the current track
    //These 4 from old base class
    eState				m_state;
    std::string			m_file;
    std::queue<std::string>    m_queue; // ActiveMQ stuff
    //
    int                 m_bytes_buffered; // Keep track of sample bytes in buffer

    void                prepareHeader (bool = true); // Fill in the 512-byte header with track info
    void                sendSilence (); // Send 1 second of silence at the end of each track
};

#endif