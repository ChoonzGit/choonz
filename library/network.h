/*
 * File:   network.h
 * Author: adrian
 * Network support using Boost::Asio
 * Created on 21 July 2013, 14:21
 */

#ifndef NETWORK_H
#define	NETWORK_H

#include <boost/array.hpp>
#include <map>
#include <string>
#include <queue>
#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <boost/thread.hpp>
#include <cstdlib>
#include <cstring>


//TODO: Give this namespace a name
namespace
{
    class TcpConnection : public boost::enable_shared_from_this<TcpConnection>
    {
    public:
        TcpConnection(boost::asio::io_service& io_service) :
            m_socket(io_service),
            m_running(false),
            m_bytes_to_write(0),
            m_bytes_written(0),
            m_writing(false)
        {
        }

        boost::asio::ip::tcp::socket& socket()
        {
            return m_socket;
        }

        void operator ()()
        {
            m_running = true;
//            std::string message("Choonz Music System Server (Boost.Asio version)\n");

//            m_socket.write_some(boost::asio::buffer(message, message.size()));

            while (m_running)
            {
//                m_socket.async_read_some(boost::asio::buffer(m_buffer, INCOMING_BUFFER_SIZE), boost::bind(&TcpConnection::handleRead, shared_from_this(), boost::asio::placeholders::error, boost::asio::placeholders::bytes_transferred));
//                m_socket.async_read_some(m_asio_buffer, boost::bind(&TcpConnection::handleRead, shared_from_this(), boost::asio::placeholders::error, boost::asio::placeholders::bytes_transferred));
//                boost::asio::async_read_until(m_socket, m_buffer, '\n', boost::bind(&TcpConnection::handleRead, shared_from_this(), boost::asio::placeholders::error, boost::asio::placeholders::bytes_transferred));
                if (m_socket.available())
                {
                    std::string         message;

                    boost::asio::read(m_socket, m_buffer, boost::asio::transfer_at_least(m_socket.available()));

                    std::istream is(&m_buffer);
                    std::getline(is, message, '\n');
                    m_incoming.push(message);
                    std::cerr << "Got message: " << message << std::endl;
                }
                if (m_outgoing.size() && !m_writing)
                {
                    m_writing = true;
                    m_bytes_to_write = m_outgoing.front().size() < OUTGOING_BUFFER_SIZE ? m_outgoing.front().size() : m_bytes_written < (m_outgoing.front().size() - OUTGOING_BUFFER_SIZE) ? OUTGOING_BUFFER_SIZE : m_outgoing.front().size() - m_bytes_written;
                    m_socket.async_write_some(boost::asio::buffer(m_outgoing.front().data() + m_bytes_written, m_bytes_to_write), boost::bind(&TcpConnection::handleWrite, shared_from_this(), boost::asio::placeholders::error, boost::asio::placeholders::bytes_transferred));
                }

                usleep(125000); //Not sure this is needed
            }
        }

        bool haveData () const
        {
            return m_incoming.size() > 0;
        }

        void getData (std::string& data)
        {
            data = m_incoming.front();
            m_incoming.pop();
        }

        void sendData (const std::string& data)
        {
            m_outgoing.push(data);
        }

    private:
        enum { INCOMING_BUFFER_SIZE = 512, OUTGOING_BUFFER_SIZE = 32768 };
        boost::asio::ip::tcp::socket    m_socket;
//        char                            m_buffer [INCOMING_BUFFER_SIZE];
        bool                            m_running;
        std::queue<std::string>         m_incoming; // Messages received
        std::queue<std::string>         m_outgoing; // Messages to be sent
        unsigned                        m_bytes_to_write;
        unsigned                        m_bytes_written;
        bool                            m_writing;
//        boost::array<char, INCOMING_BUFFER_SIZE>   m_buffer = {};
//        boost::asio::mutable_buffer     m_asio_buffer = boost::asio::buffer(m_buffer);
      boost::asio::streambuf           m_buffer;

