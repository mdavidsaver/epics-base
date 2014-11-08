#!/usr/bin/perl
#*************************************************************************
# Copyright (c) 2014 Michael Davidsaver
# EPICS BASE is distributed subject to a Software License Agreement found
# in the file LICENSE that is included with this distribution. 
#*************************************************************************
#  $Revision-Id$
#
# Transform a text file into an array of C strings
#
# Usage:
#  txt2c.pl <varname> <input file> <output file>
#
# Use forward definition
#
# extern "C" const char* varname[];

use strict;
use warnings;

my $varname = shift @ARGV;
my $inpf = shift @ARGV;
my $outf = shift @ARGV;

open(my $INP, '<', $inpf) or die "Failed to open $inpf";
open(my $OUT, '>', $outf) or die "Failed to open $outf";

print $OUT <<"DONE";
const char* $varname\[\] = {
DONE

for(<$INP>) {
  chomp;
  s/\\/\\\\/g;
  s/"/\\"/g;
  print $OUT " \"$_\",\n";
}

print $OUT <<'DONE';
 0
};
DONE

close($INP);
close($OUT);
