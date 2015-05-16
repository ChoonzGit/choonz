#include "command_processor.h"
#include <boost/algorithm/string/split.hpp>

CommandProcessor::CommandProcessor()
{

}

CommandProcessor::~CommandProcessor()
{

}

void CommandProcessor::addCommand(const std::string& command, Function* handler)
{
    m_command_map.insert(std::pair<std::string, std::unique_ptr<Function>>(command, std::unique_ptr<Function>(handler)));
}

// The command parameter consists of the command itself, followed by any arguments, all separated by '|'
bool CommandProcessor::runCommand (const std::string& command, unsigned client)
{
    bool                        success(false);
    std::vector<std::string>    parts;

    boost::iter_split(parts, command, boost::first_finder("|"));

    const auto& it(m_command_map.find(parts[0]));

    parts.erase(parts.begin());

    if (it != m_command_map.end())
    {
        success = ((it->second)->process(parts, client));
    }
    return success;
}
