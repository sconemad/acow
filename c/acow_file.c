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

/* ========================================================================= */
int load_file(
  ACOW_DATA* data,
  const char* path
)
{
  unsigned int id,user,src,dest;
  char line[1024], name[256], comment[1024];
  int value,len,err;
  unsigned int timedate;
  ACOW_ACCOUNT* acc=0;
  ACOW_TRANSACTION* tra=0;
  enum {Unknown,Accounts,Transactions} state = Unknown;
  
  FILE* file = fopen(path,"r");
  if (file == 0) return errno;

  while (fscanf(file,"%1023[^\r\n]\n",line) > 0) {

    if (0 == strcmp(line,"[accounts]")) {
      /* Initialise accounts reading */
      state = Accounts;

    } else if (0 == strcmp(line,"[transactions]")) {
      /* Initialise transactions reading */
      state = Transactions;

    } else  if (state == Accounts) {
      /* Read account */
      err = sscanf(line,"%u %u \"%255[^\"]\" \"%1023[^\"]\"",
                   &id,&user,name,comment);
      if (err == 4) {
        realloc_data(data,data->num_accounts+1,-1);
        acc = data->accounts + data->num_accounts;
        acc->id = id;
        acc->user = user;
        
        if (name[0]) {
          len = strlen(name) + 1;
          acc->name = malloc(len);
          strncpy(acc->name,name,len);
        } else {
          acc->name = 0;
        }
        
        if (comment[0]) {
          len = strlen(comment) + 1;
          acc->comment = malloc(len);
          strncpy(acc->comment,comment,len);
        } else {
          acc->comment = 0;
        }
        
        ++data->num_accounts;
      }

    } else if (state == Transactions) {
      /* Read transaction */
      err = sscanf(line,"%u %u %u %u %d \"%1023[^\"]\"",
                   &id,&timedate,&src,&dest,&value,comment);
      if (err == 6) {
        realloc_data(data,-1,data->num_transactions+1);
        tra = data->transactions + data->num_transactions;
        tra->id = id;
        tra->timedate = timedate;
        tra->src = src;
        tra->dest = dest;
        tra->value = value;
        
        if (comment[0]) {
          len = strlen(comment) + 1;
          tra->comment = malloc(len);
          strncpy(tra->comment,comment,len);
        } else {
          tra->comment = 0;
        }
        ++data->num_transactions;
      }

    } else {
      /* Unknown element in file */
      printf("ERROR: %s\n",line);
    }
  }

  fclose(file);
  return 0;
}


/* ========================================================================= */
int save_file(
  ACOW_DATA* data,
  const char* path
)
{
  int i;
  ACOW_ACCOUNT* acc;
  ACOW_TRANSACTION* tra;
  FILE* file = fopen(path,"w");

  fprintf(file,"[accounts]\n");
  for (i=0; i < data->num_accounts; ++i) {
    acc = data->accounts + i;
    fprintf(file,"%u %u \"%s\" \"%s\"\n",
           acc->id,acc->user,acc->name,acc->comment);
  }

  fprintf(file,"[transactions]\n");
  for (i=0; i < data->num_transactions; ++i) {
    tra = data->transactions + i;
    fprintf(file,"%u %u %u %u %d \"%s\"\n",
            tra->id,(int)tra->timedate,tra->src,tra->dest,tra->value,tra->comment);
  }

  fclose(file);
  return 0;
}
