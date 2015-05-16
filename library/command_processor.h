/* 
 * File:   command_processor.h
 * Author: adrian
 *
 * Created on 03 August 2013, 21:44
 */

#ifndef COMMAND_PROCESSOR_H
#define	COMMAND_PROCESSOR_H

#include "function.h"
#include <map>
#include <string>
#include <memory>

class CommandProcessor
{
public:
    CommandProcessor();
    ~CommandProcessor();
    
    void    addCommand (const std::string&, Function*);
    bool    runCommand (const std::string&, unsigned);
    
private:
    std::map <std::string, std::unique_ptr<Function>>   m_command_map;
};

#endif	/* COMMAND_PROCESSOR_H */

