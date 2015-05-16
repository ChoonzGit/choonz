#include "net_player.h"
#include "../library/pg_store.h"
#include "../library/defs.h"
#include <sys/types.h>
#include <memory>
#include <iostream>
#include <cstdlib>
#include <sys/socket.h>
#include <netdb.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>
#include <time.h>
#include <sys/types.h>

extern "C"{
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
}

//namespace {
//const int AVCODEC_MAX_AUDIO_FRAME_SIZE(8192);
const int BROADCAST_FRAME(40000); // This is a buffer with 200msec of samples plus header + a bit of free room at 48k stereo
const int HEADER_LENGTH(512); // Size of meta data
const int AUDIO_FRAME_SIZE(8192);
//}

NetPlayer::NetPlayer (const std::string& device) :
    m_device(device),
    m_stream(-1),
    m_sync(""),
    m_fd(-1),
    m_status(STATUS_IDLE),
    m_frame_buffer(0),
    m_bytes_per_packet(0),
    m_buffer_pos(0),
    m_encoding(0),
    m_channels(0),
    m_rate(0),
    m_playback_time(0),
    m_state (stateInvalid),
    m_bytes_buffered(0)
{
    m_outbuf = new uint8_t [AUDIO_FRAME_SIZE * 2];

    avcodec_register_all();
    av_register_all();
    avformat_network_init();

    m_state = stateReady;
    m_frame_buffer = new char [BROADCAST_FRAME];
}

NetPlayer::~NetPlayer ()
{
    avformat_network_deinit();
    delete [] m_frame_buffer;
    delete [] m_outbuf;
}
/*
Timestamp in Unix format
Current playback state e.g. playing, paused, idle.
Total length in bytes of audio frames which will follow (if idle, this will be 0 and remaining fields may not exist).
Number of channels
Channel sample rate
Channel encoding
Track length
Current position
Track name
Other meta data? E.g. artist, album, etc.
*/

