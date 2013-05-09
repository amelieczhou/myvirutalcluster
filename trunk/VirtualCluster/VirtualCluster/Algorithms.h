#include <vector>
using namespace std;

class VirtualCluster {
public:
	void Simulate(Job job);	
	void Simulate_SA(Job job);
	pair<double, double> Cal_Cost(Job* job);//return cost and finish time of job
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

class GanttConsolidation {
public:
	void Initialization(Job* job, int in); //in=1, best-fit; in=2, worst-fit; in=3, most-efficient
	void Simulate(Job job, int in);//in=1, best-fit; in=2, worst-fit; in=3, most-efficient
	//planner is invoked every 15mins, jobs are currently in the queue
	//returns the cost, average time and time violation of such planning
	//planning the jobs at timer
	//pair<double, pair<double,double> > Planner(vector<Job*> jobs, int timer); 
	double Planner(vector<Job*> jobs, int timer); //return cost
};