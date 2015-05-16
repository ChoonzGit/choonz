/* $Change: 46 $
 *
 * */

#include "player_alsa.h"

#include <sys/types.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <unistd.h>
#include <cstring>
#include <iostream> //debug
#include <fstream>

namespace {

}

Player::Player(const std::string& device, int rate, int channels, unsigned buffer_size, const unsigned char * buffer) :
    Runnable(),
    m_device(device),
    m_sample_rate(rate),
    m_channels(channels),
    m_buffer_size(buffer_size),
    m_buffer(buffer),
    m_buffer_pos(0),
    m_pcm_device(0),
    m_set_playback(false),
    m_new_playback(0)
{
    std::cerr << "Buffer size set to " << m_buffer_size << std::endl;
}

void Player::run()
{
    if (openAlsa())
    {
        int             error(0);
        
        configAlsa();
 
        while(m_continue) //TODO: Implement orderly shutdown
        {
            if (m_device_ready)
            {
                if ((error = snd_pcm_writei(m_pcm_device, m_buffer + m_buffer_pos, FRAME_PACKET_SIZE)) < 0)
                    recover(error);
            
                if (m_set_playback)
                {
                    m_set_playback = false;
                    m_buffer_pos = m_new_playback;
                }
                else
                {
                    unsigned pos(m_buffer_pos);
                
                    m_buffer_pos += FRAME_PACKET_SIZE * STEREO_FRAME_SIZE;

                    m_buffer_pos %= m_buffer_size;
                }
            }
        }

        close();
    }
}

void Player::setRate (unsigned rate, int channels)
{
    if (rate != m_sample_rate)
    {
        m_sample_rate = rate;
        std::cerr << "Setting rate: " << m_sample_rate << " : " << (m_channels == 2 ? "stereo" : "mono") << std::endl;

        close();
        openAlsa();
        configAlsa();
    }
}

void Player::start (unsigned buffer_pos)
{
    pthread_mutex_lock(&m_mutex);

    m_new_playback = (m_buffer_size >> 1) + buffer_pos;

    m_new_playback %= m_buffer_size;
    m_new_playback -= m_new_playback % (FRAME_PACKET_SIZE * STEREO_FRAME_SIZE);

    std::cerr << "Start playback: input " << buffer_pos << " output " << m_new_playback << std::endl;

    m_set_playback = true;

    pthread_mutex_unlock(&m_mutex);
}

unsigned Player::currentPosition () const
{
    return m_buffer_pos;
}

bool Player :: openAlsa ()
{
    return snd_pcm_open (&m_pcm_device, m_device.c_str(), SND_PCM_STREAM_PLAYBACK, 0) == 0;
}

bool Player :: configAlsa ()
{
    m_device_ready = false;

    std::cerr << "Rate = " << m_sample_rate << " with channels = " << m_channels << std::endl;
    snd_pcm_hw_params_t *   hw_params;

    if (snd_pcm_hw_params_malloc (&hw_params) == 0)
    {
        snd_pcm_hw_params_any (m_pcm_device, hw_params);
        snd_pcm_hw_params_set_access (m_pcm_device, hw_params, SND_PCM_ACCESS_RW_INTERLEAVED);
        snd_pcm_hw_params_set_format (m_pcm_device, hw_params, SND_PCM_FORMAT_S16_LE);
        snd_pcm_hw_params_set_channels (m_pcm_device, hw_params, 2);

        snd_pcm_hw_params_set_rate_near (m_pcm_device, hw_params, &m_sample_rate, 0);
        
        snd_pcm_hw_params_set_buffer_size (m_pcm_device, hw_params, (8192 * 2) >> 2);
        snd_pcm_hw_params (m_pcm_device, hw_params);

        snd_pcm_hw_params_free (hw_params);

        if (snd_pcm_prepare (m_pcm_device) == 0)
        {
            m_device_ready = true;
        }
    }

    return m_device_ready;
}

