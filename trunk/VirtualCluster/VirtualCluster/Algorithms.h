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

class GanttConsolidation {
public:
	void Initialization(Job* job, int in); //in=1, best-fit; in=2, worst-fit; in=3, most-efficient
	void Simulate(Job job);
	//planner is invoked every 15mins, jobs are currently in the queue
	void Planner(std::vector<Job*> jobs, int threshold);
};

//update the start and end time of tasks, according to their choice of VM type
void BFS_update(Job* testJob);
//update the start and end time of tasks after task_no
void time_flood(Job* testJob, int task_no);
//cost estimation of the consolidation operators, return the gain of the operation
double move(int type, double x, double y); //VM type, execution time of two tasks before move on type 
double ppromote(int type1, double t1, int type2, double t2);
double demote(int type1, double t1, int type2, double t2);//VM type and execution time before and after demote operation
double split(int type, double t, double t1, double t2);//VM type, execution time of the task, the time point to split
double coschedule(int type1, int type2, int type, double t1, double t2, double t3, double t4, double degradation);
double merge(int type, double* times, int size);//VM type, execution time and the number of the series of tasks to be merged

void move_operation1(std::vector<Job*> jobs, VM* v1, VM* v2, double moveafter); //v1.start < v2.end, move 1 to 2
void move_operation2(std::vector<Job*> jobs, VM* v1, VM* v2); //v1.start > v2.end, and v1.start < v2.start+v2.life
bool vmfunction(VM* a, VM* b);
bool comp_by_first(std::pair<double, std::pair<int,int> > a, std::pair<double, std::pair<int,int> > b);