#include <algorithm>
#include <unordered_map>
#include <utility>

#include <3dprnet/core/logging.hpp>
#include <mosquittopp.h>

#include "session.hpp"

using namespace std;
using namespace prnet;

namespace repw {
namespace mqtt {

static prnet::Logger logger( "mqtt::Session" );

class Session::Impl
        : mosqpp::mosquittopp
{
    using Lock = lock_guard< recursive_mutex >;

public:
    Impl( Endpoint const& endpoint )
            : mosquittopp( endpoint.clientId().c_str() )
    {
        call_once( initialized, [] { mosqpp::lib_init(); } );

        logger.info( "connecting to ", endpoint.host(), ":", endpoint.port() );

        connect_async( endpoint.host().c_str(), endpoint.port() );
        check( "loop", loop_start() );
    }

    ~Impl()
    {
        loop_stop( true );
    }

    void publish( string const& topic, string const& payload )
    {
        check( "publish", mosquittopp::publish( nullptr, topic.c_str(), static_cast< int >( payload.length() ), payload.data() ) );
    }

    void subscribe( string&& topic, MessageHandler&& handler )
    {
        if ( check( "subscribe", mosquittopp::subscribe( nullptr, topic.c_str() ) ) ) {
            subscriptions_.emplace( move( topic ), move( handler ) );
        }
    }

private:
    bool check( char const* what, int rc )
    {
        if ( rc == MOSQ_ERR_SUCCESS ) {
            return true;
        }
        logger.error( "mqtt error at", what, ": ", rc );
        return false;
    }

    void on_connect( int rc ) override
    {
        if ( check( "connect", rc ) ) {
            logger.info( "connection established successfully" );
        }
    }

    void on_disconnect( int rc ) override
    {
        logger.error( "connection lost due to mosquitto error ", rc, ", reconnecting" );
        reconnect_async();
    }

    void on_log( int level, char const* str ) override
    {
        logger.debug( "MQTT[", level, "] ", str );
    }

    void on_message( mosquitto_message const* message ) override
    {
        logger.debug( "received message from ", message->topic, " with mid ", message->mid );

        auto subs = subscriptions_.equal_range( message->topic );
        if ( subs.first != subs.second ) {
            string payload( static_cast< char* >( message->payload ), static_cast< size_t >( message->payloadlen ) );
            for_each( subs.first, subs.second, [&]( auto const& sub ) { sub.second( payload ); } );
        }
    }

    static once_flag initialized;

    unordered_multimap< string, MessageHandler > subscriptions_;
};

once_flag Session::Impl::initialized;

Session::Session( Endpoint const& endpoint )
        : impl_( make_unique< Impl >( endpoint ) ) {}

Session::~Session() = default;

void Session::publish( string const &topic, string const &payload )
{
    impl_->publish( topic, payload );
}

void Session::subscribe( string topic, MessageHandler handler )
{
    impl_->subscribe( move( topic ), move( handler ) );
}

} // namespace mqtt
} // namespace repw
