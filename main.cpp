#include <fstream>
#include <iostream>
#include <system_error>
#include <utility>

#include <boost/asio/io_context.hpp>
#include <nlohmann/json.hpp>
#include <core/logging.hpp>
#include <repetier/service.hpp>

#include "commandline.hpp"
#include "fhem_service.hpp"

using namespace nlohmann;
using namespace std;
using namespace prnet;

namespace asio = boost::asio;

namespace repw {

static logger logger { "repwatchd" };

static json read_properties( string const& fileName )
{
    ifstream ifs { fileName, ios::in };
    if ( !ifs ) {
        throw system_error( errno, system_category(), "couldn't open " + fileName );
    }

    json props;
    ifs >> props;
    return move( props );
}

static void update_temperature( fhem::service& fhem, string const& slug, rep::temperature const& temp )
{
    fhem.setreading( "MakerPI_input", slug + "_temperature_" + temp.controller_name(), to_string( temp.actual() ) );
}

static void update_printers( fhem::service& fhem, vector< rep::printer > const& printers )
{
    for ( auto const& printer : printers ) {
        fhem.setreading( "MakerPI_input", printer.slug() + "_state", string( to_string( printer.status() ) ) );
        fhem.setreading( "MakerPI_input", printer.slug() + "_job", printer.job() );
    }
}

void run( int argc, char* const argv[] )
{
    try {
        logger::threshold( logger::Debug );

        commandline args { argv, argc };
        if ( !args.log_file().empty() ) {
            logger::output( args.log_file().c_str() );
        }

        logger.info( "repwatchd starting" );

        auto props = read_properties( args.properties_file() );
        auto const& repetierNode = props.at( "repetier" );
        auto const& fhemNode = props.at( "fhem" );

        asio::io_context context;
        fhem::service fhem( context, fhemNode.at( "host" ), fhemNode.at( "port" ) );
        rep::settings settings( repetierNode.at( "host" ), repetierNode.at( "port" ), repetierNode.at( "apikey" ) );
        rep::service service( context, settings, [&]( auto ec ) {
            service.list_printers( [&]( auto printers ) {
                update_printers( fhem, printers );
            } );
        } );

        service.temperature.connect( [&]( auto slug, auto temp ) { update_temperature( fhem, slug, temp ); } );
        service.printers_changed.connect( [&]( auto printers ) { update_printers( fhem, printers ); } );

        context.run();
    } catch ( exception const& e ) {
        logger.error( e.what() );
    }
}

} // namespace repw

int main( int argc, char* const argv[] )
{
    repw::run( argc, argv );
}

