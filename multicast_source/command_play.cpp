#include "../library/network.h"
#include "command_play.h"
#include "net_player.h"

CommandPlay::CommandPlay (Network& net, NetPlayer& np) :
    Function(net),
    m_net_player(np)
{
    
}

// The message contains the parameter to say which format to return data in
bool CommandPlay::process(const std::vector<std::string>& message, unsigned client)
{
    bool    success(false);
    
    if (message.size())
    {
        m_net_player.play(message[0]);
    }
    
    return success;
}
