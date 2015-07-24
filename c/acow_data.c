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

#define LO_FACTOR  1.2
#define TRG_FACTOR 1.3
#define HI_FACTOR  1.4

/* ========================================================================= */
int realloc_data(
  ACOW_DATA* data,
  int num_accounts,
  int num_transactions
)
{
  if (num_accounts >= 0) {
    unsigned int trg_accounts = num_accounts > 0 ? 
      num_accounts : data->num_accounts;
    if ( (data->max_accounts > (HI_FACTOR*trg_accounts)) ||
         (data->max_accounts < (LO_FACTOR*trg_accounts)) ) {
      unsigned int new_max = TRG_FACTOR*trg_accounts*sizeof(ACOW_ACCOUNT);
      ACOW_ACCOUNT* new_acc = malloc(new_max);
      if (data->accounts) {
        memmove(new_acc,data->accounts,
                data->num_accounts*sizeof(ACOW_ACCOUNT));
        free(data->accounts);
      }
      data->accounts = new_acc;
      data->max_accounts = new_max;
    }
  }

  if (num_transactions >= 0) {
    unsigned int trg_transactions = num_transactions > 0 ? 
      num_transactions : data->num_transactions;
    if ( (data->max_transactions > (HI_FACTOR*trg_transactions)) ||
         (data->max_transactions < (LO_FACTOR*trg_transactions)) ) {
      unsigned int new_max = TRG_FACTOR*trg_transactions*sizeof(ACOW_TRANSACTION);
      ACOW_TRANSACTION* new_tra = malloc(new_max);
      if (data->transactions) {
        memmove(new_tra,data->transactions,
                data->num_transactions*sizeof(ACOW_TRANSACTION));
        free(data->transactions);
      }
      data->transactions = new_tra;
      data->max_transactions = new_max;
    }
  }

  return 0;
}

/* ========================================================================= */
int cmp_accounts(const void* a, const void* b)
{
  ACOW_ACCOUNT* aa = (ACOW_ACCOUNT*)a;
  ACOW_ACCOUNT* ab = (ACOW_ACCOUNT*)b;
  return (aa->id < ab->id) ? -1 : (aa->id > ab->id) ? 1 : 0;
}

/* ========================================================================= */
int cmp_transactions(const void* a, const void* b)
{
  ACOW_TRANSACTION* ta = (ACOW_TRANSACTION*)a;
  ACOW_TRANSACTION* tb = (ACOW_TRANSACTION*)b;
  return (ta->timedate < tb->timedate) ? -1 : (ta->timedate > tb->timedate) ? 1 : 0;
}

/* ========================================================================= */
int resort_data(ACOW_DATA* data)
{
  /* Sort accounts by ascending account id */
  qsort(data->accounts,data->num_accounts,
        sizeof(ACOW_ACCOUNT),cmp_accounts);

  /* Sort transactions by date */
  qsort(data->transactions,data->num_transactions,
        sizeof(ACOW_TRANSACTION),cmp_transactions);

  return 0;
}

/* ========================================================================= */
ACOW_ACCOUNT* lookup_account_id(ACOW_DATA* data, unsigned int id)
{
  int i;
  ACOW_ACCOUNT* acc;

  for (i=0; i < data->num_accounts; ++i) {
    acc = data->accounts + i;
    if (acc->id == id) return acc;
  }

  return 0;
}

/* ========================================================================= */
ACOW_ACCOUNT* lookup_account_name(ACOW_DATA* data, const char* name)
{
  int i;
  ACOW_ACCOUNT* acc;

  for (i=0; i < data->num_accounts; ++i) {
    acc = data->accounts + i;
    if (0 == strcmp(acc->name,name)) return acc;
  }

  return 0;
}