/*
 * Send a single buffer every 200msec. This buffer consists of a 512-byte header, followed
 * by 200 msec of samples at track sample rate.
*/
void NetPlayer::run ()
{
    PgStore *       pdb (new PgStore("127.0.0.1"));
    unsigned        tracks_played(0);

    if (pdb->isOk())
    {
        m_fd = socket (AF_INET, SOCK_DGRAM, 0);

        if (checkValid())
        {
            char                loopch(0);
            unsigned            packets_sent(0); //DEBUG

            struct sockaddr_in groupSock;
            struct in_addr     localInterface;

            memset(&groupSock, 0, sizeof(groupSock));
            memset(&localInterface, 0, sizeof(localInterface));

            groupSock.sin_family      = AF_INET;
            groupSock.sin_port        = htons(3860); //Old player uses 3850
            inet_pton(AF_INET, m_device.c_str(), &groupSock.sin_addr);

            setsockopt(m_fd, IPPROTO_IP, IP_MULTICAST_LOOP, (char *) &loopch, sizeof(loopch));

            inet_pton(AF_INET, "192.168.0.116", &localInterface.s_addr); //TODO: discover our own IPv4 or pass as a param (this is our address when playing!)
            setsockopt(m_fd, IPPROTO_IP, IP_MULTICAST_IF, (struct in_addr*) &localInterface, sizeof(localInterface));

            // Timer stuff
            timer_t             timerid;
            struct sigevent     sevent;
            struct itimerspec   new_val;

            memset(&sevent, 0, sizeof(sevent));

            sevent.sigev_notify = SIGEV_SIGNAL;
            sevent.sigev_signo = SIGUSR2;
            sevent.sigev_value.sival_ptr = &timerid;

            if (timer_create(CLOCK_REALTIME, &sevent, &timerid) == 0)
            {
                new_val.it_interval.tv_sec = 0;
                new_val.it_interval.tv_nsec = 200000000; // Set timer for 200msec
                new_val.it_value.tv_sec = 0; // Start it off by firing in 200msec
                new_val.it_value.tv_nsec = 200000000;

                timer_settime(timerid, 0, &new_val, 0);
            }

            sigset_t        mask;
            sigemptyset(&mask);
            sigaddset(&mask, SIGUSR2);

            while (m_state != stateExit)
            {
                if (m_state == stateReady && m_file.size ())
                {
                    bool                planar(false);
                    AVFormatContext *   format_ctx (0);

                    packets_sent = 0;

                    m_playback_time = 0;
                    
                    m_state = statePlaying;

                    if (avformat_open_input(&format_ctx, m_file.c_str(), 0, 0) == 0)
                    {
                        if (avformat_find_stream_info(format_ctx, 0) == 0)
                        {
                            m_stream = -1;

                            av_dump_format(format_ctx, 0, m_file.c_str(), 0);
                            for (unsigned i = 0; i < format_ctx->nb_streams; ++i)
                            {
                                if (format_ctx->streams[i]->codec->codec_type == AVMEDIA_TYPE_AUDIO)
                                {
                                    m_stream = i;
                                    break;
                                }
                            }

                            if (m_stream != -1)
                            {
                                AVCodecContext *    codec_ctx (format_ctx->streams[m_stream]->codec);

                                if (codec_ctx)
                                {
                                    AVCodec *       codec(avcodec_find_decoder(codec_ctx->codec_id));

                                    if (codec && (avcodec_open2(codec_ctx, codec, 0) == 0))
                                    {
                                        int                 len(0);
                                        int                 out_size(0);
                                        AVPacket            pkt;
                                        AVFrame *           frame(av_frame_alloc());

                                        m_channels = codec_ctx->channels;
                                        m_encoding = 16; // Assume 16 bit, else examine value of codec_ctx->sample_fmt returned enum
                                        m_rate = codec_ctx->sample_rate;
                                        planar = codec_ctx->sample_fmt == AV_SAMPLE_FMT_S16P;

                                        std::cerr << "File " << m_file << " open at " << m_rate << "Hz, channels = " << m_channels << " planar = " << (planar ? "true" : "false") << std::endl;
                                        av_init_packet(&pkt);

                                        m_status = STATUS_START;

                                        m_bytes_per_packet = m_rate/5 << 2; // Send 200ms sized chunks at 2 bytes (16bit int) per channel. mono sent as left == right

                                        prepareHeader();

                                        while (m_state == statePlaying && (av_read_frame(format_ctx, &pkt) == 0))
                                        {
                                            if (pkt.stream_index == m_stream)
                                            {
                                                out_size = AUDIO_FRAME_SIZE;
                                                
                                                while (pkt.size)
                                                {
                                                    len = avcodec_decode_audio4(codec_ctx, frame, &out_size, &pkt);

                                                    if (len > 0)
                                                    {
                                                        if (out_size)
                                                        {
                                                            if (m_bytes_buffered + (planar ? frame->linesize[0] << 1 : frame->linesize[0]) > m_bytes_per_packet)
                                                            {
                                                                int  padding(m_bytes_per_packet - m_bytes_buffered);

                                                                if (planar)
                                                                {
                                                                    for (int x = 0,y = 0,z = 0 ; x < (padding >> 1); x += 2)
                                                                    {
                                                                        m_frame_buffer[m_buffer_pos++] = frame->data[0][y++];
                                                                        m_frame_buffer[m_buffer_pos++] = frame->data[0][y++];
                                                                        m_frame_buffer[m_buffer_pos++] = frame->data[m_channels == 2 ? 1 : 0][z++];
                                                                        m_frame_buffer[m_buffer_pos++] = frame->data[m_channels == 2 ? 1 : 0][z++];
                                                                    }
                                                                }
                                                                else
                                                                {
                                                                    memcpy (m_frame_buffer + m_buffer_pos, frame->data [0], padding);
                                                                }

                                                                sigwaitinfo(&mask, 0); // Wait here until timer tick
                                                                sendto (m_fd, m_frame_buffer, BROADCAST_FRAME, 0, (struct sockaddr *) &groupSock, sizeof(groupSock));
                                                                ++packets_sent;
                                                                m_status = STATUS_CONTINUE;
                                                                prepareHeader();

                                                                if (planar)
                                                                {
                                                                    for (int x = 0,y = padding >> 1,z = padding >> 1; x < (frame->linesize[0] - (padding >> 1)); x += 2)
                                                                    {
                                                                        m_frame_buffer[m_buffer_pos++] = frame->data[0][y++];
                                                                        m_frame_buffer[m_buffer_pos++] = frame->data[0][y++];
                                                                        m_frame_buffer[m_buffer_pos++] = frame->data[m_channels == 2 ? 1 : 0][z++];
                                                                        m_frame_buffer[m_buffer_pos++] = frame->data[m_channels == 2 ? 1 : 0][z++];
                                                                    }
                                                                }
                                                                else
                                                                    memcpy (m_frame_buffer + m_buffer_pos, frame->data [0] + padding, frame->linesize[0] - padding);

                                                                m_bytes_buffered = planar ? (frame->linesize[0] - (padding >> 1)) << 1: frame->linesize[0]  - padding;
                                                                m_buffer_pos += planar ? 0 : m_bytes_buffered;
                                                            }
                                                            else
                                                            {
                                                                if (planar)
                                                                {
                                                                    for (int x = 0, y = 0, z = 0; x < frame->linesize[0]; x += 2)
                                                                    {
                                                                        m_frame_buffer[m_buffer_pos++] = frame->data[0][y++];
                                                                        m_frame_buffer[m_buffer_pos++] = frame->data[0][y++];
                                                                        m_frame_buffer[m_buffer_pos++] = frame->data[m_channels == 2 ? 1 : 0][z++];
                                                                        m_frame_buffer[m_buffer_pos++] = frame->data[m_channels == 2 ? 1 : 0][z++];
                                                                    }
                                                                }
                                                                else
                                                                {
                                                                    memcpy (m_frame_buffer + m_buffer_pos, frame->data [0], frame->linesize[0]);
                                                                }
                                                                m_buffer_pos += (planar ? 0 : frame->linesize[0]);
                                                                m_bytes_buffered += (planar ? frame->linesize[0] << 1 : frame->linesize[0]);
                                                            }
                                                        }
                                                        pkt.size -= len;
                                                        pkt.data += len;
 
                                                    }
                                                    else
                                                    {
                                                        break; //TODO: If it's not here the first track ends and lots of "Header missing" messages spit out.
                                                    }
                                                    
                                                    av_frame_unref(frame);
                                                }
                                            }

                                            av_free_packet(&pkt);
                                        }

                                        m_status = STATUS_DONE;
                                        prepareHeader(false); // No reset, just update that we're done
                                        memset (m_frame_buffer + m_buffer_pos, 0, m_bytes_per_packet - m_bytes_buffered); // Pad last buffer with silence
                                        sigwaitinfo(&mask, 0);
                                        sendto (m_fd, m_frame_buffer, BROADCAST_FRAME, 0, (struct sockaddr *) &groupSock, sizeof(groupSock));
                                        ++packets_sent;

                                        av_free_packet(&pkt);

                                        m_status = STATUS_SILENCE;

                                        prepareHeader();

                                        memset (m_frame_buffer + m_buffer_pos, 0, m_bytes_per_packet); // Create a buffer of silence

                                        for (int i = 0; i < 5; ++i) // And sent it 5 times
                                        {
                                            sigwaitinfo(&mask, 0);
                                            sendto (m_fd, m_frame_buffer, BROADCAST_FRAME, 0, (struct sockaddr *) &groupSock, sizeof(groupSock));
                                        }

                                        av_frame_unref(frame);
                                        av_frame_free(&frame);
                                    }

                                    avcodec_close(codec_ctx);
                                }
                            }
                        }
                        else std::cout << "Error: No stream info for " << m_file << "\n";

                        avformat_close_input(&format_ctx);
                    }
                    else std::cout << "Error: can't initialise context for " << m_file << "\n";

                    m_file.clear ();

                    m_state = m_state == statePlaying ? stateReady : m_state == stateStopping ? stateStopped : m_state;

                    std::cout << "Total packets sent: " << packets_sent << "\n";
                }

                sigwaitinfo(&mask, 0);

                if (m_queue.size())
                {
                    m_file = pdb->getTrack(m_queue.front());

                    m_queue.pop();
                }
            }

            timer_delete(timerid);
        }
        else
        {
            std::cerr << "Player not in a valid state" << std::endl;
        }

        delete pdb;
    }

    m_state = stateInvalid;
}

