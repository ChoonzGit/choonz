/*
  Multicast IPV4 player for choonz. Runs nicely on a Raspberry Pi :)
  $Change: 46 $
*/

#include "player_alsa.h"

#include <sys/socket.h>
#include <sys/select.h>
#include <netdb.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <cstring>
#include <string>
#include <queue>
#include <iostream>
#include <boost/algorithm/string/split.hpp>
#include <boost/tokenizer.hpp>
#include <boost/thread.hpp>
#include <boost/lexical_cast.hpp>

namespace {
const int SYNC_MESSAGE_SIZE(512); // This header appears at the begining of each broadcast
const int DATA_MESSAGE_SIZE(40000); // Meta data and frame data are in the same message, meta data says how big the frame buffer is
const int SYNC_INFO(0);
const int SYNC_FILE(1);
const int SYNC_RATE(2);
const int SYNC_BITS(3);
const int SYNC_CHAN(4);
const int SYNC_DATA(5);
const int PARAM_IFACE(1);
const int PARAM_PORT(2);
const int PARAM_DEVICE(3);
const int PARAM_BUFSIZE(4);   // Sizeof playback ring buffer in 8192 frame-sized bytes
}
int usage(const char *);

int main (int argc, const char * const * argv)
{
    int                 flag_on(1);
    char                buffer [DATA_MESSAGE_SIZE];
    struct sockaddr_in  localSock;
    struct ip_mreq      im_req;
    int                 m_fd(0);
    int                 recvd(0);
    unsigned            buffer_pos(0); // Where to place next received frames
    unsigned            buffer_size(0); // Total length of buffer in bytes
    unsigned char *     ring_buffer(0);

    if (argc < 4)
        return usage(argv [0]);

//    daemon(1, 1);

    memset(&localSock, 0, sizeof(localSock));
    memset(&im_req, 0, sizeof (im_req));

    m_fd = socket(AF_INET, SOCK_DGRAM, 0); //TODO: Check for failure
    setsockopt(m_fd, SOL_SOCKET, SO_REUSEADDR, &flag_on, sizeof(flag_on));

    localSock.sin_family      = AF_INET;
    localSock.sin_port        = htons(boost::lexical_cast<int>(argv[PARAM_PORT])); // Old player uses 3850, new 3860
    localSock.sin_addr.s_addr = INADDR_ANY;

    bind(m_fd, (struct sockaddr*) &localSock, sizeof(localSock));

    inet_pton(AF_INET, "239.255.42.42", &im_req.imr_multiaddr);
    im_req.imr_interface.s_addr = inet_addr(argv[PARAM_IFACE]); // TODO: Make this the interface we want to receive on
    setsockopt(m_fd, IPPROTO_IP, IP_ADD_MEMBERSHIP, &im_req, sizeof(im_req));

    unsigned buffer_packets (boost::lexical_cast<unsigned>(argv[PARAM_BUFSIZE]));

    buffer_size = buffer_packets * Player::FRAME_PACKET_SIZE * Player::STEREO_FRAME_SIZE;

    std::cerr << "Ring buffer size: " << buffer_size << std::endl;

    ring_buffer = new unsigned char [buffer_size];

    memset (ring_buffer, 0, buffer_size);

    Player p(argv[PARAM_DEVICE], 44100, 2, buffer_size, ring_buffer);

    p.enumerate();
    boost::thread play_thread (boost::ref (p));

    buffer_pos = buffer_size >> 1;
    buffer_pos -= buffer_pos % Player::STEREO_FRAME_SIZE;
 
    while(true)
    {
        if ((recvd = recv(m_fd, buffer, DATA_MESSAGE_SIZE, MSG_WAITALL)) == DATA_MESSAGE_SIZE)
        {
            std::string                 sync(buffer, SYNC_MESSAGE_SIZE);
            std::vector<std::string>    tokens;
            int                         frame_data_size(0);
            
            boost::iter_split(tokens, sync, boost::first_finder("|"));

            if (tokens[SYNC_INFO] == "START")
            {
                std::cerr << tokens[SYNC_FILE] << " Rate: " << boost::lexical_cast<int>(tokens[SYNC_RATE]) << " Channels: " << boost::lexical_cast<int>(tokens[SYNC_CHAN]) << "\n";
                p.setRate(boost::lexical_cast<int>(tokens[SYNC_RATE]), boost::lexical_cast<int>(tokens[SYNC_CHAN]));
                p.start(buffer_pos);
            }

          //p.setRate(boost::lexical_cast<int>(tokens[SYNC_RATE]), boost::lexical_cast<int>(tokens[SYNC_CHAN]));
            
            frame_data_size = boost::lexical_cast<int>(tokens[SYNC_DATA]);

            if (buffer_pos + frame_data_size > buffer_size)
            {
                int     padding(buffer_size - buffer_pos);

                memcpy(ring_buffer + buffer_pos, buffer + SYNC_MESSAGE_SIZE, padding);

                memcpy(ring_buffer, buffer + SYNC_MESSAGE_SIZE + padding, buffer_pos = frame_data_size - padding);
            }
            else
            {
                memcpy(ring_buffer + buffer_pos, buffer + SYNC_MESSAGE_SIZE, frame_data_size);
                buffer_pos += frame_data_size;
            }
        }
        else
        {
            std::cerr << "Network error: only got " << recvd << " bytes" << std::endl;
        }
    }

    setsockopt(m_fd, IPPROTO_IP, IP_DROP_MEMBERSHIP, &im_req, sizeof(im_req));

    delete [] ring_buffer;

    return 0;
}

int usage(const char * program)
{
    std::cout << "Choonz Multicast Player\n\n";
    std::cout << program << " [iface_address] [audio_device] [buffer_size]\n\n";
    std::cout << "    iface_address  = local IPv4 address of interface to receive multicast packets\n";
    std::cout << "    port number    = 3850 or 3860\n";
    std::cout << "    audio_device   = name of ALSA output device\n";
    std::cout << "    buffer_size    = size of playback ring buffer as number of 8192 frame-sized packets\n";
    std::cout << "E.g. " << program << " 192.168.0.24 hw:External,0,0\n";
    std::cout << "Multicast source is currently assumed to be 239.255.42.42\n";

    return 0;
}
