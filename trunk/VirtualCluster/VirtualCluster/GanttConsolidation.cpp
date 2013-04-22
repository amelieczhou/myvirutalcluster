#include "stdafx.h"
#include "Provision.h"
#include "InstanceConfig.h"
#include "Algorithms.h"
#include <fstream>
#include <string>
#include <sstream>
#include <boost/graph/topological_sort.hpp>

extern double max_t;
extern bool DynamicSchedule;
extern double deadline;
extern double chk_period;
extern bool sharing;
extern bool on_off;
extern double lamda;
extern double QoS;

//Breath-first traverse to update the start and end time of jobs
void BFS_update(Job* testJob)
{
	//std::pair<vertex_iter, vertex_iter> vp; 
	//vp = vertices(testJob->g);
	//std::queue<taskVertex> OPEN;
	//OPEN.push(testJob->g[*vp.first]);
	//std::queue<taskVertex> CLOSE;	
	//while(!OPEN.empty()) {
	//	taskVertex visited_task = OPEN.front();
	//	OPEN.pop();
	//	//get child vertices
	//	out_edge_iterator out_i, out_end;
	//	edge_descriptor e;
	//	for (boost::tie(out_i, out_end) = out_edges(visited_task.name, testJob->g); out_i != out_end; ++out_i) 
	//	{
	//		e = *out_i;
	//		Vertex tgt = target(e,testJob->g);
	//		if(testJob->g[tgt].mark == 0) OPEN.push(testJob->g[tgt]);
	//	}
	//	visited_task.mark = 1;
	//	CLOSE.push(visited_task);
	//}

	std::deque<int> topo_order;
	topological_sort(testJob->g,std::front_inserter(topo_order));
	//tasks ordered in front are parents to tasks ordered behind them
	for(int i=0; i < topo_order.size(); i++)
	{
		testJob->g[i].start_time = testJob->g[i].end_time = 0;
	}
	for(std::deque<int>::const_iterator i = topo_order.begin(); i != topo_order.end(); ++i)
	{
		taskVertex* visited_task = &testJob->g[*i];
		visited_task->end_time = visited_task->start_time + visited_task->estTime[visited_task->assigned_type];
		//get child vertices
		out_edge_iterator out_i, out_end;
		edge_descriptor e;
		for (boost::tie(out_i, out_end) = out_edges(visited_task->name, testJob->g); out_i != out_end; ++out_i) 
		{
			e = *out_i;
			Vertex tgt = target(e,testJob->g);
			if(testJob->g[tgt].start_time < visited_task->end_time) 
				testJob->g[tgt].start_time = visited_task->end_time;
		}
	}
	//std::pair<vertex_iter, vertex_iter> vp=vertices(testJob->g);
	//for(; vp.first!=vp.second; ++vp.first) {
	//		Vertex v1 = *vp.first;
	//		std::cout<<testJob->g[v1].start_time<<", "<<testJob->g[v1].end_time<<std::endl;
	//}
}

void time_flood(Job* testJob, int task_no)
{
	taskVertex* start_task = &testJob->g[task_no];
	start_task->end_time = start_task->start_time + start_task->estTime[start_task->assigned_type];
	std::queue<taskVertex*> children;
	children.push(start_task);	//the tasks pushed are already updated
	while(!children.empty()) {
		taskVertex* curr_task = children.front();
		//get children vertices
		out_edge_iterator out_i, out_end;
		edge_descriptor e;
		for (boost::tie(out_i, out_end) = out_edges(curr_task->name, testJob->g); out_i != out_end; ++out_i) 
		{
			e = *out_i;
			Vertex tgt = target(e,testJob->g);
			taskVertex* child = &testJob->g[tgt];
			child->start_time = (curr_task->end_time > child->start_time)?curr_task->end_time : child->start_time;
			child->end_time = child->start_time + child->estTime[child->assigned_type];
			children.push(child);
		}
		children.pop();
	}	
}
//cost estimation of move operator, return the gain of the operation
double move(int type, double x, double y) {
	double p = priceOnDemand[type];
	double save = (std::ceil(x/60.0) + std::ceil(y/60.0) - std::ceil((x+y)/60.0))*p;
	return save;
}
double split(int type, double t, double t1, double t2) {
	double p = priceOnDemand[type];
	double save = (std::ceil(t/60.0) - std::ceil(t1/60.0) - std::ceil(t2/60.0))*p;
	return save;
}
double demote(int type1, double t1, int type2, double t2) {	
	double p1 = priceOnDemand[type1], p2 = priceOnDemand[type2];
	double save = p1*std::ceil(t1/60.0) - p2*std::ceil(t2/60.0);
	return save;
}
double ppromote(int type1, double t1, int type2, double t2){
	double p1 = priceOnDemand[type1], p2 = priceOnDemand[type2];
	double save = p1*std::ceil(t1/60.0) - p2*std::ceil(t2/60.0);
	//find the critical path save

	return save;
}

