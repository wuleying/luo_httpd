#!/usr/bin/perl -Tw

use strict;
use CGI;

my $cgi = new CGI;

print $cgi->header;
print $cgi->start_html(-BGCOLOR => "green");
print $cgi->h1("CGI demo");
print $cgi->end_html();