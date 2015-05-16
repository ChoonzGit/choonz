/* 
 * File:   command_location.h
 * Author: adrian
 *
 * Created on 04 August 2013, 10:18
 */

#ifndef COMMAND_LOCATION_H
#define	COMMAND_LOCATION_H

#include "../library/function.h"

class TrackManager;

/**
 * @brief  Implement the LOCATION command (scan the disk)
 */
class CommandLocation : public Function
{
    public:
        CommandLocation (Network&, TrackManager&);
        bool process (const std::vector<std::string>&, unsigned);
        
private:
    TrackManager&       m_track_manager;
};

#endif	/* COMMAND_LOCATION_H */

