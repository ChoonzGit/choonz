/* 
 * File:   function.h
 * Author: adrian
 *
 * Created on 04 August 2013, 10:25
 */

#ifndef FUNCTION_H
#define	FUNCTION_H

#include <vector>
#include <string>

class Network;

/**
 * @brief This is the bas class for all commands supported by the server
 */
class Function
{
public:
    explicit Function(Network& net) : m_network(net), m_client(0) {} // FIXME 0 is a valid client, shouldn't be default
    virtual ~Function() {}
    virtual bool process (const std::vector<std::string>&, unsigned) = 0;
    
protected:
    Network&    m_network;
    unsigned    m_client;
};

#endif	/* FUNCTION_H */

