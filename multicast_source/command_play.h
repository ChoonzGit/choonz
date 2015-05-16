#ifndef COMMAND_PLAY_H
#define	COMMAND_PLAY_H

#include "../library/function.h"

class NetPlayer;

/*
 * @brief Control the multicast player (source)
 *
 */
class CommandPlay : public Function
{
public:
    CommandPlay (Network&, NetPlayer&);
    bool process (const std::vector<std::string>&, unsigned);
        
private:
    NetPlayer&       m_net_player;
};

#endif	/* COMMAND_LIBRARY_H */
