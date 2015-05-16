#include "../library/network.h"
#include "../library/command_processor.h"
#include "../library/defs.h"
#include "track_manager.h"
#include "command_library.h"
#include "command_location.h"
#include <boost/thread.hpp>
#include <cstdlib>
#include <unistd.h>

namespace {
    const unsigned      NET_PORT(1234);          //TODO: Command line option
    const std::string   PG_HOST("127.0.0.1"); //TODO: Command line option
}

int main (int argc, const char * const * argv)
{
    std::cerr << "Choonz Server (Boost.Asio)" << std::endl;

    if (daemon(1,1) == 0)
    {
        Network             net(NET_PORT);
        Network::Event      e;
        TrackManager        tm(PG_HOST);
        CommandProcessor    cp;
        bool                done(false);
        unsigned            client;

        std::cerr << "Now running as daemon" << std::endl;
        cp.addCommand(Choonz::COMMAND_LOCATION, new CommandLocation(net, tm));
        cp.addCommand(Choonz::COMMAND_LIBRARY, new CommandLibrary(net, tm));

        boost::thread(boost::ref(tm));

        net.createServer();

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
