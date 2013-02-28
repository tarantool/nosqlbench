nessDB Readme
=============

**nessDB** is a fast key-value database (embedded), which is written in ANSI C.

https://github.com/shuttler/nessDB

* nessDB v1 uses log-structured-merge (LSM) trees, it is mostly like LevelDB

* nessDB v2 uses Small-Splittable Tree (SST), it's one of Bε-tree and
Cache-oblivious write-optimized structure.

Installation
------------

nessDB does not have packages for popular Linux distros, so
you have to compile it yourself.

Create `third_party` directory:

	nosqlbench $ mkdir third_party
	nosqlbench $ cd third_party

Clone nessDB repository:

	third_party $ git clone git://github.com/shuttler/nessDB.git
	third_party $ cd nessDB
	
Checkout proper branch:

	# For v1.8.x (with LSM)
	nessDB $ git checkout v1.8.2
	
	# For v2.0.x (with SST)
	nessDB $ git checkout v2.0

Compile nessDB:

	nessDB $ make

Re-run cmake for NoSQL Benchmark:

	nessDB $ cd ../..
	nosqlbench $ cmake .
	...
	-- Found NESSDB: ./third_party/nessDB/libnessdb.so 
	-- Using NessDB V2 (with SST)
	...

Benchmarking
------------

Change your nosqlbench.conf:

	# nessDB is single-threaded
	client_start 1
	client_max 1
	db_driver 'nessdb'
	
	# update is meaningless for nessdb
	test_update 0
