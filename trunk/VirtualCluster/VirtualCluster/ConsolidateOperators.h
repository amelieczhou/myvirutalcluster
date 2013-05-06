#include "InstanceConfig.h"


//update the start and end time of tasks, according to their choice of VM type
void BFS_update(Job* testJob);
//update the start and end time of tasks after task_no
bool time_flood(taskVertex* task);//(Job* testJob, int task_no);
//cost estimation of the consolidation operators, return the gain of the operation
double move(int type, double x, double y, double total); //VM type, execution time of two tasks before move on type 
double ppromote(int type1, double t1, int type2, double t2);
double demote(int type1, double t1, int type2, double t2);//VM type and execution time before and after demote operation
double split(int type, double t, double t1, double t2);//VM type, execution time of the task, the time point to split
double coschedule(int type1, int type2, int type, double t1, double t2, double t3, double t4, double degradation);
double merge(int type, double* times, int size);//VM type, execution time and the number of the series of tasks to be merged

//actions
void move_operation1(vector<Job*> jobs, VM* v1, VM* v2, int out,double moveafter); //v1.start < v2.end, move 1 to 2
void move_operation2(vector<Job*> jobs, VM* v1, VM* v2, int out); //v1.start > v2.end, and v1.end < v2.start+v2.life
void move_operation3(vector<Job*> jobs, VM* v1, int out, VM* v2, double moveafter); //v1.start > v2.start, split v2
void demote_operation(vector<Job*> jobs, VM* vm, int type); //demote vm to type
void promote_operation(vector<Job*> jobs, VM* vm, int type); //promote vm to type
void coschedule_operation(vector<Job*> jobs, VM* vm1, VM* vm2,double degradation); //coschedule vm2 to vm1
void merge_operation(vector<Job*> jobs, VM* v1, VM* v2);
//rules
double opMove(vector<VM*>*  VMs, vector<Job*> jobs);//rules to do move operation, returns the cost saved by it
double opSplit(vector<VM*>*  VMs, vector<Job*> jobs);
double opMerge(vector<VM*>*  VMs, vector<Job*> jobs);
double opPromote(vector<VM*>*  VMs, vector<Job*> jobs);
double opDemote(vector<VM*>*  VMs, vector<Job*> jobs);
double opCoschedule(vector<VM*>*  VMs, vector<Job*> jobs);

bool updateVMqueue(vector<VM*>* VMs);
//void siteRecovery(vector<Job*> jobs, VM* vm);

bool vmfunction(VM* a, VM* b);
bool taskfunction(taskVertex* a, taskVertex* b);
bool comp_by_first(pair<double, pair<int,int> > a, pair<double, pair<int,int> > b);