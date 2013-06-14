#include "stdafx.h"
#include "ConsolidateOperators.h"

double opCoschedule(vector<VM*>* VM_queue, vector<Job*> jobs,bool checkcost, bool estimate,bool timeorcost)
{
	bool docoschedule = false;
	bool domerge = false;
	double costmerge = 0;
	double costcoschedule = 0;
	double degradation = 0.25;
/*	//co-scheduling
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
					if(i != j && VMs[i]->capacity - (std::pow(2.0,VMs[j]->type)-VMs[j]->capacity) >= 0) {
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
								//double t1 = VMs[i]->end_time-VMs[i]->start_time;
								//double t2 = VMs[j]->end_time - VMs[j]->start_time;
								//double t3 = t1;
								//double t4 = 0;
								//for(int taskiter=0; taskiter<VMs[j]->assigned_tasks.size(); taskiter++)
								//	if(t4 < VMs[j]->assigned_tasks[taskiter]->end_time) t4 = VMs[j]->assigned_tasks[taskiter]->end_time;
								//double coschedulecost = coschedule(VMs[i]->type,VMs[j]->type,VMs[i]->type,t1,t2,t3,t4,degradation);
								//if(coschedulecost > 0){
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
	*/
	for(int i=0; i<types; i++){
		for(int j=0; j<VM_queue[i].size(); j++){
			for(int k=j+1; k<VM_queue[i].size(); k++){
				if(VM_queue[i][j]->assigned_tasks.size()==1 && VM_queue[i][k]->assigned_tasks.size()==1){
					if(VM_queue[i][j]->assigned_tasks[0].size()==1 && VM_queue[i][k]->assigned_tasks[0].size()==1){
						if(VM_queue[i][j]->start_time == VM_queue[i][k]->start_time && VM_queue[i][j]->end_time == VM_queue[i][j]->end_time){
							//can co-schedule
							for(int tj=0; tj<VM_queue[i][j]->assigned_tasks[0].size();tj++){
								//VM_queue[i][j]->assigned_tasks[0][tj]->estTime[]
								docoschedule = true;
								taskVertex* task= VM_queue[i][k]->assigned_tasks[0][0];
								VM_queue[i][k]->assigned_tasks[0].pop_back();
								vector<taskVertex*> tasks;
								tasks.push_back(task);
								VM_queue[i][j]->assigned_tasks.push_back(tasks);
								VM_queue[i][j]->capacity -= 1;
								VM_queue[i].erase(VM_queue[i].begin()+k);
								printf("co-scheduling operation\n");
								costcoschedule = priceOnDemand[i];
								return costcoschedule;
							}
						}
					}
				}
			
			}

		}
	
	}
	return 0;
}