void move_operation1(std::vector<Job*> jobs, VM* vm1, VM* vm2, double moveafter) {//move 1 to 2, 1 starts before the end of 2
	double lastend = 0;
	for(int taskiter=0; taskiter < vm1->assigned_tasks.size(); taskiter++) {
		vm1->assigned_tasks[taskiter]->start_time += moveafter;
		time_flood(jobs[vm1->assigned_tasks[taskiter]->job_id],vm1->assigned_tasks[taskiter]->name);
		vm2->assigned_tasks.push_back(vm1->assigned_tasks[taskiter]);	
		if(vm1->assigned_tasks[taskiter]->end_time > lastend)
			lastend = vm1->assigned_tasks[taskiter]->end_time;
	}
	vm2->end_time = lastend;
	vm2->life_time = std::ceil((vm2->end_time - vm2->start_time)/60.0 )*60.0;
	vm2->resi_time = vm2->life_time - vm2->end_time + vm2->start_time;
	vm1->start_time = vm1->end_time = vm1->resi_time = vm1->life_time = 0;
	vm1->assigned_tasks.clear(); 
}
void move_operation2(std::vector<Job*> jobs, VM* v1, VM* v2){ //do not need to update v1
	for(int taskiter=0; taskiter < v1->assigned_tasks.size(); taskiter++)
		v2->assigned_tasks.push_back(v1->assigned_tasks[taskiter]);
	v2->end_time = VM_queue[i][k]->end_time;
	v2->life_time = std::ceil((VM_queue[i][j]->end_time - VM_queue[i][j]->start_time)/60.0)*60.0;
	v2->resi_time = VM_queue[i][j]->life_time + VM_queue[i][j]->start_time - VM_queue[i][j]->end_time; 
	v1->start_time = v1->end_time = v1->resi_time = v1->life_time = 0;
	v1->assigned_tasks.clear();
}
double coschedule(int type1, int type2, int type, double t1, double t2, double t3, double t4, double degradation){
	double p1 = priceOnDemand[type1], p2 = priceOnDemand[type2], p = priceOnDemand[type];
	double cost_old = std::ceil(t1/60.0)*p1 + std::ceil(t2/60.0)*p2;
	double cost3 = std::ceil(t3*(1+degradation)/60.0)*p; 
	double cost4 = std::ceil(t4*(1+degradation)/60.0)*p;
	double cost_new = (cost3 > cost4) ? cost3 : cost4;
	return cost_old - cost_new;
}

double merge(int type, double* times, int size) {
	double p = priceOnDemand[type];
	double save=0, sum=0;
	for(int i=0; i<size; i++) {
		sum += times[i];
		save += std::ceil(times[i]/60.0);
	}
	save = (save - std::ceil(sum/60.0))*p;
	return save;
}

