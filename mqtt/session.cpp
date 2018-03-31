#include <algorithm>
#include <stdexcept>
#include <unordered_map>
#include <utility>

#include <3dprnet/core/logging.hpp>
#include <mosquittopp.h>

#include "session.hpp"

using namespace std;
using namespace prnet;
using namespace mosqpp;

namespace repw {
namespace mqtt {

static Logger logger( "mqtt::Session" );

class Session::SessionImpl
        : mosquittopp
{
    using Lock = lock_guard< mutex >;
    using Subscriptions = unordered_multimap< string, MessageHandler >;

public:
    SessionImpl( Endpoint endpoint )
            : mosquittopp( endpoint.clientId().c_str() )
    {
        call_once( initialized, [] { lib_init(); } );
        connect( endpoint );
    }

    ~SessionImpl()
    {
        mosquittopp::loop_stop( true );
    }

    void publish( std::string const& topic, string_view payload )
    {
        Lock lock( mutex_ );

        int rc;
        if ( ( rc = mosquittopp::publish( nullptr, topic.c_str(), static_cast< int >( payload.length() ), payload.data() ) )
                != MOSQ_ERR_SUCCESS ) {
            logger.error( "error publishing to ", topic, ": ", strerror( rc ) );
        }
    }

    void subscribe( string&& topic, MessageHandler&& handler )
    {
        Lock lock( mutex_ );

        auto inserted = subscriptions_.emplace( move( topic ), move( handler ) );
        if ( connected_ ) {
            resubscribe( inserted );
        }
    }

private:
    void connect( Endpoint const& endpoint )
    {
        logger.info( "connecting to ", endpoint.host(), ":", endpoint.port() );

        int rc;
        if ( ( rc = connect_async( endpoint.host().c_str(), endpoint.port() ) ) != MOSQ_ERR_SUCCESS ||
                ( rc = loop_start() ) != MOSQ_ERR_SUCCESS ) {
            throw runtime_error( "error starting MQTT session with " + endpoint.host() + ": " + strerror( rc ) );
        }
    }

    void resubscribe( Subscriptions::iterator it = everything )
    {
        auto first = it != everything ? it : subscriptions_.begin();
        auto last = it != everything ? it : subscriptions_.end();
        for_each( first, last, [this]( auto& subscription ) {
            int rc;
            if ( ( rc = this->mosquittopp::subscribe( nullptr, subscription.first.c_str() ) ) != MOSQ_ERR_SUCCESS ) {
                logger.error( "error subscribing to MQTT topic ", subscription.first, ": ", strerror( rc ) );
            }
        } );
    }

    void on_connect( int rc ) override
    {
        Lock lock( mutex_ );

        if ( rc != MOSQ_ERR_SUCCESS ) {
            logger.error( "error connecting to MQTT broker: ", strerror( rc ), ", trying to reconnect" );
            mosquittopp::reconnect_async();
            return;
        }

        logger.info( "connection to MQTT broker established successfully" );

        connected_ = true;
        resubscribe();
    }

    void on_disconnect( int rc ) override
    {
        Lock lock( mutex_ );

        logger.error( "connection to MQTT broker lost: ", strerror( rc ), ", trying to reconnect" );
        
        connected_ = false;
        mosquittopp::reconnect_async();
    }

    void on_message( mosquitto_message const* message ) override
    {
        Lock lock( mutex_ );

        logger.debug( "received message from ", message->topic, " with mid ", message->mid );

        string_view payload( static_cast< char* >( message->payload ), static_cast< size_t >( message->payloadlen ) );
        auto subscriptions = subscriptions_.equal_range( message->topic );
        for_each( subscriptions.first, subscriptions.second, [&]( auto const& subscription ) { subscription.second( payload ); } );
    }

    static once_flag initialized;
    static Subscriptions::iterator everything;

    bool connected_ {};
    Subscriptions subscriptions_;
    mutex mutex_;
};

once_flag Session::SessionImpl::initialized;
Session::SessionImpl::Subscriptions::iterator Session::SessionImpl::everything {};

Session::Session( Endpoint const& endpoint )
        : impl_( make_unique< SessionImpl >( endpoint ) ) {}

Session::~Session() = default;

void Session::publish( std::string const& topic, string_view payload )
{
    impl_->publish( topic, payload );
}

void Session::subscribe( string topic, MessageHandler handler )
{
    impl_->subscribe( move( topic ), move( handler ) );
}

} // namespace mqtt
} // namespace repw
