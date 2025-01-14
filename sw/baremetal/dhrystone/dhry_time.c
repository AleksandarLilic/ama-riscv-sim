#include "../common/common.h"

long time_s() { return (long)(get_cpu_time()/1000000); }
