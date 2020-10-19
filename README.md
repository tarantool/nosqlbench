NoSQL Benchmark
===============

NoSQL Benchmark (nb) is a multi-threaded benchmark tool for evaluating main
database performance metrics using OLTP-like benchmarking against
a specified workload.

Features include:

* benchmarking types: unlimited, time limited or maximum thread limited
* different threads creation policies: at once or interleaved
* key distribution supported: uniform, gaussian
* key types supported: string, u32, u64
* CSV report file generation supported (for future plot generation)
* single configuration file
* workload tests are specified in percents against a total request count
* supported database drivers: tarantool, leveldb (redis, memcached planned).
* plotter tool (CSV to GNU Plot generation)

Downloading and installing
--------------------------

The instructions here are for Linux.
The package should also work on FreeBSD.

We assume that it is okay to use the $HOME directory.
If this is not the case, then change all occurrences
of the word $HOME in this document to whatever seems
more appropriate.

We assume tht it is okay to use sudo for installation
of some common packages from the distro or from
Tarantool's github repository.

Before installing tarantool-nosqlbench, it is a good idea to
install Tarantool itself from source following the instructions
in the Tarantool manual
https://www.tarantool.io/en/doc/2.5/dev_guide/building_from_source/.
Many of the packages that you install for Tarantool
are also necessary for tarantool-nosqlbench, and it
will be easier to run initial tests if you can start
a server on localhost.

```

# Install packages which should be available from your distro.
# The following is a common way to install them on Ubuntu
sudo apt-get install git cmake make build-essential libev-dev
# The following is a common way to install them on Fedora
sudo dnf install git cmake make coreutils gcc gcc-c++ libev-devel

# Install msgpuck from a Tarantool github repository.
# This is necessary for tarantool-c.
cd $HOME
git clone https://github.com/tarantool/msgpuck
cd msgpuck
cmake .
make
sudo make install

# Install tarantool-c from a Tarantool github repository.
cd $HOME
git clone https://github.com/tarantool/tarantool-c
cd tarantool-c
cmake .
make
sudo make install

# Set an environment variable to the tarantool-nosqlbench directory.
# The suggestion $HOME/tarantool-nosqlbench is a common location.
# This assumes you use bash.
export TARANTOOL_NOSQLBENCH="$HOME/tarantool-nosqlbench"

# Download the source of tarantool-nosqlbench
git clone https://github.com/tarantool/nosqlbench $TARANTOOL_NOSQLBENCH

# Edit one of the files using your favorite text editor.
# The file name is $TARANTOOL_NOSQLBENCH/src/CMakeLists.txt.
# The change is in the line
# set(nb_libs tnt mc ev ${M_LIB})
# it should be
# set(nb_libs tarantool mc ev ${M_LIB})
# Here is one way to do it where we assume the sed editor exists.
cd $TARANTOOL_NOSQLBENCH
sed -i 's/set(nb_libs tnt mc ev /set(nb_libs tarantool mc ev /' src/CMakeLists.txt

# Copy a file from the directory that you created earlier when
# you said "git clone https://github.com/tarantool/msgpuck".
mkdir $TARANTOOL_NOSQLBENCH/src/msgpuck
cp $HOME/msgpuck/msgpuck.h $TARANTOOL_NOSQLBENCH/src/msgpuck/msgpuck.h

# Build!
cd $TARANTOOL_NOSQLBENCH
cmake .
make

# When running CMake you should see these warnings:
# -- Could NOT find LEVELDB (missing: LEVELDB_LIBRARIES LEVELDB_INCLUDE_DIRS)
# -- Could NOT find NESSDB (missing: NESSDB_LIBRARIES NESSDB_INCLUDE_DIRS)
# ... This is okay. LevelDB and NessDB should not be installed.

# There are other options, for example using a different C connector.
# They can be seen by looking at the source of CMakeLists.txt.
```

Running, The First Time
-----------------------

```
# Start a Tarantool server.
# The host will be localhost and the port will be 3303,
# this can be changed by editing src/nosqlbench.conf as we'll see later.
# The nosqlbench user name will be 'guest', so make sure 'guest' can do things.
# (This is just for a test, make sure it is not a production system.)
# The 'guest' user does not require a password.
# To avoid error ER_MEMORY_ISSUE "Failed to allocate %u bytes in %s for %s",
# set the amount of memory needed for the memtx storage engine to 1GB.
# To avoid warnings on the server saying "readahead limit is reached",
# we set readahead to a value that is larger than the default (16384).
# There are lots more ways to "tune" the DBMS, but we leave them up to our users.
# Start with an empty database, that is, the default directory should have no database files.
# The space name does not have to be "512" -- it can be anything -- but nosqlbench
# will look for the first space with a non-system id, which happens to be id=512.
# We are calling this empty directory tarantool_sandbox, it can be anything.
# We are assuming that the Tarantool server can be started by saying tarantool,
# but it can be wherever you downloaded to when preparing.

mkdir $HOME/tarantool_sandbox
cd $HOME/tarantool_sandbox
tarantool
box.cfg{listen = 3303, memtx_memory = 2^30, readahead = 250000}
box.schema.user.grant('guest','read,write,execute,create,drop','universe')
box.schema.space.create("512")
box.space["512"]:create_index("I")

# Now, on a different shell, run nb, the main tarantool-nosqlbench program.
# There is one mandatory argument, the name of a configuration file.
# Luckily for test purposes, there is a default configuration file
# named nosqlbench.conf which has many settings with default values.
# We assume that the file libtarantool.so, which you installed earlier
# when you said "sudo make install" for tarantool-c, is now on a directory
# such as /usr/local/lib. You might need to specify for the loader, e.g.
# export LD_LIBRARY_PATH=/lib:/usr/lib:/usr/local/lib

export TARANTOOL_NOSQLBENCH="$HOME/tarantool-nosqlbench"
cd $TARANTOOL_NOSQLBENCH
src/nb src/nosqlbench.conf

# Let it run while it is saying "Warmup".
# Then let it run for a while, while it is displaying results at intervals.
# You will see displays for one-second intervals, and for totals, with histograms.
```

Changing the options
--------------------

If you got as far as seeing some displays when running the instructions
in the previous section, then you are now ready to change the settings
and tune the database. To do this, look at the nosqlbench.conf file
and make some changes.
For example you can change the host from "localhost" and
you can change the port from "3303".
But most of the changes involve how to stress the DBMS differently,
as you will see just by looking at the names of the settings.
You will also find that some of the DBMS's default settings will
be inappropriate when you run stressful tests, the solution then
is to check the Tarantool user manual "configuration" reference
https://www.tarantool.io/en/doc/2.5/reference/configuration/
and see what tuning can do.