/* ========================================================================= */
int add_account(
  ACOW_DATA* data,
  unsigned int user,
  char* name,
  char* comment
)
{
  int i,len;
  unsigned int id=0;
  ACOW_ACCOUNT* acc;

  /* Check name is given */
  if (name[0] == 0) {
    return -1;
  }

  /* Check name isn't already used for an account */
  if (lookup_account_name(data,name) != 0) {
    return -2;
  }

  /* Find next available account id */
  for (i=0; i < data->num_accounts; ++i) {
    acc = data->accounts + i;
    id = (id > acc->id) ? id : acc->id;
  }
  ++id;
  
  /* Expand data if required */
  realloc_data(data,data->num_accounts+1,-1);

  /* Setup new account */
  acc = data->accounts + data->num_accounts;
  acc->id = id;
  acc->user = user;
  
  /* Copy name */
  len = strlen(name) + 1;
  acc->name = malloc(len);
  strncpy(acc->name,name,len);
    
  /* Copy comment, if given */
  if (comment[0]) {
    len = strlen(comment) + 1;
    acc->comment = malloc(len);
    strncpy(acc->comment,comment,len);
  } else {
    acc->comment = 0;
  }
  
  ++data->num_accounts;

  resort_data(data);

  return id;
}

/* ========================================================================= */
int remove_account(
  ACOW_DATA* data,
  unsigned int id
)
{
  int i;
  ACOW_ACCOUNT* acc;
  ACOW_TRANSACTION* tra;

  /* Find the account */
  for (i=0; i < data->num_accounts; ++i) {
    acc = data->accounts + i;
    if (acc->id == id) {
      break;
    }
  }

  if (i == data->num_accounts) {
    /* Couldn't find account to remove */
    return -1;
  }

  /* Check there are no transactions associated with account */
  for (i=0; i < data->num_transactions; ++i) {
    tra = data->transactions + i;
    if (tra->src == id || tra->dest == id) {
      /* Transaction is associated with this account, so can't remove it */
      return -2;
    }
  }  

  /* Copy back over the account to be removed */
  if (i < data->num_accounts-1) {
    memmove(data->accounts+i,data->accounts+i+1,
            (data->num_accounts-i-1)*sizeof(ACOW_ACCOUNT));
  }

  data->num_accounts--;
  realloc_data(data,0,-1);

  return 0;
}

/* ========================================================================= */
ACOW_TRANSACTION* lookup_transaction_id(ACOW_DATA* data, unsigned int id)
{
  int i;
  ACOW_TRANSACTION* tra;

  for (i=0; i < data->num_transactions; ++i) {
    tra = data->transactions + i;
    if (tra->id == id) return tra;
  }

  return 0;
}

/* ========================================================================= */
int add_transaction(
  ACOW_DATA* data,
  time_t timedate,
  unsigned int src,
  unsigned int dest,
  int value,
  char* comment
)
{
  int i,len;
  unsigned int id=0;
  ACOW_TRANSACTION* tra;

  /* Check src and dest accounts are valid */
  if (lookup_account_id(data,src) == 0 ||
      lookup_account_id(data,dest) == 0) {
    return -1;
  }

  /* Use current time if not given */
  if (timedate == 0) {
    time(&timedate);
  }

  /* Find next available transaction id */
  for (i=0; i < data->num_transactions; ++i) {
    tra = data->transactions + i;
    id = (id > tra->id) ? id : tra->id;
  }
  ++id;
  
  /* Expand data if required */
  realloc_data(data,-1,data->num_transactions+1);

  /* Setup new account */
  tra = data->transactions + data->num_transactions;
  tra->id = id;
  tra->timedate = timedate;
  tra->src = src;
  tra->dest = dest;
  tra->value = value;
  
  /* Copy comment, if given */
  if (comment[0]) {
    len = strlen(comment) + 1;
    tra->comment = malloc(len);
    strncpy(tra->comment,comment,len);
  } else {
    tra->comment = 0;
  }
  
  ++data->num_transactions;

  resort_data(data);
   
  return id;
}

/* ========================================================================= */
int remove_transaction(
  ACOW_DATA* data,
  unsigned int id
)
{
  int i;
  ACOW_TRANSACTION* tra;

  /* Find the transaction */
  for (i=0; i < data->num_transactions; ++i) {
    tra = data->transactions + i;
    if (tra->id == id) {
      break;
    }
  }

  if (i == data->num_transactions) {
    /* Couldn't find transaction to remove */
    return -1;
  }

  /* Copy back over the transaction to be removed */
  if (i < data->num_transactions-1) {
    memmove(data->transactions+i,data->transactions+i+1,
            (data->num_transactions-i-1)*sizeof(ACOW_TRANSACTION));
  }

  data->num_transactions--;
  realloc_data(data,-1,0);

  return 0;
}
