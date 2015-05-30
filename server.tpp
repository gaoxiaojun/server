#include <string>

#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <boost/shared_ptr.hpp>

#include <boost/thread.hpp>
#include <boost/thread/locks.hpp>

namespace asio = boost::asio;
namespace ip = asio::ip;
namespace placeholders = asio::placeholders;
namespace sys = boost::system;

#include "./task_chat.hpp"
#include "./connection.hpp"
#include "./server.hpp"

template< typename TTask >
Server< TTask >::Server(
    std::string const & port
)
    : m_strand( m_ioService )
    , m_acceptor( m_ioService )
    , m_signals( m_ioService )
{
    m_signals.add( SIGINT );
    m_signals.add( SIGTERM );
    m_signals.add( SIGQUIT );

    m_signals.async_wait(
        boost::bind( & Server< TTask >::stop, this )
    );

    ip::tcp::resolver resolver( m_ioService );
    ip::tcp::resolver::query query( "127.0.0.1", port );
    ip::tcp::endpoint endpoint = * resolver.resolve( query );

    m_acceptor.open( endpoint.protocol() );
    m_acceptor.set_option( ip::tcp::acceptor::reuse_address( true ) );
    m_acceptor.bind( endpoint );
    m_acceptor.listen();

    startAccept();
}

template< typename TTask >
void Server< TTask >::run()
{
    boost::thread_group threadGroup; 

    auto const threadBody = boost::bind(
        & asio::io_service::run,
        & m_ioService
    );

    for( unsigned i = 0 ; i < 4 ; ++ i )
    {
        threadGroup.create_thread( threadBody );
    }

    threadGroup.join_all();
}

template< typename TTask >
void Server< TTask >::broadcast(
    ConnectionPtr const & sender,
    char const * const message,
    std::size_t const size
)
{
    auto const skipSender = [ & sender ]( ConnectionPtr const & connectionPtr )
    {
        return sender->socket().native_handle() != connectionPtr->socket().native_handle();
    };

    auto sendMessage = [ this, & sender, & size, & message ]( ConnectionPtr const & connectionPtr )
    {
        auto const continuation = boost::bind(
            & Connection< TTask >::doNothing,
            sender,
            placeholders::error
        );

        asio::async_write(
            connectionPtr->socket(),
            asio::buffer( message, size ),
            continuation
        );
    };

    m_connectionManager.forEachIf( skipSender, sendMessage );
}

template< typename TTask >
void Server< TTask >::unicast(
    ConnectionPtr const & sender,
    std::string const & receiverId,
    char const * const message,
    std::size_t const size
)
{
    auto const matchReceiver = [ this, & receiverId ]( ConnectionPtr const & connectionPtr )
    {
        return receiverId == connectionPtr->getId();
    };

    auto sendMessage = [ this, & sender, & size, & message ]( ConnectionPtr const & connectionPtr )
    {
        auto const continuation = boost::bind(
            & Connection< TTask >::doNothing,
            sender,
            placeholders::error
        );

        asio::async_write(
            connectionPtr->socket(),
            asio::buffer( message, size ),
            continuation
        );
    };

    m_connectionManager.forEachIf( matchReceiver, sendMessage );
}

template< typename TTask >
void Server< TTask >::disconnect(
    ConnectionPtr const & sender
)
{
    m_connectionManager.remove( sender );
}

template< typename TTask >
void Server< TTask >::startAccept()
{
    m_newConnection.reset( new Connection< TTask >( m_ioService, *this ) );

    auto const onAccepted = boost::bind(
        & Server< TTask >::onAccepted,
        this,
        placeholders::error
    );

    m_acceptor.async_accept(
        m_newConnection->socket(),
        onAccepted
    );
}

template< typename TTask >
void Server< TTask >::onAccepted(
    sys::error_code const & errorCode
)
{
    if( ! errorCode )
    {
        m_connectionManager.add( m_newConnection );

        m_newConnection->start( TTask::start() );
    }

    startAccept();
}

template< typename TTask >
void Server< TTask >::stop()
{
    m_ioService.stop();
}