void Player :: close ()
{
    m_device_ready = false;
    //snd_pcm_drain(m_pcm_device);
    snd_pcm_close (m_pcm_device);
    m_pcm_device = 0;
}

int Player::recover (int error)
{
    int     result (0);
    std::cerr << "Got error: " << strerror(error) << "\n";
    if (error == -EPIPE)
        result = snd_pcm_prepare (m_pcm_device);
    else if (error == -ESTRPIPE)
    {
        while ((result = snd_pcm_resume (m_pcm_device)) == -EAGAIN)    usleep (5000000);

        if (result < 0)
            result = snd_pcm_prepare (m_pcm_device);
    }

    if (result < 0)
        std::cerr << "Can't recover: " << snd_strerror (result) << "\n";

    return result;
}

void Player::enumerate () const
{
    register int  err;
    int           cardNum;

    // Start with first card
    cardNum = -1;

    for (;;)
    {
        snd_ctl_t *cardHandle;

        // Get next sound card's card number. When "cardNum" == -1, then ALSA
        // fetches the first card
        if ((err = snd_card_next(&cardNum)) < 0)
        {
            printf("Can't get the next card number: %s\n", snd_strerror(err));
            break;
        }

        // No more cards? ALSA sets "cardNum" to -1 if so
        if (cardNum < 0) break;

        // Open this card's control interface. We specify only the card number -- not
        // any device nor sub-device too
        {
            char   str[64];

            sprintf(str, "hw:%i", cardNum);
            if ((err = snd_ctl_open(&cardHandle, str, 0)) < 0)
            {
                printf("Can't open card %i: %s\n", cardNum, snd_strerror(err));
                continue;
            }
        }

        {
            int      devNum;

            // Start with the first wave device on this card
            devNum = -1;

            for (;;)
            {
                snd_pcm_info_t  *pcmInfo;
                register int        subDevCount, i;

                // Get the number of the next wave device on this card
                if ((err = snd_ctl_pcm_next_device(cardHandle, &devNum)) < 0)
                {
                    printf("Can't get next wave device number: %s\n", snd_strerror(err));
                    break;
                }

                // No more wave devices on this card? ALSA sets "devNum" to -1 if so.
                // NOTE: It's possible that this sound card may have no wave devices on it
                // at all, for example if it's only a MIDI card
                if (devNum < 0) break;

                // To get some info about the subdevices of this wave device (on the card), we need a
                // snd_pcm_info_t, so let's allocate one on the stack
                snd_pcm_info_alloca(&pcmInfo);
                memset(pcmInfo, 0, snd_pcm_info_sizeof());

                // Tell ALSA which device (number) we want info about
                snd_pcm_info_set_device(pcmInfo, devNum);

                // Get info on the wave outs of this device
                snd_pcm_info_set_stream(pcmInfo, SND_PCM_STREAM_PLAYBACK);

                i = -1;
                subDevCount = 1;

                // More subdevices?
                while (++i < subDevCount)
                {
                    // Tell ALSA to fill in our snd_pcm_info_t with info on this subdevice
                    snd_pcm_info_set_subdevice(pcmInfo, i);
                    if ((err = snd_ctl_pcm_info(cardHandle, pcmInfo)) < 0)
                    {
                        printf("Can't get info for wave output subdevice hw:%i,%i,%i: %s\n", cardNum, devNum, i, snd_strerror(err));
                        continue;
                    }

                    // Print out how many subdevices (once only)
                    if (!i)
                    {
                        subDevCount = snd_pcm_info_get_subdevices_count(pcmInfo);
                        printf("\nFound %i wave output subdevices on card %i\n", subDevCount, cardNum);
                    }

                    // NOTE: If there's only one subdevice, then the subdevice number is immaterial,
                    // and can be omitted when you specify the hardware name
                    printf((subDevCount > 1 ? "    hw:%i,%i,%i\n" : "    hw:%i,%i\n"), cardNum, devNum, i);
                }
            }
        }

        // Close the card's control interface after we're done with it
        snd_ctl_close(cardHandle);
    }

    snd_config_update_free_global();
}
