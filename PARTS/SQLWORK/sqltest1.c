/*
    mysql test.

*/


#include <my_global.h>
#include <mysql.h>

int main(int argc, char **argv)
{
  printf("MySQL client version: %s\n", mysql_get_client_info());

  exit(0);
}


gcc test1.c -o test1  `mysql_config --cflags --libs`