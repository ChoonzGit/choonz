#include "../library/network.h"
#include "../library/defs.h"
#include "command_library.h"
#include "track_manager.h"

CommandLibrary::CommandLibrary (Network& net, TrackManager& tm) :
    Function(net),
    m_track_manager(tm)
{
    
}

// The message contains the parameter to say which format to return data in
// FIXME: Make this tidier, maybe use a helper
bool CommandLibrary::process(const std::vector<std::string>& message, unsigned client)
{
    bool    success(false);
    
    if (message.size())
    {
        std::string     library;
        m_track_manager.getTracks(library);
        
        m_network.sendData(client, Choonz::RESPONSE_BEGIN);
        m_network.sendData(client, "|");
        m_network.sendData(client, Choonz::COMMAND_LIBRARY);
        m_network.sendData(client, "|");
        m_network.sendData(client, Choonz::PARAM_SUCCESS);
        m_network.sendData(client, "\n");
        
        m_network.sendData(client, library);
        m_network.sendData(client, Choonz::RESPONSE_END);
        m_network.sendData(client, "\n");
    }
    
    return success;
}
