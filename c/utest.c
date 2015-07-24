/* acow - simple accounting program (http://www.sconemad.com)

utest - Acow unit tests

Copyright (c) 2005-2007 Andrew Wedgbury <wedge@sconemad.com>

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2, or (at your option)
any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program (see the file COPYING); if not, write to the
Free Software Foundation, Inc.,
59 Temple Place - Suite 330, Boston, MA  02111-1307, USA */

#include "acow.h"

const char filename[] = "test.acow";

#define UTEST(expected,cmd) \
  printf("UTEST: running " #cmd "\n"); \
  err = cmd; \
  if (err != expected) {\
    printf("\n### TEST FAILED got %d, expected %d, errno %d ###\n", \
           err,expected,errno); \
    exit(1); \
  }

/* ========================================================================= */
int main(int argc,char* argv[])
{

  int err;
  ACOW_DATA data;
  ACOW_ACCOUNT* acc;
  memset(&data,0,sizeof(ACOW_DATA));
  data.accounts = 0;
  data.transactions = 0;

  //  load_file(&data,filename);
  
  UTEST(1,add_account(&data,1,"current","My current account"));
  UTEST(2,add_account(&data,1,"savings","My savings account"));

  UTEST(3,add_account(&data,0,"wizzycorp","Generic company/employer"));
  UTEST(4,add_account(&data,0,"bank loan","A loan from the bank"));

  UTEST(5,add_account(&data,0,"ultra markt","Generic store 1"));
  UTEST(6,add_account(&data,0,"price busterz","Generic store 2"));
  UTEST(7,add_account(&data,0,"expensives r us","Generic store 3"));

  UTEST(1,add_transaction(&data,0,2,2,4321,"Initial savings balance"));
  UTEST(2,add_transaction(&data,0,4,1,9999,"Take out bank loan"));

  UTEST(3,add_transaction(&data,0,1,2,123,"Save one twenty three"));

  UTEST(4,add_transaction(&data,0,1,5,1,"Spend a penny"));
  UTEST(5,add_transaction(&data,0,1,6,100,"Spend a pound"));
  UTEST(6,add_transaction(&data,0,1,7,10000,"Spend lots"));

  UTEST(7,add_transaction(&data,0,2,1,20000,"Plunder savings"));

  UTEST(0,remove_transaction(&data,4));
  UTEST(-1,remove_transaction(&data,4));

  save_file(&data,"test.acow");

  printf("\nTEST PASSED!\n");
  exit(0);
}
