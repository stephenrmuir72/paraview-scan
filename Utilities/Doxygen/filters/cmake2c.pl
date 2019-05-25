#!/usr/bin/env perl
use warnings;
use strict;

# print $ARGV[0];

open fp, "<$ARGV[0]";

my $result = q{};
my $commentBlockStarted = 0;
my $functionStarted = 0;

for my $line (<fp>) {
    chomp $line;

    if ($line =~ m{^\s*$}) {
        $result .= "\n";
        next;
    }
    if ($commentBlockStarted == 1) {
        if ($line =~ m{#\]==\]}) {
            $line =~ s{#\]==\]}{*/};
            $commentBlockStarted = 0;
        }
        $line =~ s{^\s*\*\s+}{ - };
        $result .= "$line\n";
        next;
    }
    if ($functionStarted == 1) {
        if ($line =~ m{(^endfunction\s*\(.*\)|^endmacro\s*\(.*\))}) {
            $result .= "}\n";
            $functionStarted = 0;
            next;
        }
        $result .= "\n";
        next;
    }

    if ($line =~ m{#\[==\[}) {
        $line =~ s{#\[==\[}{/**};
        $result .= "$line\n";
        $commentBlockStarted = 1;
        next;
    }

    if ($line =~ m{^\s*#}) {
        $result .= "\n";
        next;
    }

    $line =~ s{#.*}{};

    if ($line =~ m{(^function\s*\()|(^macro\s*\()}) {
        $functionStarted = 1;
        if ($line =~ m{^function\s*\(}) {
            $result .= "function ";
            $line =~ s{^function\s*\(}{};
        }
        elsif ($line =~ m{^macro\s*\(}) {
            $result .= "macro ";
            $line =~ s{^macro\s*\(}{};
        }

        $line =~ s{\)\s*$}{};
        my @args = split(' ', $line);
        $result .= $args[0];
        shift(@args);
        $result .= " (";
        for my $arg (@args) {
            $result .= $arg;
            if ($arg ne $args[-1]) {
                $result .= " ,";
            }
        }
        $result .= ") {\n";
        next;
    }
    $result .= "\n";
    next;
}

print $result;
