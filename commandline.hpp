#ifndef REPWATCHD_COMMANDLINE_HPP
#define REPWATCHD_COMMANDLINE_HPP

#include <string>

namespace repw {

class commandline
{
public:
    commandline( char* const* argv, int argc );

    std::string const& properties_file() const { return propertiesFile_; }
    std::string const& log_file() const { return logFile_; }
    std::string const& pid_file() const { return pidFile_; }
    bool daemon() const { return daemon_; }

private:
    std::string propertiesFile_;
    std::string logFile_;
    std::string pidFile_;
    bool daemon_ {};
};

} // namespace repw

#endif // REPWATCHD_COMMANDLINE_HPP
