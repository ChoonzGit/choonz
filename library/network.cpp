#include "network.h"
#include <boost/asio.hpp>
#include <boost/thread.hpp>
#include <iostream> //debug
unsigned TcpServer::m_next_client(0);

Network::Network(unsigned port) :
    m_port(port)
{

}

Network::~Network()
{
    delete m_connection;
    delete m_server;
}

bool Network::createServer()
{
    m_server = new TcpServer(m_service, m_port);
    boost::thread (boost::bind(&boost::asio::io_service::run, &m_service));
    return true;
}

bool Network::createConnection(const std::string& host, unsigned port)
{
    return true;
}

Network::Event Network::getEvent (unsigned& client) const
{
    Event   e(eventNone);

    if (m_server->hasData(client))
        e = eventData;

    return e;
}

bool Network::clientData(unsigned client, std::string& data) const
{
    return m_server->hasData(client) ? m_server->getData(client, data) : false;
}

bool Network::sendData(unsigned client, const std::string& data)
{
    return m_server->sendData(client, data);
}