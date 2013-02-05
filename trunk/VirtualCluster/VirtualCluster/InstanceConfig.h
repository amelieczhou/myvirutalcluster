#include <boost/graph/adjacency_list.hpp>
#include <boost/graph/graph_traits.hpp>
#include <vector>
#include <queue>
using namespace boost;

const int types = 4; //VM types
const double priceOnDemand[] = {0.095, 0.19, 0.38, 0.76};
const double OnDemandLag = 5;//0;

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

	void instance_config(); //configuration for the prefer_type
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

bool mycompare(std::pair<double, double> a, std::pair<double, double> b);

class Job{
public:
	Graph g;
	double deadline;
	Integer_job type;
	int* serverGroup;
	double lamda; //arrival rate
	double arrival_time;
	std::vector<std::pair<double, double> > templates[types];
	
	Job (Integer_job t, double d, double l) {deadline = d; type = t; lamda = l;}
	Job (Graph dag) {g = dag; }
	void reset(); // reset the parameters of tasks in the Job
	std::vector<int> find_CP(); //find the critical path of the Job, for deadline assignment
	void deadline_assign(); // deadline assignment
	int* find_ServerGroup(); //find the number of VMs in each type for one job
	int* find_ServerGroup_SA(); //for simulate_sa
};

class VM {
public:
	double price; //price for one hour
	int type; //instance type
	taskVertex* curr_task; //the task currently executing on the VM
	double life_time; //how long the vm has been on
	double in_use; //how long the vm is been used
	double idle_time; //how long the vm has been idle

	VM() { life_time = idle_time = in_use = 0; curr_task = NULL;}
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
void AssignPath(std::deque<Vertex> PCP,Job* job);
bool has_unassigned_parent(taskVertex* tk, Job* job);
bool has_unassigned_child(taskVertex* tk, Job* job);
void update_EST(taskVertex* tk, Job* job);
void update_LFT(taskVertex* tk, Job* job);

//the following functions are for finding the server groups
std::vector<Point> merge(std::vector<Point> build1, std::vector<Point> build2);
std::vector<Point> merge_sort(std::vector<Building> b);

//use the following functions to calculate the sharing rate
std::vector<std::pair<double, double> >  merge_sort_sharing(std::vector<std::pair<double, double> > buildlist);
std::vector<std::pair<double, double> >  merge_sharing(std::vector<std::pair<double, double> > build1, std::vector<std::pair<double, double> > build2);
//double Cal_Cost(std::vector<std::pair<double, double> > points);

bool myfunction(taskVertex* a, taskVertex* b);