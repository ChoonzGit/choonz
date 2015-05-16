#include "net_player.h"
#include "../library/network.h"
#include "../library/command_processor.h"
#include "../library/defs.h"
#include "command_play.h"
#include <cstdlib>
#include <signal.h>
#include <boost/thread.hpp>
#include <unistd.h>

namespace {
    const unsigned      NET_PORT(1235);          //TODO: Command line option
    const std::string   PG_HOST("127.0.0.1"); //TODO: Command line option to pass to NetPlayer
}

int main (int argc, char * * argv)
{
    std::cerr << "Choonz Multicast Source (Boost.Asio)" << std::endl;

    if (daemon(1,1) == 0)
    {
        Network             net(Choonz::PLAYER_PORT);
        Network::Event      e;
        unsigned            client(0);
        CommandProcessor    cp;
        bool                done(false);
        sigset_t            mask;

        std::cerr << "Running as daemon" << std::endl;
        
        sigemptyset(&mask);
        sigaddset(&mask, SIGUSR2);

        pthread_sigmask(SIG_BLOCK, &mask, 0);

        net.createServer();

        NetPlayer   np("239.255.42.42");

        cp.addCommand(Choonz::COMMAND_PLAY_FILE, new CommandPlay(net, np));

        boost::thread np_thread (boost::ref(np));

        while (!done)
        {
            switch ((e = net.getEvent(client)))
            {
                case Network::eventData:
                {
                    std::string     data;

                    if (net.clientData(client, data))
                    {
                        cp.runCommand(data, client);
                    }
                }
                break;
                default:
                    ; // Shut GCC up

            }
            usleep(250000);
        }
    }
    return EXIT_SUCCESS;
}
