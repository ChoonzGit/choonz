#include "command_location.h"
#include "track_manager.h"
#include <iostream> //AUDEBUG

CommandLocation::CommandLocation (Network& net, TrackManager& tm) :
    Function(net),
    m_track_manager(tm)
{

}

bool CommandLocation::process(const std::vector<std::string>& message, unsigned client)
{
    bool        success(false);

    if (message.size())
    {
        for (const auto& i : message)
	{
	    std::cout << "Adding location: " << i << std::endl;	
            m_track_manager.addLocation(i);
	}
        success = true;
    }

    return success; //FIXME: Can we test for success?
}
