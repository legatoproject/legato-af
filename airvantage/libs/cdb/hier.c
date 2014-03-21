#include "auto_home.h"

void hier()
{
  h(auto_home,-1,-1,02755);
  d(auto_home,"bin",-1,-1,02755);

  c(auto_home,"bin","cdbget",-1,-1,0755);
  c(auto_home,"bin","cdbmake",-1,-1,0755);
  c(auto_home,"bin","cdbdump",-1,-1,0755);
  c(auto_home,"bin","cdbstats",-1,-1,0755);
  c(auto_home,"bin","cdbtest",-1,-1,0755);
  c(auto_home,"bin","cdbmake-12",-1,-1,0755);
  c(auto_home,"bin","cdbmake-sv",-1,-1,0755);
}
