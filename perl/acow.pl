#!/usr/bin/perl -w
#
# acow - simple accounting (http://www.sconemad.com/acow)
#
# Copyright (c) 2005-2007 Andrew Wedgbury <wedge@sconemad.com>
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2, or (at your option)
# any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program (see the file COPYING); if not, write to the
# Free Software Foundation, Inc.,
# 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA

use DBI;
use Getopt::Std;

my $ACOW_VER = "0.1.0";
my $RCFILE = $ENV{HOME}.'/.acowrc';

my $compress = 0;
my $MFMT = "%9.02f";
my $TRANS_FMT = "%6d %s  %-30s %9s %9s %9s\n";
my $ACC_FMT = " %-32s %5d $MFMT\n";

my $LINE = ('-' x 80)."\n";
my $MIN_PAYEE = 1000;
my $PROMPT = "\nacow> ";

#===================================================================[ START ]==
# Check for simple standard info options
exit do_help() if (@ARGV==1 && $ARGV[0] =~ /^\-*(h|\?)/);
exit do_ver() if (@ARGV==1 && $ARGV[0] =~ /^\-*ver/);

# Set defaults then read settings from rcfile
my %rcopts;
$rcopts{dbuser} = $ENV{USER};
$rcopts{dbpasswd} = '';
$rcopts{dbhost} = 'localhost';
read_rcfile();
$rcopts{dbname} = 'acow_'.$rcopts{dbuser} if (!$rcopts{dbname});

# Connect to database
my $db = 'dbi:mysql:dbname='.$rcopts{dbname}.';host='.$rcopts{dbhost};
my $dbh = DBI->connect($db,$rcopts{dbuser},
		       $rcopts{dbpasswd},
		       { RaiseError => 1, AutoCommit => 0 });
die "Cannot connect to database" if (!$dbh);
do_init_tables();

my %opts;
my $account = 'current';
my $payee;
my $date;
my $type = 'd';
my $value;
my $comment = '';

