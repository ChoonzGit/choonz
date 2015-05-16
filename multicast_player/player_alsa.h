#ifndef PLAYER_ALSA_H_DEFINED
#define PLAYER_ALSA_H_DEFINED

#include "../library/runnable.h"
#include <alsa/asoundlib.h>
#include <vector>
#include <string>
#include <pthread.h>

class Player : public Runnable
{
public:
    Player(const std::string&, int, int, unsigned, const unsigned char *);
    void run ();
    void setRate (unsigned, int);
    void enumerate () const;
    void start (unsigned);
    unsigned currentPosition () const;

    enum { MONO_FRAME_SIZE = 2, STEREO_FRAME_SIZE = 4, FRAME_PACKET_SIZE = 8192 }; // Optimum number of frames to feed alsa with

private:
    std::string                     m_device;
    unsigned                        m_sample_rate;
    int                             m_channels;
    unsigned                        m_buffer_size;  // How many packets in the buffer
    const unsigned char *           m_buffer; // Ring buffer for playback
    unsigned                        m_buffer_pos; // Playback position
    pthread_mutex_t                 m_mutex;
    snd_pcm_t *                     m_pcm_device;
    bool                            m_set_playback; // Set to true if we need to adjust the playback point
    unsigned                        m_new_playback; // New playback position to set

    bool                m_device_ready;
    int                 m_last_error;
    unsigned            m_playback_index; // Where we are playing back from

    bool                openAlsa ();
    bool                configAlsa ();
    void                close ();
    int                 recover (int);
};
#endif