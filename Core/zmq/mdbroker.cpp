
#include "mdbroker.hpp"


// With this one we can have non homogeneous servers
// e.g: not loading some plugin (like a plugin requiring GPU, which may not be available on the computer
// so this worker will not try to call the plugin

