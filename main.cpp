#include <iostream>

#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/version.hpp>
#include <boost/asio/connect.hpp>
#include <boost/asio/spawn.hpp>
#include <boost/asio/io_context.hpp>
#include <boost/asio/ip/tcp.hpp>

#include <repetier/service.hpp>

using namespace std;
using namespace prnet;

using tcp = boost::asio::ip::tcp;

namespace asio = boost::asio;
namespace http = boost::beast::http;

class fhem_service
{
public:
    fhem_service( asio::io_context& context, std::string host, std::string port );

    void setreading( std::string devspec, std::string reading, std::string value );

private:
    template< typename Callback >
    auto do_connected( asio::yield_context yield, Callback&& cb )
    {
        tcp::resolver resolver { context_ };
        auto resolved { resolver.async_resolve( host_, port_, yield ) };

        tcp::socket socket { context_ };
        asio::async_connect( socket, resolved, yield );

        return cb( socket );
    }

    std::string request_csrf_token( asio::yield_context yield );

    asio::io_context& context_;
    std::string host_;
    std::string port_;
};

fhem_service::fhem_service( asio::io_context& context, std::string host, std::string port )
        : context_ { context }
        , host_ { move( host ) }
        , port_ { move( port ) } {}

string fhem_service::request_csrf_token( asio::yield_context yield )
{
    return do_connected( yield, [&]( auto& socket ) {
        http::request< http::empty_body > request { http::verb::get, "/fhem?xhr=1", 11 };
        request.set( http::field::host, host_ );
        request.set( http::field::user_agent, BOOST_BEAST_VERSION_STRING );
        http::async_write( socket, request, yield );

        boost::beast::multi_buffer buffer;
        http::response< http::dynamic_body > response;
        http::async_read( socket, buffer, response, yield );

        return string( response[ "X-FHEM-csrfToken" ] );
    } );
}

void fhem_service::setreading( std::string devspec, std::string reading, std::string value )
{
    asio::spawn( context_, [this, devspec { move( devspec ) }, reading { move( reading ) }, value { move( value ) }]( asio::yield_context yield ) {
        do_connected( yield, [&]( auto& socket ) {
            auto target { "/fhem?fwcsrf=" + request_csrf_token( yield ) + "&cmd=setreading%20" + devspec + "%20" + reading + "%20" + value };

            http::request< http::empty_body > request { http::verb::get, target, 11 };
            request.set( http::field::host, host_ );
            request.set( http::field::user_agent, BOOST_BEAST_VERSION_STRING );
            http::async_write( socket, request, yield );

            boost::beast::multi_buffer buffer;
            http::response< http::dynamic_body > response;
            http::async_read( socket, buffer, response, yield );
        } );
    } );
}

int main( int argc, char const* const argv[] )
{
    if ( argc != 4 ) {
        cerr << "Usage: " << argv[ 0 ] << " <host> <port> <apikey>";
        return 1;
    }

    asio::io_context context;
    rep::settings settings { argv[ 1 ], argv[ 2 ], argv[ 3 ] };
    rep::service service { context, settings };
    fhem_service fhem { context, "speedstar", "8083" };

    service.job_started.connect( [&]( auto printer ) {
        fhem.setreading( "MakerPI", "printerState_" + printer, "printing" );
    } );
    service.job_finished.connect( [&]( auto printer ) {
        fhem.setreading( "MakerPI", "printerState_" + printer, "idle" );
    } );
    service.job_killed.connect( [&]( auto printer ) {
        fhem.setreading( "MakerPI", "printerState_" + printer, "idle" );
    } );
    service.temperature.connect( [&]( auto printer, auto info ) {
        fhem.setreading( "MakerPI", "printerTemp_" + printer + "_" + info.controller_name(), to_string( info.actual() ) );
    } );

    context.run();
}

