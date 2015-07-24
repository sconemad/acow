/* acow - simple accounting program (http://www.sconemad.com)

acow_file - File operations

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
#include <getopt.h>

const char filename[] = "test.acow";

/* ========================================================================= */
int list_account_callback(
  ACOW_TRANSACTION* tra,
  enum AcowTransactionType type,
  int balance,
  void* user_data
)
{
  ACOW_DATA* data = (ACOW_DATA*)user_data;
  ACOW_ACCOUNT* acc =0;
  struct tm tmstruct;
  char tmstring[32];

  /* Convert time to a string */
  localtime_r(&tra->timedate,&tmstruct);
  strftime(tmstring,31,"%Y-%m-%d",&tmstruct);
      
  switch (type) {
    case Checkpoint:
      acc = lookup_account_id(data,tra->dest);
      printf("%6u %s  =BALANCE CHECKPOINT=                               %9.2f=\n",
             tra->id,tmstring,
             balance*0.01);
      break;

    case Debit:
      acc = lookup_account_id(data,tra->dest);
      printf("%6u %s  %-30s %9.2f           %9.2f\n",
             tra->id,tmstring,acc->name,
             tra->value*0.01,balance*0.01);
      break;
      
    case Credit:
      acc = lookup_account_id(data,tra->src);
      printf("%6u %s  %-30s           %9.2f %9.2f\n",
             tra->id,tmstring,acc->name,
             tra->value*0.01,balance*0.01);
      break;
  }
}

/* ========================================================================= */
int list_account(ACOW_DATA* data, unsigned int acc_id)
{
  int i,num=0;
  ACOW_TRANSACTION* tra;
  ACOW_ACCOUNT* acc;
  int balance=0,debit_tot=0,credit_tot=0;

  acc = lookup_account_id(data,acc_id);
  printf("Listing transactions for account: %s (ID=%u)\n\n",
         acc->name,acc_id);

  printf("    ID    DATE     PAYEE                              DEBIT    CREDIT   BALANCE\n");
  printf("--------------------------------------------------------------------------------\n");

  enumerate_transactions(data,acc_id,0,0,list_account_callback,data);
 
  printf("--------------------------------------------------------------------------------\n");
}

/* ========================================================================= */
int list_all_accounts(ACOW_DATA* data)
{
  int i;
  ACOW_ACCOUNT* acc;

  printf("    ID  NAME                             BALANCE\n");
  printf("--------------------------------------------------------------------------------\n");

  for (i=0; i < data->num_accounts; ++i) {
    acc = data->accounts + i;
    if (0 != acc->user) {
      printf( "%6u  %-30s %9.2f\n",
              acc->id,
              acc->name,
              account_balance(data,acc->id)*0.01 );
    }
  }

  return 0;
}

/* ========================================================================= */
time_t parse_date(const char* str)
{
  struct tm tmstruct;
  memset(&tmstruct,0,sizeof(struct tm));

  if (3 != sscanf(str,"%d-%d-%d",
                  &tmstruct.tm_year,&tmstruct.tm_mon,&tmstruct.tm_mday)) {
    return -1;
  }

  /* Month should be zero-based */
  --tmstruct.tm_mon;

  /* Years start at 1900 */
  tmstruct.tm_year -= 1900;

  /* Turn it into a time_t */
  return mktime(&tmstruct);
}

/* ========================================================================= */
int main(int argc,char* argv[])
{
  int c=0;
  int err=0;
  ACOW_DATA data;

  /* options */
  extern char* optarg;
  ACOW_ACCOUNT* acc=0;
  ACOW_ACCOUNT* pay=0;
  time_t datetime=0;
  int value=0;
  unsigned int id=0;

  memset(&data,0,sizeof(ACOW_DATA));
  data.accounts = 0;
  data.transactions = 0;

  char* cmd = argv[1];
  if (!cmd) exit(0);

  load_file(&data,filename);

  while ((c = getopt(argc,argv,"?a:d:t:p:v:c:f:i:e")) >= 0) {
    switch (c) {
      case '?': 
        printf("HELP\n"); 
        exit(1);
        break;

      case 'a': // Account 
        acc = lookup_account_name(&data,optarg); 
        if (acc == 0) {
          printf("ERROR: Unknown account '%s'\n",optarg);
          exit(1);
        }
        break;

      case 'd': // Date
        datetime = parse_date(optarg);
        if (datetime < 0) {
          printf("ERROR: Invalid date '%s'\n",optarg);
          exit(1);
        }
        break;

      case 't': // Type
        break;

      case 'p': // Payee
        pay = lookup_account_name(&data,optarg); 
        if (pay == 0) {
          printf("ERROR: Unknown payee '%s'\n",optarg);
          exit(1);
        }
        break;

      case 'v': // Value
        if (1 != sscanf(optarg,"%d",&value)) {
          printf("ERROR: Invalid transaction value '%s'\n",optarg);
          exit(1);
        }
        break;

      case 'c': // Comment
        break;
      case 'f': // File
        break;

      case 'i': // ID
        if (1 != sscanf(optarg,"%u",&id)) {
          printf("ERROR: Invalid transaction id '%s'\n",optarg);
          exit(1);
        }
        break;
      case 'e': // Extended
        break;
    }
  }

  if (0 == strcmp(cmd,"ls") ||
      0 == strcmp(cmd,"list")) {
    if (acc == 0) {
      printf("ERROR: No account specified\n");
      exit(1);
    }
    list_account(&data,acc->id);

  } else if (0 == strcmp(cmd,"acc")) {
    list_all_accounts(&data);

  } else {
    printf("ERROR: Unknown command '%s'\n",cmd);
    exit(1);
  }

  save_file(&data,filename);

  exit(0);
}