bool vmfunction(VM* a, VM* b) {
	return (a->start_time < b->start_time);
}
bool comp_by_first(std::pair<double, std::pair<int,int> > a, std::pair<double, std::pair<int,int> > b) {
	return (a.first < b.first);
}
void GanttConsolidation::Initialization(Job* testJob, int policy)
{
	std::pair<vertex_iter, vertex_iter> vp;
	vp=vertices(testJob->g);
	if(policy == 1) {//best fit
		for(; vp.first!=vp.second; ++vp.first) {
			Vertex v1 = *vp.first;
			testJob->g[v1].assigned_type = types-1;
		}
	}
	else if(policy == 2) {//worst fit
		for(; vp.first!=vp.second; ++vp.first) {
			Vertex v1 = *vp.first;
			testJob->g[v1].assigned_type = 0;
		}
	}
	else if(policy == 3) {//most efficient
		for(; vp.first!=vp.second; ++vp.first) {
			Vertex v1 = *vp.first;
			int max_type=0; double max=std::ceil(testJob->g[v1].estTime[0]/60.0)*priceOnDemand[0];
			for(int i=1; i<types; i++) {
				double tmp = std::ceil(testJob->g[v1].estTime[i]/60.0)*priceOnDemand[i];
				if(tmp<max) max_type = i;
			}
			testJob->g[v1].assigned_type = max_type;
		}
	}
	//make sure the deadline, update start time and end time
	BFS_update(testJob);
	vp = vertices(testJob->g);
	Vertex vend = *(vp.second-1);
	if(policy == 1) {
		if(testJob->g[vend].end_time> testJob->deadline) {
			std::cout<<"deadline too tight to finish!"<<std::endl;
			return;
		}
	}
	else {
		while(testJob->g[vend].end_time> testJob->deadline) { //most gain to satisfy deadline
			
			/* only change one task at a time, do not work for DAGs like Ligo
				because Ligo may have two parallel critical path*/
			/*vp = vertices(testJob->g);
			std::vector<double> gains;
			int size_tasks = *vp.second - *vp.first - 1;
			gains.resize(size_tasks);

			//promote VM type of the tasks on CP and find the most gain one
			for(; vp.first!=(vp.second-1); ++vp.first) {				
				if(testJob->g[*vp.first].assigned_type < types-1) {
					testJob->g[*vp.first].prefer_type = testJob->g[*vp.first].assigned_type;
					testJob->g[*vp.first].assigned_type += 1;
				
					double t_old = testJob->g[vend].end_time;
					double cost_old = std::ceil(testJob->g[*vp.first].estTime[testJob->g[*vp.first].prefer_type]/60.0) *
						priceOnDemand[testJob->g[*vp.first].prefer_type];
					BFS_update(testJob);
					double t_new = testJob->g[vend].end_time;
					double cost_new = std::ceil(testJob->g[*vp.first].estTime[testJob->g[*vp.first].assigned_type]/60.0) *
						priceOnDemand[testJob->g[*vp.first].assigned_type];
					if(t_new < t_old && cost_new != cost_old)
						gains[*vp.first]= (t_old - t_new)/(cost_new - cost_old);
					//only let one task change type at a time
					testJob->g[*vp.first].assigned_type -= 1;
				}
			}
			double max = gains[0]; int max_id = 0;
			for(int i=1; i<size_tasks; i++)	{
				if(gains[i] > max) {
					max_id = i;
					max = gains[i];
				}
			}
			testJob->g[max_id].assigned_type += 1;
			BFS_update(testJob);*/

			//change two tasks at the same time
			vp = vertices(testJob->g);
			int size_tasks = *vp.second - *vp.first - 1;
			int task1 = (double)rand() / RAND_MAX * size_tasks;
			int task2 = (double)rand() / RAND_MAX * size_tasks;
			//std::cout<<task1<<", "<<task2;
			if(testJob->g[task1].assigned_type<types-1 && testJob->g[task2].assigned_type<types-1) {
				testJob->g[task1].assigned_type += 1;
				testJob->g[task2].assigned_type += 1;
				double t_old = testJob->g[vend].end_time;
				BFS_update(testJob);
				double t_new = testJob->g[vend].end_time;
				if(!(t_new < t_old)) {
					testJob->g[task1].assigned_type -= 1;
					testJob->g[task2].assigned_type -= 1;
				}
			}
		}
	}
}
//jobs are currently waiting in the queue for planning
void GanttConsolidation::Planner(std::vector<Job*> jobs, int threshold)
{
	int count = 0;
	//construct a priority queue to store VMs that hourly covers tasks' requests
	std::vector<VM*> VM_queue[types];
	std::pair<vertex_iter, vertex_iter> vp; 
	//std::priority_queue<VM*> VM_pque;
	for(int i=0; i<jobs.size(); i++) {		
		vp = vertices(jobs[i]->g);
		for(; vp.first!=vp.second; ++vp.first) {
			VM* vm = new VM();
			vm->assigned_tasks.push_back(&jobs[i]->g[*vp.first]);
			vm->start_time = jobs[i]->g[*vp.first ].start_time;
			vm->life_time = std::ceil((jobs[i]->g[*vp.first ].end_time-jobs[i]->g[*vp.first ].start_time)/60)*60;			
			vm->resi_time = vm->life_time - (jobs[i]->g[*vp.first ].end_time-jobs[i]->g[*vp.first ].start_time);
			vm->end_time = vm->start_time + vm->life_time - vm->resi_time;
			vm->type = jobs[i]->g[*vp.first ].assigned_type;
			//vm->dependentVMs.insert();
			//vm->price = priceOnDemand[vm->type];
			VM_queue[vm->type].push_back(vm);
		}
	}

	do{	
		//sort according to the start time of each VM
		for(int i=0; i<types; i++) 
			std::sort(VM_queue[i].begin(), VM_queue[i].end(),vmfunction);
		
		//start the operators 
		//if this condition, do move or split
		double residualtime = 20.0; 
		bool domove = false;
		bool dosplit = false;
		double costsplit = 0;
		double costmove = 0;
		for(int i=0; i<types; i++){
			for(int j=0; j<VM_queue[i].size(); j++) {
				if(VM_queue[i][j]->resi_time > residualtime) {//residual time is long
					for(int k=0; k<VM_queue[i].size(); k++) {
						//task k starts before the end of task j, then move k to the end of task j and merge them
						if(VM_queue[i][k]->start_time < VM_queue[i][j]->end_time && (60.0 - VM_queue[i][k]->resi_time) < VM_queue[i][j]->resi_time) { 
							//Job* Jobk = jobs[VM_queue[i][k]->curr_task->job_id];
							//Job* Jobj = jobs[VM_queue[i][j]->curr_task->job_id];
							//vp = vertices(Jobk->g);
							//int vend = *(vp.second-1);
							double moveafter = VM_queue[i][j]->start_time - VM_queue[i][k]->start_time + VM_queue[i][j]->life_time- VM_queue[i][j]->resi_time;
							bool deadlineenough = true;
							for(int taskiter=0; taskiter < VM_queue[i][k]->assigned_tasks.size(); taskiter++) {
								Job* jobinvm = jobs[VM_queue[i][k]->assigned_tasks[taskiter]->job_id];
								vp = vertices(jobinvm->g);
								int vend = *(vp.second - 1);
								if(jobinvm->g[vend].end_time + moveafter > jobinvm->deadline) {
									deadlineenough = false;
									break;
								}
							}
							if(deadlineenough) {//deadline is long enough
								domove = true;
								double t1 = VM_queue[i][j]->life_time - VM_queue[i][j]->resi_time;
								double t2 = VM_queue[i][k]->life_time - VM_queue[i][k]->resi_time;
								costmove = move(i,t1,t2);
								move_operation1(jobs,VM_queue[i][k],VM_queue[i][j],moveafter);
								break;
							}
							else {//at least one task in VM k cannot finish before deadline, need to split VM j 
								//double moveafter = VM_queue[i][j]->start_time - VM_queue[i][k]->start_time;//j move after
								//vp = vertices(Jobj->g);
								//int vend1 = *(vp.second -1);
								//double moveafter1 = VM_queue[i][k]->life_time - VM_queue[i][k]->resi_time;
								////if let VM k run first and delay VM j, deadlines are not violated
								//if(Jobk->g[vend].end_time + moveafter < Jobk->deadline && Jobj->g[vend1].end_time + moveafter1 < Jobj->deadline) {
								//	dosplit = true;
								//	//costsplit = split(1,1,1,1);
								//	break;
								//}
								if(VM_queue[i][k]->start_time < VM_queue[i][j]->start_time) {//move vm j afterward to the end of vm k
									bool deadlineshort = true;
									double jmoveafter = VM_queue[i][k]->end_time - VM_queue[i][j]->start_time;
									for(int taskiter =0; taskiter < VM_queue[i][j]->assigned_tasks.size(); taskiter++) {
										Job* curr_job = jobs[VM_queue[i][j]->assigned_tasks[taskiter]->job_id];
										vp = vertices(curr_job->g);
										int vend = *(vp.second - 1);
										if(curr_job->g[vend].end_time + jmoveafter > curr_job->deadline) {
											deadlineshort = false;
											break;
										}
									}
									if(deadlineshort) { //move vm j to the end of vm k do not violate deadline
										dosplit = true;
										move_operation1(jobs,VM_queue[i][j],VM_queue[i][k],jmoveafter);
										break;
									}
								}
								else { //split vm j to let vm k run first

								}
							}
						}
						else if((VM_queue[i][k]->start_time > VM_queue[i][j]->end_time) && (VM_queue[i][k]->end_time < VM_queue[i][j]->start_time +VM_queue[i][j]->life_time)) {
							domove = true;
							double t1 = VM_queue[i][j]->life_time - VM_queue[i][j]->resi_time;
							double t2 = VM_queue[i][k]->life_time - VM_queue[i][k]->resi_time;
							costmove = move(i,t1,t2);
							//do not need to update any task
							move_operation2();
							break;
						}
					}
					// if(domove || dosplit) break; 
				}	
			}
			// if(domove || dosplit) break;
		}
		//if satisfy this condition, do promote or demote
		bool dopromote = false;
		bool dodemote = false;
		double costpromote = 0;
		double costdemote = 0;
		std::vector<VM*> VMs;
		for(int i=0; i<types; i++) 
			for(int j=0; j<VM_queue[i].size(); j++) 
				VMs.push_back(VM_queue[i][j]);
		std::sort(VMs.begin(),VMs.end(),vmfunction);
		//demote
		std::vector<std::pair<double, std::pair<int, int> > > costs_demote;//costdemote, the ith VM to demote, demote to j type
		//promote
		std::vector<std::pair<double, std::pair<int, int> > > costs_promote;//costpromote, the ith VM to promote, promote to j type

		for(int i=0; i<VMs.size(); i++){
			//demote from worst to type-1, select the one do not violate deadline and has the highest gain
			//when demote, consider the merge cost
			for(int taskiter = 0; taskiter<VMs[i]->assigned_tasks.size(); taskiter++) {
				Job* currentJob = jobs[VMs[i]->assigned_tasks[taskiter]->job_id];
				vp = vertices(currentJob->g);
				int vend = *(vp.second -1);
				for(int j=0; j<VMs[i]->type; j++) {
					if(currentJob->g[vend].end_time + VMs[i]->assigned_tasks[taskiter]->estTime[j] - VMs[i]->assigned_tasks[taskiter]->estTime[VMs[i]->type] < currentJob->deadline) {
						dodemote = true;
						costdemote = demote(VMs[i]->type,VMs[i]->assigned_tasks[taskiter]->estTime[VMs[i]->type],j,VMs[i]->assigned_tasks[taskiter]->estTime[j]);
						//demote vm i to type j
						VMs[i]->type = j;
						double lastend = 0;
						for(int taskiter=0; taskiter< VMs[i]->assigned_tasks.size(); taskiter++) {
							VMs[i]->assigned_tasks[taskiter]->assigned_type = j;
							time_flood(jobs[VMs[i]->assigned_tasks[taskiter]->job_id],VMs[i]->assigned_tasks[taskiter]->name);
							lastend = (lastend > VMs[i]->assigned_tasks[taskiter]->end_time)?lastend : VMs[i]->assigned_tasks[taskiter]->end_time;
						}
						VMs[i]->end_time = lastend;
						VMs[i]->life_time = std::ceil((VMs[i]->end_time - VMs[i]->start_time)/60.0 )*60.0;
						VMs[i]->resi_time = VMs[i]->start_time + VMs[i]->life_time - VMs[i]->end_time;
						//test for merge
						for(int k=0; k<VM_queue[j].size(); k++) {
							if(VM_queue[j][k]->end_time < VMs[i]->start_time &&(VMs[i]->end_time < VM_queue[j][k]->start_time + VM_queue[j][k]->life_time)){ //can merge
									double t1 = VMs[i]->life_time - VMs[i]->resi_time;
									double t2 = VM_queue[j][k]->life_time - VM_queue[j][k]->resi_time;
									costdemote += move(j,t1,t2);
									//merge the demotion with vm k

									break;
							}
						}
						std::pair<double,std::pair<int, int> > cost_demote;
						std::pair<int,int> demote_plan;
						cost_demote.first = costdemote;
						demote_plan.first = i; demote_plan.second = j;
						cost_demote.second = demote_plan;
						costs_demote.push_back(cost_demote);
					}
				}
				//promote
				for(int j=VMs[i]->type+1; j<types; j++) {
					for(int k=0; k<VM_queue[j].size(); k++) {
						if(VM_queue[j][k]->curr_task->end_time < VMs[i]->start_time)
							if(60.0 - VMs[i]->resi_time < VM_queue[j][k]->start_time + VM_queue[j][k]->life_time - VMs[i]->start_time){ //can merge
								double t1 = VMs[i]->life_time - VMs[i]->resi_time;
								double t2 = VM_queue[j][k]->life_time - VM_queue[j][k]->resi_time;
								double savebymerge = move(j,t1, t2);
								double costbypromote = ppromote(VMs[i]->type,VMs[i]->curr_task->estTime[VMs[i]->type],j,VMs[i]->curr_task->estTime[j]);
								if(savebymerge + costbypromote > 0) {
									//dopromote = true;
									std::pair<double, std::pair<int,int> > cost_promote;
									cost_promote.first = savebymerge + costbypromote;
									std::pair<int, int> promote_plan;
									promote_plan.first = i;
									promote_plan.second = j;
									costs_promote.push_back(cost_promote);
								}
							}
					}
				}
			}

		}
		if(costs_demote.size() > 0) {
			std::sort(costs_demote.begin(),costs_demote.end(),comp_by_first);
			costdemote = costs_demote[costs_demote.size()-1].first;
			dodemote = true;
			//demote VM i in VMs to type j

		}
		if(costs_promote.size() > 0) {
			std::sort(costs_promote.begin(),costs_promote.end(),comp_by_first);
			costpromote = costs_promote[costs_promote.size()-1].first;
			dopromote = true;
			//promote VM i in VMs to type j
		}
		bool docoschedule = false;
		bool domerge = false;
		double costmerge = 0;
		double costcoschedule = 0;
		//co-scheduling
		double degradation = 0.2;
		for(int i=0; i<VMs.size(); i++) {
			
		}
		//merge
		for(int i=0; i<types; i++)
			for(int j=0; j< VM_queue[i].size(); j++) {
				//do until nothing can be done, merge with the VM before it
				for(int k=0; k<j; k++){
					if(VM_queue[i][k]->curr_task->end_time < VM_queue[i][j]->start_time &&
						VM_queue[i][k]->resi_time + VM_queue[i][j]->resi_time < 60) {// can merge
						domerge = true;
						double t1 = VM_queue[i][k]->curr_task->estTime[i];
						double t2 = VM_queue[i][j]->curr_task->estTime[j];
						costmerge += move(i,t1,t2);
					}
				}
			}
		if(!(domove || dosplit || dodemote || dopromote || docoschedule || domerge)) count++;
	}while(count<threshold);
}
void GanttConsolidation::Simulate(Job testJob)
{
	std::vector<Job*> workflows; //continuous workflow
	double arrival_time = 0;
	Job* job = new Job(pipeline, deadline, lamda);
	job->g = testJob.g;
	job->arrival_time = 0;
	workflows.push_back(job);
	double t = 0; bool condition = false;
	double moneycost = 0;
	std::ifstream infile;
	std::string a = "arrivaltime_integer_", b, c = ".txt";
	std::ostringstream strlamda;
	strlamda << lamda;
	b = strlamda.str();
	std::string fname = a + b + c;
	char time[256];
	infile.open(fname.c_str());
	infile.getline(time,256); //jump the lamda line
	infile.getline(time,256); //jump the 0 line
	//incomming jobs
	while(arrival_time < max_t){
		infile.getline(time,256);
		arrival_time = atof(time);

		Job* job = new Job(pipeline,deadline+arrival_time,lamda);
		job->g = testJob.g;
		job->arrival_time = arrival_time;
		//std::pair<vertex_iter, vertex_iter> vp = vertices(job->g);
		workflows.push_back(job);
	}

	//start simulation
	int pointer = 0;
	int period = 60; //planning every an hour
	int threshold = 10; //when more than thrshold times no gain, stop the operations
	int timer = 0;
	while(timer<max_t) {
		std::vector<Job*> jobs;
		//pop all jobs waiting and start planning
		for(int i=pointer; i<workflows.size(); i++)	{
			if(workflows[i]->arrival_time - workflows[pointer]->arrival_time < period)
				jobs.push_back(workflows[i]);
			else { pointer = i; break;	}
		}
		//for tasks to refer to the job they belong to 
		for(int i=0; i<jobs.size(); i++) {
			jobs[i]->name = i;
			std::pair<vertex_iter, vertex_iter> vp = vertices(jobs[i]->g);
			for(; vp.first != vp.second; ++vp.first) {
				jobs[i]->g[*vp.first].job_id = i;
			}
		}
		Planner(jobs, threshold);

		timer ++;

	}
}