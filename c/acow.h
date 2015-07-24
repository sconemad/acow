/* acow - simple accounting program (http://www.sconemad.com)

Global definitions

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

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <time.h>

typedef struct {
  unsigned int id;   // Unique ID
  time_t timedate;   // Time/date of transaction
  unsigned int src;  // Source account ID
  unsigned int dest; // Destination account ID
  int value;         // Transaction value
  char* comment;     // Space for comment if required
} ACOW_TRANSACTION;

typedef struct {
  unsigned int id;   // Unique ID
  unsigned int user; // User ID (0 for external account)
  char* name;        // Account name
  char* comment;     // Space for comment if required
} ACOW_ACCOUNT;

typedef struct {
  unsigned int num_accounts;
  unsigned int max_accounts;
  ACOW_ACCOUNT* accounts;
  unsigned int num_transactions;
  unsigned int max_transactions;
  ACOW_TRANSACTION* transactions;
} ACOW_DATA;

enum AcowTransactionType {
  Credit,
  Debit,
  Checkpoint
};

/* ========================================================================= */
/* acow_calc */

/* Enumerate transactions in a given account */
int enumerate_transactions(
  ACOW_DATA* data,
  unsigned int acc_id,
  time_t start,
  time_t end,
  int (*transaction_callback)(
    ACOW_TRANSACTION* tra,
    enum AcowTransactionType type,
    int balance,
    void* user_data),
  void* user_data
);

/* Obtain the current balance for a given account */
int account_balance(
  ACOW_DATA* data,
  unsigned int acc_id
);


/* ========================================================================= */
/* acow_data */

/* Resize data arrays as required */
int realloc_data(
  ACOW_DATA* data,
  int num_accounts,
  int num_transactions
);

/* Re-sort the data arrays */
int resort_data(
  ACOW_DATA* data
);

/* Lookup account by id or name */
ACOW_ACCOUNT* lookup_account_id(ACOW_DATA* data, unsigned int id);
ACOW_ACCOUNT* lookup_account_name(ACOW_DATA* data, const char* name);

/* Add an account */
int add_account(
  ACOW_DATA* data,
  unsigned int user,
  char* name,
  char* comment
);

/* Remove an account */
int remove_account(
  ACOW_DATA* data,
  unsigned int id
);

/* Lookup transaction by id */
ACOW_TRANSACTION* lookup_transaction_id(ACOW_DATA* data, unsigned int id);

/* Add a transaction */
int add_transaction(
  ACOW_DATA* data,
  time_t timedate,
  unsigned int src,
  unsigned int dest,
  int value,
  char* comment
);

/* Remove a transaction */
int remove_transaction(
  ACOW_DATA* data,
  unsigned int id
);

/* ========================================================================= */
/* acow_file */

/* Load from a file */
int load_file(
  ACOW_DATA* data,
  const char* path
);

/* Save to a file */
int save_file(
  ACOW_DATA* data,
  const char* path
);  
