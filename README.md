# National-Scale-Secure-Parallel-Computation
Secure parallel computation on national scale volumes of data


Installation

- Install required packages: 
	sudo apt-get install libboost-all-dev
	GMP
	EMP-Toolkit
	Crypto++

Run an example
	
	You can modify machine ip address configurations in machine_spec folder; for example use 127.0.0.1 if run on local host, or use machine real ip address in WAN setup.

	$ ./runAll.sh <Number of Parallel Machines/Cores> <Party Id> <Number of Ratings> <Number of Users> <Number of Movies> <Privacy Paremter Epsilon>

	To run a Matrix Factorization test example, following commands spine and run four parties with IDs [0..3], where each has 32 processors, with 1M real edges, 6k users and 4k movies, and privacy parameter \epsilon = 0.3 and \delta 2^-40

	./runAll.sh 32 0 1944000 6016 4000 0.3
	./runAll.sh 32 1 1944000 6016 4000 0.3
	./runAll.sh 32 2 1944000 6016 4000 0.3
	./runAll.sh 32 3 1944000 6016 4000 0.3
