#include <iostream>

#include <boost/asio/io_context.hpp>
#include <repetier/service.hpp>

#include "fhem_service.hpp"

using namespace std;
using namespace prnet;
using namespace repw;

namespace asio = boost::asio;

int main( int argc, char const* const argv[] )
{
    if ( argc != 4 ) {
        cerr << "Usage: " << argv[ 0 ] << " <host> <port> <apikey>";
        return 1;
    }

    asio::io_context context;
    rep::settings settings { argv[ 1 ], argv[ 2 ], argv[ 3 ] };
    rep::service service { context, settings };
    fhem::service fhem { context, "speedstar", "8083" };

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

