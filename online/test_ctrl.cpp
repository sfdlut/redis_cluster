#include <stdlib.h>

#include <boost/lexical_cast.hpp>

#include "../src/RedisCtrl.h"
using namespace std;

LogOut g_log;

int main (int argc, char **argv)
{
  RedisCtrl rc(&g_log, "10.2.72.12:4180,10.2.72.12:4181,10.2.72.12:4182", "/tx/online");
  rc.start();

  for (;;) {
    rc.check();
    sleep(2);
  }

	return 0;
}

