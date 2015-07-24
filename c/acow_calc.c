/* acow - simple accounting program (http://www.sconemad.com)

acow_calc - Account calculations

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
#include <limits.h>

/* ========================================================================= */
int enumerate_transactions(
  ACOW_DATA* data,
  unsigned int acc_id,
  time_t start,
  time_t end,
  int (*transaction_callback)(
    ACOW_TRANSACTION*,
    enum AcowTransactionType,
    int,
    void*),
  void* user_data
)
{
  int i,num=0;
  ACOW_TRANSACTION* tra;
  int balance=0;
  enum AcowTransactionType type;

  if (end == 0) end = INT_MAX;

  for (i=0; i < data->num_transactions; ++i) {
    tra = data->transactions + i;
    if (tra->timedate > end) break;

    if (tra->src == acc_id || tra->dest == acc_id) {
      /* This transaction affects this account*/
      if (tra->src == acc_id && tra->dest == acc_id) {
        balance = tra->value; /* Checkpoint */
        type = Checkpoint;
      } else if (tra->src == acc_id) {
        balance -= tra->value; /* Debit */
        type = Debit;
      } else if (tra->dest == acc_id) {
        balance += tra->value; /* Credit */
        type = Credit;
      }

      // Call user-supplied function if the transaction is
      // within the required date range
      if (tra->timedate >= start) {
        ++num;
        transaction_callback(tra,type,balance,user_data);
      }
    }
  }

  return num;
}

/* ========================================================================= */
int account_balance_tra_callback(
  ACOW_TRANSACTION* tra,
  enum AcowTransactionType type,
  int balance,
  void* user_data
)
{
  *((int*)user_data) = balance;
}

/* ========================================================================= */
int account_balance(
  ACOW_DATA* data,
  unsigned int acc_id
)
{
  int balance =0;
  enumerate_transactions(data,acc_id,0,0,account_balance_tra_callback,&balance);
  return balance;
}

