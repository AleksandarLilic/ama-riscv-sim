#include "../common/common.h"

long time_s() { return (long)(time_us()/1000000); }
