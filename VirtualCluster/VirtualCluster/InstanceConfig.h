#include <boost/graph/adjacency_list.hpp>
#include <boost/graph/graph_traits.hpp>
#include <vector>
#include <queue>
using namespace boost;
using namespace std;

const int types = 4; //VM types
const double priceOnDemand[] = {0.095, 0.19, 0.38, 0.76};//{0.022, 0.06, 0.12, 0.24};//
const double OnDemandLag = 0;//0;
const double inter_time = 5; //time interval between 2 vms less than inter_time can be merged
const double idle_interval = 30; //a vm has been idle for idle_interval has to be turned off
const double degradation = 0.2;
//const int threshold = 10; //terminate planning if no gains can be obtained by consolidation for more than threshold times

enum Integer_vm{
	not_ready = 0,
	ready = 1,
	scheduled = 2,
	finished = 3
};

enum Integer_job{
	pipeline = 0,
	hybrid = 1,
	Montage = 2,
	Ligo = 3,
	Cybershake = 4,
	single = 5
};

class taskVertex{
public:
	int name; //mark the tasks with name
	double sub_deadline; //after deadline assignment
	double* estTime; //estimated execution time of the task on all instance types
	int prefer_type; //if multi-cloud or with spot, int -> int*
	int assigned_type; //actual VM type assigned at run time
	double readyCountDown; //for status change
	Integer_vm status; //not_ready, ready, scheduled, finished
	double cost; //monetary cost spent on this task
	double start_time; // earliest start time of the task
	double end_time; // actual end time of the task
	double EST; //earliest start time
	double LFT; //latest finish time
	double restTime; // for runtime execution
	bool mark; //already assigned sub-deadline => 1
	int type; //type of task

	//for GanttChart
	//double scheduled_time; //the time when the task is scheduled to run
	bool operator< (const taskVertex& x) const {return start_time < x.start_time; } //for priority queue
	//Job* job; //the task belong to job
	int job_id;
	//vector<taskVertex*> children;
	//vector<double> break_time; //for split operation
	double breaktime1, breaktime2;
	taskVertex* virtualtask; //the next half after split

	void instance_config(); //configuration for the prefer_type

	taskVertex() {
		assigned_type = prefer_type = mark = breaktime1 = breaktime2 = 0;
		cost = start_time = end_time = sub_deadline = restTime = 0;
		EST = LFT = 0;
		readyCountDown = -1; //
		status = not_ready;
		virtualtask = NULL;
	}
};

typedef adjacency_list<vecS, vecS, bidirectionalS, taskVertex, property<edge_weight_t, double> > Graph;
typedef graph_traits<Graph>::vertex_descriptor Vertex;
typedef graph_traits<Graph>::vertex_iterator vertex_iter;
typedef graph_traits<Graph>::out_edge_iterator out_edge_iterator;
typedef graph_traits<Graph>::in_edge_iterator in_edge_iterator;
typedef graph_traits<Graph>::edge_descriptor edge_descriptor;
typedef graph_traits<Graph>::adjacency_iterator adja_iterator;
typedef graph_traits<Graph>::edge_descriptor edge_descriptor;
//typedef typename boost::graph_traits<Graph>::vertices_size_type vertices_size;

bool mycompare(pair<double, double> a, pair<double, double> b);

class Job{
public:
	int name; //mark the job with a name
	Graph g;
	double deadline;
	double budget;
	Integer_job type;
	int* serverGroup;
	double lamda; //arrival rate
	double arrival_time;
	vector<pair<double, double> > templates[types];
	
	Job (Integer_job t, double d, double l) {deadline = d; type = t; lamda = l;}
	Job (Graph dag) {g = dag; }
	void reset(); // reset the parameters of tasks in the Job
	vector<int> find_CP(); //find the critical path of the Job, for deadline assignment
	void deadline_assign(); // deadline assignment
	int* find_ServerGroup(); //find the number of VMs in each type for one job
	int* find_ServerGroup_SA(); //for simulate_sa
};

class VM {
public:
	//double price; //price for one hour
	//int name;
	int type; //instance type
	taskVertex* curr_task; //the task currently executing on the VM
	double life_time; //how long the vm has been on
	double in_use; //how long the vm is been used
	double idle_time; //how long the vm has been idle

	//for GanttChart
	double start_time; //the time the VM is turned on
	double end_time; //the end time of the last task executed on this vm
	double resi_time; //left over one hour time
	vector<vector<taskVertex*> > assigned_tasks; //the tasks assigned to share the one hour time
	//std::list<int> dependentVMs; //the VMs depending on the current VM
	bool operator< (const VM& x) const {return start_time < x.start_time; } //for priority queue
	int capacity; //for coscheduling, stands for the number of tasks can be executed at the same time

	VM() { life_time = idle_time = in_use = start_time = resi_time = 0; curr_task = NULL; }
	//VM(const VM& vm) {//deep copy
	//	if(this!=&vm){
	//		type = vm.type;	capacity = vm.capacity;	end_time = vm.end_time; life_time = vm.life_time; in_use = vm.in_use;
	//		idle_time = vm.idle_time; start_time = vm.start_time; resi_time = vm.resi_time;
	//		//curr_task = new taskVertex();
	//		//memcpy(curr_task,vm.curr_task,sizeof(taskVertex));
	//		for(int i=0; i<vm.assigned_tasks.size(); i++){
	//			vector<taskVertex*> tasks;
	//			for(int j=0; j<vm.assigned_tasks[i].size(); j++){
	//				taskVertex* task = new taskVertex();
	//				memcpy(task,vm.assigned_tasks[i][j],sizeof(taskVertex));
	//				//*task = *vm.assigned_tasks[i][j];
	//				tasks.push_back(task);
	//			}
	//			assigned_tasks.push_back(tasks);
	//		}
	//	}
	//}
	//~VM() {
	//	//delete curr_task; 
	//	for(int i=0; i<assigned_tasks.size(); i++)
	//		for(int j=0; j<assigned_tasks[i].size(); j++)
	//			delete assigned_tasks[i][j];
	//}
};

class Point {
public:
	double height;
	double location;

	Point(){height = location = 0;}
	Point(double x, double y) {location = x; height = y;}
};
class Building {
public:
	double height;
	double left;
	double right;

	Building() {height = left = right =0;}
	Building(double x, double y, double z) {left = x; right = y; height = z;}
};

double MET(taskVertex tk);
double MTT(taskVertex* tk1, taskVertex* tk2);
double EST(taskVertex tk, Job job);
double LFT(taskVertex tk, Job job);
void AssignParents(taskVertex* tk, Job* job);
Vertex CriticalParent(taskVertex* tk, Job* job);
void AssignPath(deque<Vertex> PCP,Job* job);
bool has_unassigned_parent(taskVertex* tk, Job* job);
bool has_unassigned_child(taskVertex* tk, Job* job);
void update_EST(taskVertex* tk, Job* job);
void update_LFT(taskVertex* tk, Job* job);

//the following functions are for finding the server groups
vector<Point> merge(vector<Point> build1, vector<Point> build2);
vector<Point> merge_sort(vector<Building> b);

//use the following functions to calculate the sharing rate
vector<pair<double, double> >  merge_sort_sharing(vector<pair<double, double> > buildlist);
vector<pair<double, double> >  merge_sharing(vector<pair<double, double> > build1, vector<pair<double, double> > build2);
//double Cal_Cost(std::vector<std::pair<double, double> > points);

bool myfunction(taskVertex* a, taskVertex* b);