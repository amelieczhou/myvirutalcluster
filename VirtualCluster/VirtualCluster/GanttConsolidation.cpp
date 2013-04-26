#include "stdafx.h"
#include "Provision.h"
#include "ConsolidateOperators.h"
#include "Algorithms.h"
#include <fstream>
#include <string>
#include <sstream>
//#include <boost/graph/topological_sort.hpp>

extern double max_t;
extern bool DynamicSchedule;
extern double deadline;
extern double chk_period;
extern bool sharing;
extern bool on_off;
extern double lamda;
extern double QoS;

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
double GanttConsolidation::Planner(std::vector<Job*> jobs, int threshold)
{
	if(jobs.size() == 0)
		return 0;
	int count = 0;
	double cost = 0;
	//construct a priority queue to store VMs that hourly covers tasks' requests
	std::vector<VM*> VM_queue[types];
	std::pair<vertex_iter, vertex_iter> vp; 
	//std::priority_queue<VM*> VM_pque;
	for(int i=0; i<jobs.size(); i++) {		
		vp = vertices(jobs[i]->g);
		for(; vp.first!=vp.second; ++vp.first) {
			if(jobs[i]->g[*vp.first].start_time!=jobs[i]->g[*vp.first].end_time){
				VM* vm = new VM();
				vm->type = jobs[i]->g[*vp.first ].assigned_type;
				vector<taskVertex*> cotasks;
				cotasks.push_back(&jobs[i]->g[*vp.first]);
				vm->assigned_tasks.push_back(cotasks);
				//vm->assigned_tasks.push_back(&jobs[i]->g[*vp.first]);
				vm->start_time = jobs[i]->g[*vp.first ].start_time;
				vm->life_time = std::ceil((jobs[i]->g[*vp.first ].end_time-jobs[i]->g[*vp.first ].start_time)/60)*60;			
				vm->resi_time = vm->life_time - (jobs[i]->g[*vp.first ].end_time-jobs[i]->g[*vp.first ].start_time);
				vm->end_time = vm->start_time + vm->life_time - vm->resi_time;
				
				//vm->dependentVMs.insert();
				//vm->price = priceOnDemand[vm->type];
				VM_queue[vm->type].push_back(vm);
			}
		}
	}
	int num_vms = 0;
	//define capacity of each type of VMs
	for(int i=0; i<types; i++)
		for(int j=0; j<VM_queue[i].size(); j++) {
			VM_queue[i][j]->capacity = std::pow(2.0,i) - 1; //already 1 in the vm
			num_vms += 1;
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
				bool condition = false;
				int outiter;
				double endtime;
				double starttime;
				double leftovertime;
				for(int out=0; out<VM_queue[i][j]->assigned_tasks.size(); out++){
					int insize = VM_queue[i][j]->assigned_tasks[out].size();
					//keep sorted at runtime
					//sort(VM_queue[i][j]->assigned_tasks[out].begin(),VM_queue[i][j]->assigned_tasks[out].end(),taskfunction);
					leftovertime = VM_queue[i][j]->start_time + VM_queue[i][j]->life_time - VM_queue[i][j]->assigned_tasks[out][insize-1]->end_time;
					if(leftovertime > residualtime)	{
						condition = true;
						outiter = out;
						endtime = VM_queue[i][j]->assigned_tasks[out][insize-1]->end_time;
						starttime = VM_queue[i][j]->assigned_tasks[out][0]->start_time;
						break;
					}
				}
				//move with the out-th task queue of the ij vm
				if(condition) { //out-th queue residual time longer than threshold
					for(int k=0; k<VM_queue[i].size(); k++){
						if(k!=j && VM_queue[i][k]->assigned_tasks.size()==1){//only merge with a VM without coscheduling tasks
							//task k starts before the end of task j, then move k to the end of task j and merge them
							// ik:        |----------         |
							// ij: --------------
							if(VM_queue[i][k]->start_time < endtime && (60.0 - VM_queue[i][k]->resi_time) < leftovertime) {//can merge
								double moveafter = endtime - VM_queue[i][k]->start_time;
								bool deadlineok = true;
								for(int in = 0; in<VM_queue[i][k]->assigned_tasks[0].size(); in++){
									Job* jobinvm = jobs[VM_queue[i][k]->assigned_tasks[0][in]->job_id];
									vp = vertices(jobinvm->g);
									int vend = *(vp.second - 1);
									if(jobinvm->g[vend].end_time + moveafter > jobinvm->deadline) {
										deadlineok = false;
										break;
									}
								}
								if(deadlineok) {//deadline is long enough to move k afterwards
									double t1 = endtime - starttime;
									double t2 = VM_queue[i][k]->life_time - VM_queue[i][k]->resi_time;
									costmove = move(i,t1,t2);
									if(costmove > 0) {		
										domove = true;
										move_operation1(jobs,VM_queue[i][k],VM_queue[i][j],outiter,moveafter);
										VM_queue[i].erase(VM_queue[i].begin()+k);									
									}
									break;//or k--;
								}
								else if(VM_queue[i][k]->start_time < starttime)//move vm j afterward to the end of vm k
								{//if at least one task in VM k cannot cannot finish before deadline, need to split VM j
									bool deadlineok = true;
									double jmoveafter = VM_queue[i][k]->end_time - starttime;
									if(jmoveafter < 0) jmoveafter = 0;
									for(int out=0; out<VM_queue[i][j]->assigned_tasks.size();out++){
										for(int in=0; in<VM_queue[i][j]->assigned_tasks[out].size();in++){
											Job* curr_job = jobs[VM_queue[i][j]->assigned_tasks[out][in]->job_id];
											vp = vertices(curr_job->g);
											int vend = *(vp.second - 1);
											if(curr_job->g[vend].end_time + jmoveafter > curr_job->deadline) {
												deadlineok = false;
												break;
											}
										}
									}
									if(deadlineok){ //move vm j to the end of vm k do not violdate deadline							
										double t1 = endtime - starttime;
										double t2 = VM_queue[i][k]->life_time - VM_queue[i][k]->resi_time;
										costmove = move(i,t1,t2);
										if(costmove > 0) {
											dosplit = true;
											split_operation(jobs,VM_queue[i][j],outiter,VM_queue[i][k],jmoveafter);
											VM_queue[i].erase(VM_queue[i].begin()+k);
											//VM_queue[i].erase(VM_queue[i].begin()+j);
										}
										break; //or k--;
									}
								}								
							}
						else if(VM_queue[i][k]->start_time > endtime && (60.0 - VM_queue[i][k]->resi_time)< VM_queue[i][j]->end_time - VM_queue[i][k]->start_time){
								//VM k starts before the lifetime of j, but after the end of j
								//ik:              |-------
								//ij: |-----------
								double t1 = endtime - starttime;
								double t2 = VM_queue[i][k]->life_time - VM_queue[i][k]->resi_time;
								costmove = move(i, t1,t2);
								//do not need to update any task
								if(costmove > 0) {
									domove = true;
									move_operation2(jobs, VM_queue[i][k], VM_queue[i][j],outiter);
									VM_queue[i].erase(VM_queue[i].begin()+k);
								}
								break; //or k--
							}
						}
						if(domove || dosplit) break;
					}
				}	
			}
			 if(domove || dosplit) break;
		}
		//the number of VMs saved by move
		int num_vms_move = 0;
		for(int i=0; i<types; i++)
			for(int j=0; j<VM_queue[i].size(); j++)
				if(!VM_queue[i][j]->assigned_tasks.empty()) num_vms_move += 1;
		int num_vms_saved = num_vms - num_vms_move;
		//printf("The overall number of VMs saved by move operation is %d\n",num_vms_saved);

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

		for(int i=0; i<VMs.size(); i++){
			//demote from worst to type-1, select the one do not violate deadline and has the highest gain
			//when demote, consider the merge cost			
			for(int j=0; j<VMs[i]->type; j++){
				//demote to type j
				if(pow(2.0,VMs[i]->type) - std::pow(2.0,j) <= VMs[i]->capacity){//leftover capacity can support demotion
					bool deadlineok = true;
					for(int out=0; out<VMs[i]->assigned_tasks.size(); out++){	
						double moveafter=0;
						for(int in=0; in<VMs[i]->assigned_tasks[out].size(); in++){							
							Job* currentJob = jobs[VMs[i]->assigned_tasks[out][in]->job_id];
							vp = vertices(currentJob->g);
							double exetime = VMs[i]->assigned_tasks[out][in]->end_time - VMs[i]->assigned_tasks[out][in]->start_time;
							double newexetime = exetime*VMs[i]->assigned_tasks[out][in]->estTime[j]/VMs[i]->assigned_tasks[out][in]->estTime[VMs[i]->type];
							if(currentJob->g[*(vp.second-1)].end_time + moveafter + newexetime - exetime > currentJob->deadline)
							{
								deadlineok = false;
								break;
							}
							if(in<VMs[i]->assigned_tasks[out].size()-1){
								double blank = VMs[i]->assigned_tasks[out][in+1]->start_time - VMs[i]->assigned_tasks[out][in]->end_time;
								moveafter += newexetime - exetime - blank;
								if(moveafter < 0) moveafter = 0;
							}
						}
						if(!deadlineok) break;
					}
					if(deadlineok) {
						dodemote = true;
						//demote vm i to type j
						int oldtype = VMs[i]->type;
						demote_operation(jobs,VMs[i],j);
						VM_queue[j].push_back(VMs[i]);
						for(int iter=0; iter<VM_queue[oldtype].size();iter++)
							if(VM_queue[oldtype][iter] == VMs[i])
								VM_queue[oldtype].erase(VM_queue[oldtype].begin()+iter);
						//test for merge
						bool merge = false;
						for(int k=0; k<VM_queue[j].size()-1; k++){
							if(VM_queue[j][k]->assigned_tasks.size()==1) {
								for(int out=0; out<VMs[i]->assigned_tasks.size(); out++){
									int taskend = VMs[i]->assigned_tasks[out].size()-1;		
									bool condition = (VM_queue[j][k]->start_time > VMs[i]->assigned_tasks[out][taskend]->end_time)&&(
										VM_queue[j][k]->end_time < VMs[i]->end_time);
									if(condition){			
										merge = true;
										double t1 = VMs[i]->assigned_tasks[out][taskend]->end_time - VMs[i]->assigned_tasks[out][0]->start_time;
										double t2 = VM_queue[j][k]->life_time - VM_queue[j][k]->resi_time;
										//move VM k to the end of the out-th queue of VM i
										move_operation2(jobs,VM_queue[j][k],VMs[i],out);
										VM_queue[j].erase(VM_queue[j].begin()+k);
										int pos=0;
										for(; pos<VMs.size(); pos++)
											if(VMs[pos]==VM_queue[j][k]) break;
										VMs.erase(VMs.begin() + pos);
									}
									if(merge) break;
								}
							}
							if(merge) break;
						}
					}
				}
				if(dodemote) break;
			}
			
			for(int j=VMs[i]->type+1; j<types; j++) {
				//promote to type j
				double oldtime =0, newtime =0;
				for(int out=0; out<VMs[i]->assigned_tasks.size(); out++)
					for(int in=0; in<VMs[i]->assigned_tasks[out].size(); in++) {
						newtime = (newtime > VMs[i]->assigned_tasks[out][in]->estTime[j])?newtime:VMs[i]->assigned_tasks[out][in]->estTime[j];
						oldtime = (oldtime > VMs[i]->assigned_tasks[out][in]->estTime[VMs[i]->type])?oldtime:VMs[i]->assigned_tasks[out][in]->estTime[VMs[i]->type];
					}
				double costbypromote = ppromote(VMs[i]->type,oldtime,j,newtime);
				//find out if there is a vm to be merged, if no, dont promote
				for(int k=0; k<VM_queue[j].size(); k++) {
					if(VM_queue[j][k]->assigned_tasks.size()==1) {
						for(int out=0; out<VMs[i]->assigned_tasks.size(); out++) {
							int endtask = VMs[i]->assigned_tasks[out].size() -1;
							if(VM_queue[j][k]->start_time>VMs[i]->assigned_tasks[out][endtask]->end_time && VM_queue[j][k]->end_time < VMs[i]->start_time+VMs[i]->life_time){//?????
								//k in front of i.out
								//first promote then merge								
								double t1 = VM_queue[j][k]->end_time - VM_queue[j][k]->start_time;
								double t2 = VMs[i]->assigned_tasks[out][endtask]->end_time - VMs[i]->assigned_tasks[out][0]->start_time;
								double savebymerge = move(j,t1,t2);
								if(savebymerge+costbypromote > 0) {
									dopromote = true;
									int type = VMs[i]->type;
									int pos = 0;
									for(; pos<VM_queue[type].size(); pos++ )
										if(VM_queue[type][pos] == VMs[i]) break;
									promote_operation(jobs,VMs[i],j);	
									VM_queue[j].push_back(VMs[i]);
									VM_queue[type].erase(VM_queue[type].begin()+pos);
									move_operation2(jobs,VM_queue[j][k],VMs[i],out);
									VM_queue[j].erase(VM_queue[j].begin()+k);
									break;	
								}
							}	
							if(dopromote) break;
						}						
					}
					if(dopromote) break;
				}	
				if(dopromote) break;
			}
			if(dodemote || dopromote) break;
		}

		bool docoschedule = false;
		bool domerge = false;
		double costmerge = 0;
		double costcoschedule = 0;
		//co-scheduling
		//double degradation = 0.2;
		for(int i=0; i<VMs.size(); i++) {
			if(VMs[i]->capacity > 0){ //have space for co-scheduling
				bool deadlineoki = true;
				double tightest_before_deadline_i = VMs[i]->assigned_tasks[0][0]->sub_deadline - VMs[i]->assigned_tasks[0][0]->end_time;				
				for(int out=0; out<VMs[i]->assigned_tasks.size(); out++) {
					for(int in=0; in<VMs[i]->assigned_tasks[out].size(); in++) {
						Job* job=jobs[VMs[i]->assigned_tasks[out][in]->job_id];
						vp = vertices(job->g);
						if(job->g[0].start_time + (job->g[*(vp.second-1)].end_time-job->g[0].start_time)*(1+degradation) > job->deadline) {
							deadlineoki = false;
							break;
						}
						if(VMs[i]->assigned_tasks[out][in]->sub_deadline - VMs[i]->assigned_tasks[out][in]->sub_deadline < tightest_before_deadline_i)
							tightest_before_deadline_i = VMs[i]->assigned_tasks[out][in]->sub_deadline - VMs[i]->assigned_tasks[out][in]->end_time;				
					}
					if(!deadlineoki) break;
				}
				if(deadlineoki) {
					for(int j=0; j<VMs.size(); j++) {				
						if(i != j && VMs[i]->capacity - (std::pow(2.0,VMs[j]->type)-VMs[j]->capacity) > 0) {
							double tightest_before_deadline_j = 0;
							double moveafter;
							if(VMs[j]->start_time < VMs[i]->start_time) 
								moveafter = VMs[i]->start_time - VMs[j]->start_time;
							else moveafter = 0;
							bool deadlineokj = true;
							for(int out=0; out<VMs[j]->assigned_tasks.size(); out++){
								for(int in=0; in<VMs[j]->assigned_tasks[out].size(); in++){
									Job* job = jobs[VMs[j]->assigned_tasks[out][in]->job_id];
									vp = vertices(job->g);
									if(job->g[0].start_time + (job->g[*(vp.second-1)].end_time-job->g[0].start_time)*(1+degradation) > job->deadline){
										deadlineokj = false;
										break;
									}
								} if(!deadlineokj) break;
							}
							if(deadlineokj){
								tightest_before_deadline_j = VMs[j]->assigned_tasks[0][0]->sub_deadline - VMs[j]->assigned_tasks[0][0]->end_time;
								for(int out=0; out<VMs[j]->assigned_tasks.size(); out++)
									for(int in=0; in<VMs[j]->assigned_tasks[out].size(); in++)
										if(VMs[j]->assigned_tasks[out][in]->sub_deadline - VMs[j]->assigned_tasks[out][in]->sub_deadline < tightest_before_deadline_j)
											tightest_before_deadline_j = VMs[j]->assigned_tasks[out][in]->sub_deadline - VMs[j]->assigned_tasks[out][in]->end_time;
								
								if(std::abs((tightest_before_deadline_i - tightest_before_deadline_j)/tightest_before_deadline_i) < 0.1) { //distances to deadline are close
									/*double t1 = VMs[i]->end_time-VMs[i]->start_time;
									double t2 = VMs[j]->end_time - VMs[j]->start_time;
									double t3 = t1;
									double t4 = 0;
									for(int taskiter=0; taskiter<VMs[j]->assigned_tasks.size(); taskiter++)
										if(t4 < VMs[j]->assigned_tasks[taskiter]->end_time) t4 = VMs[j]->assigned_tasks[taskiter]->end_time;
									double coschedulecost = coschedule(VMs[i]->type,VMs[j]->type,VMs[i]->type,t1,t2,t3,t4,degradation);
									if(coschedulecost > 0){*/
										docoschedule = true;
										int type = VMs[j]->type;
										int pos = 0;
										for(; pos < VM_queue[type].size(); pos++)
											if(VM_queue[type][pos] == VMs[j]) break;
										coschedule_operation(jobs,VMs[i],VMs[j],degradation);
										VMs.erase(VMs.begin()+j);
										VM_queue[type].erase(VM_queue[type].begin()+pos);
										break;
									//}
								}
							}							
						}
					}
				}
			}
			if(docoschedule) break;
		}
		//merge
		for(int i=0; i<types; i++) {
			for(int k=0; k< VM_queue[i].size(); k++) {
				//do until nothing can be done, merge with the VM after it
				for(int j=0; j<VM_queue[i].size(); j++){
					if(VM_queue[i][k]->end_time < VM_queue[i][j]->start_time &&
						VM_queue[i][k]->resi_time + VM_queue[i][j]->resi_time < 60 &&
						VM_queue[i][k]->assigned_tasks.size() == 1 && VM_queue[i][j]->assigned_tasks.size() == 1) {// can merge
						domerge = true;
						double t1 = VM_queue[i][k]->end_time - VM_queue[i][k]->start_time;
						double t2 = VM_queue[i][j]->end_time - VM_queue[i][j]->start_time;
						costmerge += move(i,t1,t2);
						int pos = 0;
						for(; pos<VMs.size(); pos++)
							if(VMs[pos] == VM_queue[i][j]) break;
						move_operation2(jobs,VM_queue[i][j],VM_queue[i][k],0);
						VM_queue[i].erase(VM_queue[i].begin()+j);
						VMs.erase(VMs.begin()+pos);
						break;//break or continue?? j--;
					}
				}
				if(domerge) break;
			}
			if(domerge) break;
		}
		if(!(domove || dosplit || dodemote || dopromote || docoschedule || domerge)) count++;
	}while(count<threshold);
	for(int i=0; i<types; i++)
		for(int j=0; j<VM_queue[i].size(); j++)
			cost += priceOnDemand[i]*VM_queue[i][j]->life_time;
	
	return cost;
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
	double totalcost = 0;
	while(timer < max_t) {
		std::vector<Job*> jobs;
		//pop all jobs waiting and start planning
		if(timer%period == 0) {
			for(int i=pointer; i<workflows.size(); i++) {
				if(workflows[i]->arrival_time < timer)
					jobs.push_back(workflows[i]);
				else {pointer = i; break;}
			}
		
			//for tasks to refer to the job they belong to 
			for(int i=0; i<jobs.size(); i++) {
				jobs[i]->name = i;
				std::pair<vertex_iter, vertex_iter> vp = vertices(jobs[i]->g);
				for(; vp.first != vp.second; ++vp.first) {
					jobs[i]->g[*vp.first].job_id = i;
					jobs[i]->g[*vp.first].start_time += jobs[i]->arrival_time;
					jobs[i]->g[*vp.first].end_time += jobs[i]->arrival_time;
					jobs[i]->g[*vp.first].sub_deadline += jobs[i]->arrival_time;
				}
			}
			printf("current time is: %d\n", timer);
			double cost = Planner(jobs, threshold);
			totalcost += cost;
		}
		timer ++;
	}
	//the end of simulation
	//calculate the deadline violation rate
	int violate = 0;
	for(int i=0; i<workflows.size(); i++) {
		std::pair<vertex_iter, vertex_iter> vp;
		vp = vertices(workflows[i]->g);
		int vend = *(vp.second -1);
		if(workflows[i]->g[vend].end_time > workflows[i]->deadline)
			violate += 1;
	}
	double violate_rate = (double)violate/workflows.size();
	printf("deadline violation rate is: %4f\n",violate_rate);
	printf("total cost is: %4f\n",totalcost);
}