        void handleWrite(const boost::system::error_code& error, size_t bytes)
        {
            m_writing = false;
            m_bytes_written += bytes;

            if (m_bytes_written == m_outgoing.front().size())
            {
                m_outgoing.pop();
                m_bytes_written = 0;
            }
        }

        void handleRead(const boost::system::error_code& error, std::size_t bytes)
        {
            std::cerr << "Got read\n";
            if (bytes == 0)
            {
                m_running = false;
            }
            else
            {

                for (unsigned x = 0; x < INCOMING_BUFFER_SIZE; ++x)
                {
                    //if (m_buffer [x] == 10 || m_buffer [x] == 13)
                    //    m_buffer [x] = 0;
                }

                //m_incoming.push(std::string(m_buffer));

                //memset(m_buffer, 0, INCOMING_BUFFER_SIZE);
            }
        }
    };

    class TcpServer
    {
    public:
        TcpServer(boost::asio::io_service& service, unsigned port) :
            m_acceptor(service, boost::asio::ip::tcp::endpoint(boost::asio::ip::tcp::v4(), port)),
            m_listener(0),
            m_client_list()
            {
                m_acceptor.set_option(boost::asio::ip::tcp::acceptor::reuse_address(true));
                startAccept();
            }

            bool hasData (unsigned& client) const
            {
                bool    found(false);

                for (const auto& i : m_client_list)
                {
                    if ((found = i.second->haveData()))
                    {
                        client = i.first;
                        break;
                    }
                }

                return found;
            }

            bool getData (unsigned client, std::string& data)
            {
                bool    got_data(false);

                const auto& x(m_client_list.find(client));
                std::cerr << "getData()\n";
                if (x != m_client_list.end())
                {
                    std::string s;

                    (*x).second->getData(s);

                    if (s.size() == 0) // Our client closed the connection
                    {
                        std::cerr << "Connection closed\n";
                        m_client_list.erase(x);
                    }
                    else
                    {
                        got_data = true;
                        data = s;
                    }
                }

                return got_data;
            }

            bool sendData (unsigned client, const std::string& data)
            {
                bool        sent_data(false);

                const auto& x(m_client_list.find(client));

                if (x != m_client_list.end())
                {
                    (*x).second->sendData(data);
                    sent_data = true;
                }

                return sent_data;
            }
    private:
        static unsigned                                         m_next_client;
        unsigned                                                m_port;
        boost::asio::ip::tcp::acceptor                          m_acceptor;
        boost::shared_ptr<TcpConnection>                        m_listener;
        std::map<unsigned, boost::shared_ptr<TcpConnection>>    m_client_list;

        void startAccept()
        {
            m_listener.reset(new TcpConnection(m_acceptor.get_io_service()));
            m_acceptor.async_accept(m_listener->socket(), boost::bind(&TcpServer::handleAccept, this, m_listener, boost::asio::placeholders::error));
        }

        void handleAccept(boost::shared_ptr<TcpConnection> connection, const boost::system::error_code& error)
        {
            if (!error)
            {
                m_client_list.insert(std::pair<unsigned, boost::shared_ptr<TcpConnection>> (m_next_client, connection));
                boost::thread(boost::ref(*connection));
                ++m_next_client;
            }

            startAccept();
        }
    };
}

class Network
{
public:
    enum Event { eventNone, eventConnect, eventDisconnect, eventData, };
    explicit Network(unsigned port);
    ~Network();
    bool createServer();
    bool createConnection(const std::string& host, unsigned port);
    Event getEvent (unsigned&) const;
    bool clientData(unsigned, std::string& data) const;
    bool sendData (unsigned, const std::string&);

private:
    TcpConnection *             m_connection;
    TcpServer *                 m_server;
    boost::asio::io_service     m_service;

    unsigned                    m_port;
};

#endif	/* NETWORK_H */

