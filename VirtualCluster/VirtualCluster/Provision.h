//return the m value when the probability is no less than p
int provision(double lamda, double miu, double deadline, double p); 

//calculate the probability of response time less than deadline given m
double function(double lamda, double miu, double deadline, int m); 

//return the factorial value of n
double factorial(int n);

//return a random number from exponential distribution
double rnd_exponential(double lamda, double t);

//return the probability of waiting time less than time t given m
double wait_distribution(double lamda, double miu, double t, int m);

//for M/D/c queuing system
double stationary_p(double lamda, double miu, int m, int i);