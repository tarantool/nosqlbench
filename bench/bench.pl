#!/usr/bin/perl

use warnings;

##
## tarantool benchmark comparison suite.
##

# configuration
#

# git repository
local $REP="https://github.com/mailru/tarantool.git";
# local repository dir
local $REP_DIR="rep";
# local temporary dir
local $VAR="var";
# statistic files saving dir
local $STAT="stat";
# count of last commits to test
local $COUNT=2;

# tarantool binary
local $TNT_BIN="tarantool_box";
# tarantool config
local $TNT_CFG="tarantool.cfg";
# tarantool pid file
local $TNT_PID="tarantool.pid";
# benchmarking server host
local $TNT_HOST="localhost";
# benchmarking server port
local $TNT_PORT="33013";

# benchmark test
local $TEST="tnt-insert";
# benchmark test requests number
local $TEST_REQS=100000;
# benchmark test repeats
local $TEST_REPS=3;
# benchmark test zone
local $TEST_ZONE="5-100:10";
# benchmark threads
local $TEST_THREADS=10;

# command executing routine
#
sub notify { print("> @_\n"); }

sub runas {
	my $silent = $_[0];
	my $dump = $_[1];
	$cmd = "$_[2]";
	if ($silent) {
		$cmd .= " &> /dev/null";
	}
	if ($dump) {
		notify($_[2]);
	}
	system("$cmd") == 0 or die "error: failed to run $cmd";
}
sub runs { runas(1, 0, $_[0]); }
sub run { runas(1, 1, $_[0]); }
sub rundump { runas(0, 0, $_[0]); }

# toolchain checking routine
#
sub chtc {
	@path = split/:/, $ENV{"PATH"};
U:	foreach $u (@_) {
		foreach $p (@path) {
			if (-e $p."/".$u) {
				next U;
			}
		}
		notify("required utility \"$u\" not found");
		exit 1;
	}
}

# files existence checking routine
#
sub chfiles {
	foreach $u (@_) {
		if (-e $u) {
			next;
		}
		notify("required file \"$u\" not found");
		exit 1;
	}
}

# server control routines
#
sub server_start {
	runs("./$TNT_BIN --init-storage");
	runs("./$TNT_BIN -B");
	while (1) {
		if (-e $TNT_PID) {
			last
		}
		sleep(1);
	}
}
sub server_stop {
	$pid = `cat $TNT_PID`;
	chomp($pid);
	kill 15, $pid;
	while (1) {
		system("pgrep -f $TNT_BIN &> /dev/null") == 0 or next;
		last;
	}
}

notify("tarantool benchmark comparison suite.");

# checking required toolchain
chtc("mkdir", "cp", "rm", "cat", "make", "cmake",
     "grep", "pgrep", "git", "nb");

# checking required files
chfiles($TNT_CFG);

if (-d $REP_DIR) {
	notify("using local repository \"$REP_DIR\"");
} else {
	run("git clone $REP $REP_DIR");
}
chdir($REP_DIR);

# getting list of commits hashes
@commits = `git log | grep -o -E -e "[0-9a-f]{40}"`;
if ($#commits < $COUNT) {
	notify("commits are less than required");
}

@sums = ();
$current = 0;
foreach $commit (@commits) {
	if ($current == $COUNT) {
		last;
	}
	chop($commit);

	# checking out and building commit
	run("git checkout $commit");
	if (-e "Makefile") {
		run("make clean");
	}
	run("cmake .");
	run("make");

	# preparing testing environment
	notify("preparing var and running server");
	chdir("..");
	if (!-d $STAT) {
		runs("mkdir $STAT");
	}
	runs("rm -rf $VAR");
	runs("mkdir $VAR");
	runs("cp $REP_DIR/mod/box/$TNT_BIN $VAR/");
	runs("cp $TNT_CFG $VAR/");

	chdir($VAR);
	server_start();

	notify("running benchmark");
	$cmd = "nb -a $TNT_HOST -p $TNT_PORT -T $TEST -Z $TEST_ZONE ".
	       "-C $TEST_REQS -R $TEST_REPS -t $TEST_THREADS ".
	       "-S \"../$STAT/$commit.stat\"";
	rundump($cmd);

	# saving integral sum
	@result = split/\t/,`nb -G "../$STAT/$commit.stat"`;
	$sums[$current]=$result[1];
	notify("integral sum: $sums[$current]");

	notify("stopping server");
	server_stop();

	chdir("../$REP_DIR");
	$current++;
}

notify("comparing commits");
for ($i = 0 ; $i < $COUNT ;  $i++) {
	$ti = $sums[$i];
	for ($j = $i ; $j < $COUNT ;  $j++) {
		if ($i == $j) {
			next;
		}
		$tj = $sums[$j];
		$perc = 0;
		if ($ti >= $ti) {
			$diff = $ti - $tj;
			$perc = ($diff / $ti) * 100;
		} else {
			$diff = $tj - $ti;
			$perc = ($diff / $tj) * 100;
		}
		$percs = sprintf("%.2f%%", $perc);
		$tci = $commits[$i];
		$tcj = $commits[$j];
		notify("$tci ($ti) vs.");
		notify("$tcj ($tj) diff $percs");
	}
}
notify("done.");
