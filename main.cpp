#include <algorithm>
#include <fstream>
#include <iostream>
#include <system_error>
#include <unordered_map>
#include <unordered_set>
#include <utility>

#include <3dprnet/core/encoding.hpp>
#include <3dprnet/core/logging.hpp>
#include <3dprnet/core/optional.hpp>
#include <3dprnet/repetier/service.hpp>
#include <3dprnet/repetier/types.hpp>
#include <boost/asio/io_context.hpp>
#include <boost/lexical_cast.hpp>
#include <nlohmann/json.hpp>

#include "commandline.hpp"
#include "mqtt/session.hpp"
#include "wiring/output.hpp"

using namespace nlohmann;
using namespace std;
using namespace prnet;

namespace asio = boost::asio;

namespace repw {

static Logger logger( "repwatchd" );

struct PrinterConfig
{
    optional< wiring::Output > powerPin;
    bool online;
    bool heatbed;
    bool configured;
};

unique_ptr< rep::Service > repetier;
unique_ptr< mqtt::Session > broker;
unordered_map< string, PrinterConfig > configs;

json read_properties( string const& fileName )
{
    ifstream ifs { fileName, ios::in };
    if ( !ifs ) {
        throw system_error( errno, system_category(), "couldn't open " + fileName );
    }

    json props;
    ifs >> props;
    return move( props );
}

void handle_mqtt_power( string const& slug, string const& value )
{
    bool boolValue;
    if ( value == "ON" ) {
        boolValue = true;
    } else if ( value == "OFF" ) {
        boolValue = false;
    } else {
        logger.warning( "unknown power value ", value );
        return;
    }

    logger.info( "switching power for ", slug, " to ", value );

    configs.at( slug ).powerPin->set( boolValue );
}

void handle_mqtt_heatbed_temp( string const& slug, string const& value )
{
    logger.info( "setting heatbed temperature of ", slug, " to ", value );
    repetier->sendCommand( slug, "M140 S" + value );
}

void handle_temperature( string const& slug, rep::Temperature const& temperature )
{
    if ( temperature.controller() == rep::Temperature::heatbed ) {
        broker->publish( "stat/Printers/" + slug + "/Heatbed/SETTEMP", to_string( temperature.wanted() ) );
        broker->publish( "stat/Printers/" + slug + "/Heatbed/GETTEMP", to_string( temperature.actual() ) );
    }
}

void handle_config( string const& slug, rep::PrinterConfig const& printerConfig )
{
    auto& config = configs[ printerConfig.slug() ];
    if ( !config.configured ) {
        if ( config.powerPin != nullopt ) {
            broker->subscribe( "cmnd/Printers/" + printerConfig.slug() + "/POWER",
                               [slug = printerConfig.slug()]( auto payload ) { handle_mqtt_power( slug, payload ); } );
        }
        if ( printerConfig.heatbed() != nullopt ) {
            config.heatbed = true;
            broker->subscribe( "cmnd/Printers/" + printerConfig.slug() + "/Heatbed/SETTEMP",
                               [slug = printerConfig.slug()]( auto payload ) { handle_mqtt_heatbed_temp( slug, payload ); } );
            if ( !config.online ) {
                broker->publish( "stat/Printers/" + printerConfig.slug() + "/Heatbed/GETTEMP", "none" );
            }
        }
        config.configured = true;
    }
}

void handle_printers( vector< rep::Printer > const& printers )
{
    for ( auto const& printer : printers ) {
        auto& config = configs[ printer.slug() ];
        if ( !config.configured ) {
            repetier->request_config( printer.slug() );
        }

        config.online = printer.online();
        broker->publish( "stat/Printers/" + printer.slug() + "/POWER", printer.online() ? "ON" : "OFF" );
        broker->publish( "stat/Printers/" + printer.slug() + "/JOB", printer.job() );
        if ( !printer.online() && config.heatbed ) {
            broker->publish( "stat/Printers/" + printer.slug() + "/Heatbed/GETTEMP", "none" );
        }
    }
}

void run( int argc, char* const argv[] )
{
    try {
        Logger::threshold( Logger::Level::debug );

        commandline args { argv, argc };
        if ( !args.log_file().empty() ) {
            Logger::output( args.log_file().c_str() );
        }

        logger.info( "repwatchd starting" );

        auto props = read_properties( args.properties_file() );

        auto powerPins = props.value( "powerPins", json::object() );
        for ( auto powerPin = powerPins.begin() ; powerPin != powerPins.end() ; ++powerPin ) {
            logger.info( "configuring printer ", powerPin.key(), " from properties" );

            PrinterConfig config = { wiring::Output( powerPin.value().get< int >() ) };
            configs.emplace( powerPin.key(), move( config ) );
        }

        asio::io_context context;
        repetier = make_unique< rep::Service >( context, props.at( "repetier" ) );
        broker = make_unique< mqtt::Session >( props.at( "mqtt" ).get< mqtt::Endpoint >() );

        repetier->on_disconnect( [=]( auto ec ) {
            repetier->request_printers();
        } );
        repetier->on_temperature( []( auto slug, auto temperature ) { handle_temperature( slug, temperature ); } );
        repetier->on_config( []( auto slug, auto printerConfig ) { handle_config( slug, printerConfig ); } );
        repetier->on_printers( []( auto printers ) { handle_printers( printers ); } );
        repetier->request_printers();

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

