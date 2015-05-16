/* 
 * File:   command_library.h
 * Author: adrian
 *
 * Created on 04 August 2013, 10:18
 */

#ifndef COMMAND_LIBRARY_H
#define	COMMAND_LIBRARY_H

#include "../library/function.h"

class TrackManager;

/*
 * @brief Implement LIBRARY command (return library in one of 3 views)
 *
 */
class CommandLibrary : public Function
{
public:
    CommandLibrary (Network&, TrackManager&);
    bool process (const std::vector<std::string>&, unsigned);
        
private:
    TrackManager&       m_track_manager;
};

#endif	/* COMMAND_LIBRARY_H */