void NetPlayer::play ()
{
    while (m_state == stateStopping) sleep (1);

    m_state = stateReady;
}

void NetPlayer::play (const std::string& file)
{
    //m_file = file;
    m_queue.push(file); //TODO: Locking if we keep this code
}

void NetPlayer::stop ()
{
    m_state = m_state == statePlaying ? stateStopping : stateStopped;
}

void NetPlayer::pause ()
{
    m_state = m_state == statePlaying ? statePaused : m_state;
}

bool NetPlayer::checkValid ()
{
    return m_fd > -1;
}

void NetPlayer::prepareHeader (bool reset_position)
{
    const char * const status_name [] = { "IDLE", "START", "CONT", "DONE", "SILENT" };

    ++m_playback_time;

    m_sync.str("");
    m_sync << status_name [m_status] << "|"
           << m_file << "|"
           << m_rate << "|"
           << m_encoding << "|"
           << m_channels << "|"
           << m_bytes_per_packet << "|"
           << m_playback_time/5 << "|";

    memcpy (m_frame_buffer, m_sync.str().data(), m_sync.str().size());

    for (int x = m_sync.str().size(); x < HEADER_LENGTH; ++x) { m_frame_buffer [x] = 0; } // Pad out to a known size

    if (reset_position)
    {
        m_buffer_pos = HEADER_LENGTH;
        m_bytes_buffered = 0;
    }
}
