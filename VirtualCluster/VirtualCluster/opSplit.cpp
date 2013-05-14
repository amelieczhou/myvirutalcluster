#include "stdafx.h"
#include "ConsolidateOperators.h"

double opSplit(vector<VM*>* VM_queue, vector<Job*> jobs,bool checkcost)
{
	bool dosplit = false;
	double costsplit = 0;

	vector<vector<VM*> > VM_queue_backup;
	for(int i=0; i<types; i++){
		vector<VM*> vms;		
		for(int j=0; j<VM_queue[i].size(); j++){
			VM* vm = new VM();
			*vm = *VM_queue[i][j];		
			vm->assigned_tasks.clear();
			for(int out=0; out<VM_queue[i][j]->assigned_tasks.size(); out++){
				vector<taskVertex*> tasks;
				for(int in=0; in<VM_queue[i][j]->assigned_tasks[out].size(); in++){
					taskVertex* task = new taskVertex();
					*task = *VM_queue[i][j]->assigned_tasks[out][in];
					tasks.push_back(task);
				}
				vm->assigned_tasks.push_back(tasks);				
			}
			vms.push_back(vm);
		}
		VM_queue_backup.push_back(vms);
	}
	double initialcost = 0;
	for(int i=0; i<types; i++)
		for(int j=0; j<VM_queue[i].size(); j++)
			initialcost += priceOnDemand[i]*VM_queue[i][j]->life_time/60.0;

	pair<vertex_iter, vertex_iter> vp;
	//first consider split the tasks on the same VM but with very large gap
	//case2: split a task to different VMs to use their residual time
	for(int i=0; i<types; i++){
		for(int j=0; j<VM_queue[i].size(); j++){
			if(VM_queue[i][j]->assigned_tasks.size() == 1){
				//find vms in vm queue i that the sum of their residual time is enough for task j execution
				for(int k1=0; k1<VM_queue[i].size(); k1++){
					if(k1 != j && VM_queue[i][k1]->start_time + VM_queue[i][k1]->life_time > VM_queue[i][j]->start_time){
						double moveafter1 = VM_queue[i][k1]->end_time - VM_queue[i][j]->start_time;
						if(moveafter1 <= 0) moveafter1 =0;
						bool deadlineok1 = true;
						for(int in=0; in<VM_queue[i][j]->assigned_tasks[0].size(); in++){
							double exetime = VM_queue[i][j]->assigned_tasks[0][in]->end_time - VM_queue[i][j]->assigned_tasks[0][in]->start_time;
							double oldestTime = VM_queue[i][j]->assigned_tasks[0][in]->estTime[VM_queue[i][j]->assigned_tasks[0][in]->assigned_type];
							VM_queue[i][j]->assigned_tasks[0][in]->start_time += moveafter1;
							VM_queue[i][j]->assigned_tasks[0][in]->estTime[VM_queue[i][j]->assigned_tasks[0][in]->assigned_type] = exetime;
							deadlineok1 = time_flood(jobs[VM_queue[i][j]->assigned_tasks[0][in]->job_id],VM_queue[i][j]->assigned_tasks[0][in]->name);
							VM_queue[i][j]->assigned_tasks[0][in]->estTime[VM_queue[i][j]->assigned_tasks[0][in]->assigned_type] = oldestTime;
							if(!deadlineok1) break;
						}
						if(!deadlineok1){
							//site recovery
							for(int t=0; t<types; t++)
								for(int s=0; s<VM_queue_backup[t].size(); s++){
									deepcopy(VM_queue[t][s], VM_queue_backup[t][s]);
								}
							continue;//try next vm k1 in queue i
						}
						vector<vector<VM*> > VM_queue_state;
						for(int iiter=0; iiter<types; iiter++){
							vector<VM*> vms;		
							for(int jiter=0; jiter<VM_queue[iiter].size(); jiter++){
								VM* vm = new VM();
								*vm = *VM_queue[iiter][jiter];		
								vm->assigned_tasks.clear();
								for(int out=0; out<VM_queue[iiter][jiter]->assigned_tasks.size(); out++){
									vector<taskVertex*> tasks;
									for(int in=0; in<VM_queue[iiter][jiter]->assigned_tasks[out].size(); in++){
										taskVertex* task = new taskVertex();
										*task = *VM_queue[iiter][jiter]->assigned_tasks[out][in];
										tasks.push_back(task);
									}
									vm->assigned_tasks.push_back(tasks);				
								}
								vms.push_back(vm);
							}
							VM_queue_state.push_back(vms);
						}

						int splittask = -1, splitindex = -1;
						for(int in=0; in<VM_queue[i][j]->assigned_tasks[0].size(); in++){
							if(VM_queue[i][k1]->start_time + VM_queue[i][k1]->life_time > VM_queue[i][j]->assigned_tasks[0][in]->start_time && 
								VM_queue[i][k1]->start_time + VM_queue[i][k1]->life_time < VM_queue[i][j]->assigned_tasks[0][in]->end_time)
								splittask = in;	
							if(in<VM_queue[i][j]->assigned_tasks[0].size()-1){
								if(VM_queue[i][k1]->start_time + VM_queue[i][k1]->life_time >= VM_queue[i][j]->assigned_tasks[0][in]->end_time &&
								VM_queue[i][k1]->start_time + VM_queue[i][k1]->life_time <= VM_queue[i][j]->assigned_tasks[0][in+1]->start_time)
									splitindex = in;
							}
							//else{//in==VM_queue[i][j]->assigned_tasks[0].size()-1
							//	if(VM_queue[i][k1]->start_time + VM_queue[i][k1]->life_time >= VM_queue[i][j]->assigned_tasks[0][in]->end_time)
							//		splitindex = in;
							//}								
						}
						if(splittask == -1 && splitindex == -1){
							//do not need to split a task, can put vm j entirely on vm k1
							double cost1=0;
							int numoftasks =  VM_queue[i][j]->assigned_tasks[0].size();
							for(int ttype=0; ttype<types; ttype++)
								for(int tsize=0; tsize<VM_queue[ttype].size(); tsize++)
									cost1 += priceOnDemand[VM_queue[ttype][tsize]->type]*VM_queue[ttype][tsize]->life_time /60.0;
							if(moveafter1 == 0)
								move_operation2(jobs, VM_queue[i][j], VM_queue[i][k1],0);
							else
								move_operation1(jobs, VM_queue[i][j], VM_queue[i][k1],0,moveafter1);
							bool update = updateVMqueue(VM_queue);
							double cost2=0;
							for(int ttype=0; ttype<types; ttype++)
								for(int tsize=0; tsize<VM_queue[ttype].size(); tsize++)
									cost2 += priceOnDemand[VM_queue[ttype][tsize]->type]*VM_queue[ttype][tsize]->life_time /60.0;
							if(cost2 >= cost1){
								vector<taskVertex*> tasks;
								for(int t=0; t<numoftasks; t++)	{
									taskVertex* task = VM_queue[i][k1]->assigned_tasks[0].back();
									VM_queue[i][k1]->assigned_tasks[0].pop_back();
									tasks.push_back(task);
								}
								VM_queue[i][j]->assigned_tasks.push_back(tasks);
								//site recovery 
								for(int t=0; t<types; t++){
									//VM_queue[t].resize(VM_queue_backup[t].size());
									for(int s=0; s<VM_queue_backup[t].size(); s++){
										deepcopy(VM_queue[t][s], VM_queue_backup[t][s]);									
									}
								}
								deepdelete(VM_queue_state);
								continue;//to the next k1
							}
							else{
								if(checkcost){
									vector<taskVertex*> tasks;
									for(int t=0; t<numoftasks; t++)	{
										taskVertex* task = VM_queue[i][k1]->assigned_tasks[0].back();
										VM_queue[i][k1]->assigned_tasks[0].pop_back();
										tasks.push_back(task);
									}
									VM_queue[i][j]->assigned_tasks.push_back(tasks);
									//site recovery 
									for(int t=0; t<types; t++){
										//VM_queue[t].resize(VM_queue_backup[t].size());
										for(int s=0; s<VM_queue_backup[t].size(); s++){
											deepcopy(VM_queue[t][s], VM_queue_backup[t][s]);									
										}
									}
									deepdelete(VM_queue_backup);
									deepdelete(VM_queue_state);
									return cost1-cost2;
								}
								VM_queue[i].erase(VM_queue[i].begin()+j);
								deepdelete(VM_queue_backup);
								deepdelete(VM_queue_state);
								if(moveafter1 == 0)
									printf("move operation 2\n");
								else printf("move operation 1\n");
								return cost1-cost2;
							}
						}else if(splittask == -1){
							//do not need to split task, can put some of the tasks in vm j to vm k1
							//find another vm to hold the rest part of vm j
							for(int k2=0; k2<VM_queue[i].size(); k2++){
								double starttime = VM_queue[i][j]->assigned_tasks[0][splitindex+1]->start_time;
								if(k1!=k2 && k2!=j && VM_queue[i][k2]->start_time + VM_queue[i][k2]->life_time > starttime){
									double moveafter2 = VM_queue[i][k2]->end_time - starttime;
									if(moveafter2 <0) moveafter2 = 0;
									bool deadlineok2 = true;
									for(int in=splitindex+1; in<VM_queue[i][j]->assigned_tasks[0].size(); in++){
										double exetime = VM_queue[i][j]->assigned_tasks[0][in]->end_time - VM_queue[i][j]->assigned_tasks[0][in]->start_time;
										double oldestTime = VM_queue[i][j]->assigned_tasks[0][in]->estTime[VM_queue[i][j]->assigned_tasks[0][in]->assigned_type];
										VM_queue[i][j]->assigned_tasks[0][in]->start_time += moveafter2;
										VM_queue[i][j]->assigned_tasks[0][in]->estTime[VM_queue[i][j]->assigned_tasks[0][in]->assigned_type] = exetime;
										deadlineok2 = time_flood(jobs[VM_queue[i][j]->assigned_tasks[0][in]->job_id],VM_queue[i][j]->assigned_tasks[0][in]->name);
										VM_queue[i][j]->assigned_tasks[0][in]->estTime[VM_queue[i][j]->assigned_tasks[0][in]->assigned_type] = oldestTime;
										if(!deadlineok2) break;
									}
									if(!deadlineok2){
										//site recovery
										for(int t=0; t<types; t++)
											for(int s=0; s<VM_queue_backup[t].size(); s++){
												deepcopy(VM_queue[t][s], VM_queue_state[t][s]);
											}
										continue;//try next vm k2 in queue i
									}
									double cost1=0;
									for(int ttype=0; ttype<types; ttype++)
										for(int tsize=0; tsize<VM_queue[ttype].size(); tsize++)
											cost1 += priceOnDemand[VM_queue[ttype][tsize]->type]*VM_queue[ttype][tsize]->life_time /60.0;
									int numoftasks = VM_queue[i][j]->assigned_tasks[0].size();
									for(int in=0; in<splitindex+1; in++){
										taskVertex* task = VM_queue[i][j]->assigned_tasks[0][in];
										VM_queue[i][k1]->assigned_tasks[0].push_back(task);
									}							
									for(int in=splitindex+1; in<numoftasks; in++){
										taskVertex* task =  VM_queue[i][j]->assigned_tasks[0][in];
										VM_queue[i][k2]->assigned_tasks[0].push_back(task);
									}
									VM_queue[i][j]->assigned_tasks.clear();
									VM_queue[i][j]->end_time=VM_queue[i][j]->start_time=VM_queue[i][j]->life_time=VM_queue[i][j]->resi_time=0;
									bool update = updateVMqueue(VM_queue);
									double cost2=0;
									for(int ttype=0; ttype<types; ttype++)
										for(int tsize=0; tsize<VM_queue[ttype].size(); tsize++)
											cost2 += priceOnDemand[VM_queue[ttype][tsize]->type]*VM_queue[ttype][tsize]->life_time /60.0;
									if(cost2>=cost1){
										vector<taskVertex*> tasks;
										for(int t=0; t<splitindex+1; t++){
											taskVertex* task = VM_queue[i][k1]->assigned_tasks[0].back();
											VM_queue[i][k1]->assigned_tasks[0].pop_back();
											tasks.push_back(task);
										}
										for(int t=0; t<numoftasks-splitindex-1; t++){
											taskVertex* task = VM_queue[i][k2]->assigned_tasks[0].back();
											VM_queue[i][k2]->assigned_tasks[0].pop_back();
											tasks.push_back(task);
										}
										VM_queue[i][j]->assigned_tasks.push_back(tasks);
										for(int t=0; t<types; t++)
											for(int s=0; s<VM_queue_backup[t].size(); s++)	
												deepcopy(VM_queue[t][s],VM_queue_state[t][s]);
										continue;
									}else{
										if(checkcost){
											vector<taskVertex*> tasks;
											for(int t=0; t<splitindex+1; t++){
												taskVertex* task = VM_queue[i][k1]->assigned_tasks[0].back();
												VM_queue[i][k1]->assigned_tasks[0].pop_back();
												tasks.push_back(task);
											}
											for(int t=0; t<numoftasks-splitindex-1; t++){
												taskVertex* task = VM_queue[i][k2]->assigned_tasks[0].back();
												VM_queue[i][k2]->assigned_tasks[0].pop_back();
												tasks.push_back(task);
											}
											VM_queue[i][j]->assigned_tasks.push_back(tasks);
											for(int t=0; t<types; t++)
												for(int s=0; s<VM_queue_backup[t].size(); s++)	
													deepcopy(VM_queue[t][s],VM_queue_state[t][s]);
											deepdelete(VM_queue_backup);
											deepdelete(VM_queue_state);
											return cost1-cost2;
										}
										VM_queue[i].erase(VM_queue[i].begin()+j);
										deepdelete(VM_queue_backup);
										deepdelete(VM_queue_state);
										printf("split without split a task\n");
										return cost1-cost2;
									}
								}
							}//for next k2	
							deepdelete(VM_queue_state);
						}//if splittask == -1	
						else if(splitindex == -1){
							//need to split a task, what to do with the data structure???
							deepdelete(VM_queue_state);
						}						
					}
				}//k1 for breaktime1
			}
		}//task j in VM type i
	}//type i

	for(int t=0; t<types; t++)
		for(int s=0; s<VM_queue_backup[t].size(); s++){
			deepcopy(VM_queue[t][s], VM_queue_backup[t][s]);
		}
	deepdelete(VM_queue_backup);	
	return 0;
}