//const double Times[4][types] = {{120,65,38,24},{90,50,30,20},{60,35,23,17},{30,20,15,13}};
//const double Times[4][types] = {{12,7,4,2},{9,5,3,2},{6,4,2,1},{3,2,2,1}};
const double Times[4][types] = {{210,110,58,32},{150,65,38,24},{90,50,30,20},{30,20,15,13}};
const double inter_time = 5; //time interval between 2 vms less than inter_time can be merged
const double idle_interval = 30; //a vm has been idle for idle_interval has to be turned off

class VirtualCluster {
public:
	void Simulate(Job job);	
	void Simulate_SA(Job job);
	std::pair<double, double> Cal_Cost(Job* job);//return cost and finish time of job
	double* sharing_rate(Job job); //return the sharing rate of a job template
};

class AutoScaling {
public:
	void Simulate(Job job);
};

enum Integer_Alg {
	virtualcluster = 0,
	autoscaling = 1
};