if (@ARGV) {
    # Single command
    do_cmd();

} else {
    # Interactive multiple commands
    print $PROMPT;
    while (<>) {
	@ARGV = ();
	foreach my $arg (split(/(\"[^\"]*\")/,$_)) {
	    if ($arg =~ /\"([^\"]*)\"/) {
		push(@ARGV,$1);
	    } else {
		push(@ARGV,split(' ',$arg));
	    }
	}
	last if (do_cmd());
	print $PROMPT;
    }
}

$dbh->disconnect();


#----------------------------------------------------------
sub do_cmd
{
    if (scalar @ARGV == 0) {
	return 0;
    }
    my $cmd = '';
    $cmd = shift(@ARGV) if ($ARGV[0] !~ /^\-/);
    %opts = ();
    if (!getopts('a:d:t:p:v:c:f:i:e',\%opts)) {
	return 0;
    }

    # Get options or defaults
    $account = $opts{a} if ($opts{a});
    $payee = $opts{p};
    $date = $opts{d} if ($opts{d});
    $type = $opts{t} || 'd';
    $value = $opts{v};
    $comment = $opts{c} || '';
    
    # Commands
    if ($cmd eq 'help') {
	do_help();
    } elsif ($cmd =~ /^ver/) {
	do_ver();
    } elsif ($cmd eq 'add') {
	do_add();
    } elsif ($cmd =~ /^del/) {
	do_delete();
    } elsif ($cmd =~ /^edi/) {
	do_edit();
    } elsif ($cmd =~ /^cop/) {
	do_copy();
    } elsif ($cmd =~ /^lis/ || $cmd eq 'ls') {
	do_list();
    } elsif ($cmd =~ /^inf/) {
	do_info();
    } elsif ($cmd =~ /^acc/) {
	do_accounts(1);
    } elsif ($cmd =~ /^pay/) {
	do_accounts(0);
    } elsif ($cmd =~ /^imp/) {
	do_import();
    } elsif ($cmd eq 'compress') {
	$compress = 1;
	$MFMT = "%.2f";
	$TRANS_FMT = "%d,%s,%s,%s,%s,%s\n";
	$ACC_FMT = "%s,%d,$MFMT\n";
    } elsif ($cmd eq 'exit' || $cmd eq 'quit') {
	return 1;
    } else {
	print "ERROR: Unknown command\n";
    }
    return 0;
}

#----------------------------------------------------------
sub do_add
{
    # Date - default to today's date
    if (!defined $date) {
	my @t = localtime();
	$date = sprintf("%04d-%02d-%02d",1900+$t[5],1+$t[4],$t[3]);
    }
  
    # Payee
    if (!defined $payee && $type ne 'b') {
	print "ERROR: No payee account specified\n";
	return 1;
    }

    # Value
    if (!defined $value) {
	print "ERROR: No value specified\n";
	return 1;
    }
    
    my $account_id = lookup_account($account,0);
    if (!defined $account_id) {
	print "ERROR: Unknown account '$account'\n";
	return 1;
    }
    my $payee_id = $payee ? lookup_account($payee,1) : 0;
    my $src = ($type ne 'c') ? $account_id : $payee_id;
    my $dst = ($type ne 'd') ? $account_id : $payee_id;
    $value *= 100;

    $dbh->do(
      "INSERT INTO transaction (date, acc_src, acc_dst, value, comment) ".
      "VALUES (\"$date\", $src, $dst, $value, \"$comment\")");
}

#----------------------------------------------------------
sub do_delete
{
    my $id = $opts{i};
    if (!defined $id) {
	print "ERROR: Must specify transaction ID to delete\n";
	return 1;
    }
    my $r = $dbh->do(
      "DELETE FROM transaction WHERE id = $id");

    if ($r != 1) {
	print "ERROR: Unable to delete transaction $id\n";
	return 1;
    }
}

#----------------------------------------------------------
sub do_edit
{
    my $id = $opts{i};
    if (!defined $id) {
	print "ERROR: Must specify transaction ID to edit\n";
	return 1;
    }

    if (defined $opts{d}) {
	my $r = $dbh->do(
          "UPDATE transaction SET date = \"$date\" WHERE id = $id");
    }

    if (defined $opts{a} || defined $opts{p}) {

	my $r = $dbh->selectrow_arrayref(
          "SELECT acc_src, acc_dst FROM transaction WHERE id = $id");
	my ($src,$dst) = @{$r};

	my $account_id;
	if ($opts{a}) {
	    $account_id = lookup_account($account,0);
	    if (!defined $account_id) {
		print "ERROR: Unknown account '$account'\n";
		return 1;
	    }
	}
	my $payee_id;
	if ($opts{p}) {
	    $payee_id = lookup_account($payee,1);
	}
	if ($src < $MIN_PAYEE && defined $opts{a}) {
	    my $r = $dbh->do(
              "UPDATE transaction SET acc_src = $account_id WHERE id = $id");
	}
	if ($dst < $MIN_PAYEE && defined $opts{a}) {
	    my $r = $dbh->do(
              "UPDATE transaction SET acc_dst = $account_id WHERE id = $id");
	}
	if ($src > $MIN_PAYEE && defined $opts{p}) {
	    my $r = $dbh->do(
              "UPDATE transaction SET acc_src = $payee_id WHERE id = $id");
	}
	if ($dst > $MIN_PAYEE && defined $opts{p}) {
	    my $r = $dbh->do(
              "UPDATE transaction SET acc_dst = $payee_id WHERE id = $id");
	}
    }

    if (defined $opts{v}) {
	$value *= 100;
	my $r = $dbh->do(
          "UPDATE transaction SET value = $value WHERE id = $id");
    }

    if (defined $opts{c}) {
	my $r = $dbh->do(
          "UPDATE transaction SET comment = \"$comment\" WHERE id = $id");
    }

    if (defined $opts{t}) {
	print "ERROR: Cannot change transaction type\n";
	return 1;
    }
}

#----------------------------------------------------------
sub do_copy
{
    my $id = $opts{i};
    if (!defined $id) {
	print "ERROR: Must specify transaction ID to copy\n";
	return 1;
    }

    my $r = $dbh->selectrow_arrayref(
      "SELECT date, acc_src, acc_dst, value, comment ".
      "FROM transaction ".
      "WHERE id = '$id'");

    if (!$r) {
	print "ERROR: Transaction '$id' not found\n";
	return 1;
    }
    my ($c_date,$src,$dst,$c_value,$c_comment) = @{$r};
    $c_value /= 100;

    my $date = $opts{d} || $c_date;
    my $value = $opts{v} || $c_value;
    $value *= 100;
    my $comment = $opts{c} || $c_comment;

    $dbh->do(
      "INSERT INTO transaction (date, acc_src, acc_dst, value, comment) ".
      "VALUES (\"$date\", $src, $dst, $value, \"$comment\")");
}

#----------------------------------------------------------
sub do_list
{
    print "ACCOUNT: $account\n";
    print "\n";

    my $trans = get_transactions($account);

    my $final_bal = 0;
    my $credit_sum = 0;
    my $debit_sum = 0;

    if (!$compress) {
	print "    ID    DATE     PAYEE                             ".
	    " DEBIT    CREDIT   BALANCE\n";
	print $LINE;
    }

    foreach my $row (@{$trans}) {
	my ($id,$date,$payee,$comment,$debit,$credit,$bal) = @{$row};

	$credit_sum += $credit;
	$debit_sum += $debit;
	$credit_str = $credit ? sprintf($MFMT,$credit/100) : "";
	$debit_str = $debit ? sprintf($MFMT,$debit/100) : "";
	$bal_str = sprintf($MFMT,$bal/100);
	$final_bal = $bal;
	printf($TRANS_FMT,$id,$date,$payee,$debit_str,$credit_str,$bal_str);
	if ($comment && $opts{e}) {
	    print((" "x20)."($comment)\n");
	}
    }

    if (!$compress) {
	print $LINE;
	printf(" %-48s $MFMT $MFMT $MFMT\n",
	       "TOTALS FOR ".scalar @{$trans}. " TRANSACTIONS:", 
	       $debit_sum/100, $credit_sum/100, $final_bal/100);
	print $LINE;
    }
}

#----------------------------------------------------------
sub do_info
{
    my $id = $opts{i};
    if (!$id) {
	print "ERROR: Must specify transaction ID for info\n";
	return 1;
    }
    my $r = $dbh->selectrow_arrayref(
      "SELECT date, a1.name AS 'from', a2.name AS 'to', value, comment ".
      "FROM transaction AS t, account AS a1, account AS a2 ".
      "WHERE t.id = '$id' AND t.acc_src = a1.id AND t.acc_dst = a2.id");

    if (!$r) {
	print "ERROR: Transaction '$id' not found\n";
	return 1;
    }
    my ($date,$src,$dst,$value,$comment) = @{$r};
    $value /= 100;
    print "ID: $id\n".
	  "DATE: $date\n".
	  "SOURCE: $src\n".
	  "DEST: $dst\n".
	  "VALUE: $value\n".
	  "COMMENT: $comment\n";
}

#----------------------------------------------------------
sub do_accounts
{
    print "\n";
    my ($a) = @_;
    my $title = $a ? "ACCOUNT" : "PAYEE  ";
    my $r = $dbh->selectall_arrayref(
      "SELECT id, name ".
      "FROM account ".
      "WHERE id ". ($a ? '<' : '>') ." $MIN_PAYEE ".
      "ORDER BY name");
 
    if (!$compress) {
	print " $title                   TRANSACTIONS   BALANCE\n";
	print $LINE;
    }

    my $bal_sum = 0;
    my $trans_total = 0;
    foreach my $row (@{$r}) {
	my ($id,$name) = @{$row};

	my $trans = get_transactions($name);
	my $num = scalar @{$trans};
	my $lrow = $trans->[-1];
	$bal = $lrow ? $lrow->[-1] : 0;
	$bal_sum += $bal;
	$trans_total += $num;

	printf($ACC_FMT,$name,$num,$bal/100);
    }

    if (!$compress) {
	print $LINE;
	printf(" TOTALS                           %5d $MFMT\n",
	       $trans_total, $bal_sum/100);
	print $LINE;
    }

}

#----------------------------------------------------------
sub do_import
{
    my $file = $opts{f};
    open(FILE,$file) || die "Cannot open file $file";
    foreach (<FILE>) {
	chomp;
	if (/^\s*([0-9\-]+)\s+\"([^\"]+)\"\s+([0-9\-\.]+)\s*$/) {
	    ($date,$payee,$value) = ($1,$2,$3);
	    $type = 'c';
	    if ($value < 0) {
		$type = 'd';
		$value = -$value;
	    }
	    do_add();
	}
    }
    close(FILE);
}

#----------------------------------------------------------
sub get_transactions
{
    my ($account) = @_;
    my @trans = ();

    my $r = $dbh->selectall_arrayref(
      "SELECT t.id, date, a1.name AS 'from', a2.name AS 'to', value, comment ".
      "FROM transaction AS t, account AS a1, account AS a2 ".
      "WHERE t.acc_src = a1.id AND t.acc_dst = a2.id AND ".
      "(a1.name = \"$account\" OR a2.name = \"$account\") ".
      "ORDER BY date, t.id");

    my $bal = 0;
    foreach my $row (@{$r}) {
	my ($id,$date,$src,$dst,$value,$comment) = @{$row};
	my $payee = "";
	my $credit = 0;
	my $debit = 0;
	
	if ($src eq $account && $dst eq $account) {
	    $debit = $bal-$value if ($bal > $value);
	    $credit = $value-$bal if ($bal < $value);
	    $payee = '=BALANCE CHECKPOINT=';

	} elsif ($src eq $account) {
	    $debit = $value;
	    $payee = $dst;

	} elsif ($dst eq $account) {
	    $credit = $value;
	    $payee = $src;
	    
	} else {
	    die "Selected bad transaction";
	}
	
	$bal += $credit - $debit;
	my @row = ($id,$date,$payee,$comment,$debit,$credit,$bal);
	push(@trans,\@row);
    }
    
    my $tref = \@trans;
    return $tref;
}

#----------------------------------------------------------
sub lookup_account
{
    my ($name,$auto_create) = @_;

    my $r = $dbh->selectrow_arrayref(
      "SELECT id FROM account WHERE name = \"$name\"");
    my $id = $r->[0];

    if ($auto_create && !defined $id) {
	$r = $dbh->selectrow_arrayref(
          "SELECT MAX(id) FROM account");
	$id = 1 + $r->[0];
	print "Payee '$payee' not found...";
        $dbh->do(
           "INSERT INTO account (id, name) ".
           "VALUES ($id, \"$name\")");
	print " added (id=$id)\n";
    }

    return $id;
}

#----------------------------------------------------------
sub do_init_tables
{
    $dbh->do(
      "CREATE TABLE IF NOT EXISTS transaction ( ".
      "id       INT AUTO_INCREMENT PRIMARY KEY,".
      "date     DATE,".
      "acc_src  INT,".
      "acc_dst  INT,".
      "value    INT,".
      "comment  VARCHAR(255) )");

    $dbh->do(
      "CREATE TABLE IF NOT EXISTS account ( ".
      "id       INT UNIQUE PRIMARY KEY,".
      "name     VARCHAR(32) )");
    

    if (!lookup_account("MIN_PAYEE",0)) {
	$dbh->do("INSERT INTO account (id,name) VALUES (1000,'MIN_PAYEE')");

	# Create default accounts
	$dbh->do("INSERT INTO account (id,name) VALUES (0,'current')");
	$dbh->do("INSERT INTO account (id,name) VALUES (1,'credit card')");
	$dbh->do("INSERT INTO account (id,name) VALUES (2,'savings')");
    }
}

#----------------------------------------------------------
sub read_rcfile
{
    if (open(RC,$RCFILE)) {
	foreach (<RC>) {
	    chomp;
	    if (/^ *\#/) {
		# Comment
	    } elsif (/^ *$/) {
		# Blank line
	    } elsif (/^ *([^ ]+)\: *(.*) *$/) {
		$rcopts{$1} = $2;
	    } else {
		print "ERROR: Unknown line in $RCFILE: $_\n";
	    }
	}
	close(RC);
    }
}

#----------------------------------------------------------
sub do_help
{
print <<'END';
acow - Simple command-line accounting
usage: acow <command> {options}
  command is one of:
    add           Add a transaction
    delete        Delete a transaction
    edit          Edit a transaction
    copy          Copy a transaction
    list          List transactions on an account
    info          Display info about a transaction
    accounts      List my accounts with balances
    payees        List payee accounts with balances
    import        Import transactions from file
    help          Displays this useful help text
    version       Displays version and copyright
  options:
    -a <account>  Select account [default: current]
    -d <date>     Transaction date
    -t <type>     Transaction type:
      (d=debit [default], c=credit, b=balance checkpoint)
    -p <payee>    Payee account
    -v <value>    Transaction value
    -c <comment>  Comment on transaction
    -f <file>     File to import/export
    -i <id>       Transaction ID to edit/copy
    -e            Display extra information
END
}

#----------------------------------------------------------
sub do_ver
{
print "acow-$ACOW_VER (http://sconemad.com/acow)\n";
print "Copyright (c) 2006 Andrew Wedgbury <wedge\@sconemad.com>\n";
print "\n";
}
