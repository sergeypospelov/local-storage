#include "log.h"

namespace NLogging {

////////////////////////////////////////////////////////////////////////////////

const LoggingEnv& logging_env()
{
    static LoggingEnv env;
    return env;
}

}   // namespace NLogging
