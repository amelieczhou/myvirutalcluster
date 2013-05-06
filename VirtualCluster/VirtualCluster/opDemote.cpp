#include "stdafx.h"
#include "ConsolidateOperators.h"

double opDemote(vector<VM*>* VM_queue, vector<Job*> jobs)
{
	bool dodemote = false;
	double costdemote = 0;

	//save context for site recovery to the very beginning
	vector<vector<VM*> > VM_queue_backup;

	for(int i=0; i<types; i++){
		vector<VM*> vms;		
		for(int j=0; j<VM_queue[i].size(); j++){
			VM* vm = new VM();
			*vm = *VM_queue[i][j];	
			//VM_queue[i][j]->start_time = 100000;
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
	//VM_queue[3][0]->assigned_tasks[0][0]->start_time = 1000000;

	for(int i=0; i<types; i++){
		for(int j=0; j<i; j++){
			for(int iter=0; iter<VM_queue[i].size(); iter++){
				//demote VM_queue[i][iter] to type j
				//when demote, consider the merge cost		
				if(pow(2.0,i) - std::pow(2.0,j) <= VM_queue[i][iter]->capacity){//leftover capacity can support demotion
					bool deadlineok = true;
					double originaltime = VM_queue[i][iter]->end_time - VM_queue[i][iter]->start_time;

					for(int out=0; out<VM_queue[i][iter]->assigned_tasks.size(); out++){
						double moveafter = 0;
						for(int in=0; in<VM_queue[i][iter]->assigned_tasks[out].size(); in++){
							VM_queue[i][iter]->assigned_tasks[out][in]->assigned_type = j;
							VM_queue[i][iter]->assigned_tasks[out][in]->start_time += moveafter;
							deadlineok = time_flood(VM_queue[i][iter]->assigned_tasks[out][in]);
							double exetime = VM_queue[i][iter]->assigned_tasks[out][in]->end_time - VM_queue[i][iter]->assigned_tasks[out][in]->start_time;
							double newexetime = exetime*VM_queue[i][iter]->assigned_tasks[out][in]->estTime[j]/VM_queue[i][iter]->assigned_tasks[out][in]->estTime[i];
							if(!deadlineok) break;
							if(in<VM_queue[i][iter]->assigned_tasks[out].size()-1){
								double blank = VM_queue[i][iter]->assigned_tasks[out][in+1]->start_time - VM_queue[i][iter]->assigned_tasks[out][in]->end_time;
								moveafter += newexetime - exetime - blank;
								if(moveafter < 0) moveafter = 0;
							}
						}
						if(!deadlineok)  break;
					}
					if(deadlineok){
						dodemote = true;
						updateVMqueue(VM_queue);//update the start and endtime of each VM
						//save context at the status when the assigned type of tasks are changed
						vector<vector<VM*> > VM_queue_state;
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
							VM_queue_state.push_back(vms);
						}
						//demote vm i to type j
						double newtime = VM_queue[i][iter]->end_time - VM_queue[i][iter]->start_time;
						costdemote = demote(i, originaltime, j, newtime);
						if(costdemote>1e-12){
							demote_operation(jobs,VM_queue[i][iter],j);
							bool update = updateVMqueue(VM_queue);
							if(update){ //new cost less than old cost
								VM_queue[j].push_back(VM_queue[i][iter]);
								VM_queue[i].erase(VM_queue[i].begin()+iter);

								for(int t=0; t<types; t++){
									for(int s=0; s<VM_queue_backup[t].size(); s++){	
										for(int out=0; out<VM_queue_backup[t][s]->assigned_tasks.size();out++)
											for(int in=0; in<VM_queue_backup[t][s]->assigned_tasks[out].size(); in++)	{
												delete VM_queue_backup[t][s]->assigned_tasks[out][in];
												VM_queue_backup[t][s]->assigned_tasks[out][in] = NULL;
											}
										delete VM_queue_backup[t][s];
										VM_queue_backup[t][s] = NULL;
									}
									for(int s=0; s<VM_queue_state[t].size();s++){
										for(int out=0; out<VM_queue_state[t][s]->assigned_tasks.size();out++)
											for(int in=0; in<VM_queue_state[t][s]->assigned_tasks[out].size(); in++){
												delete VM_queue_state[t][s]->assigned_tasks[out][in];
												VM_queue_state[t][s]->assigned_tasks[out][in] = NULL;
											}

										delete VM_queue_state[t][s];
										VM_queue_state[t][s] = NULL;
									}
								}
								printf("demote operation\n");
								return costdemote;
							}else{
								for(int t=0; t<types; t++)
									for(int s=0; s<VM_queue_state[t].size(); s++){										
										VM_queue[t][s]->capacity=VM_queue_state[t][s]->capacity;
										VM_queue[t][s]->start_time=VM_queue_state[t][s]->start_time;
										VM_queue[t][s]->end_time=VM_queue_state[t][s]->end_time;
										VM_queue[t][s]->resi_time=VM_queue_state[t][s]->resi_time;
										VM_queue[t][s]->life_time=VM_queue_state[t][s]->life_time;
										VM_queue[t][s]->type=VM_queue_state[t][s]->type;
										for(int out=0; out<VM_queue_state[t][s]->assigned_tasks.size(); out++)
											for(int in=0; in<VM_queue_state[t][s]->assigned_tasks[out].size(); in++)
												VM_queue[t][s]->assigned_tasks[out][in]=VM_queue_state[t][s]->assigned_tasks[out][in];		
									}
							}
						}
						//test for merge
						bool merge = false;
						for(int k=0; k<VM_queue[j].size(); k++){
							if(VM_queue[j][k]->assigned_tasks.size()==1) {
								for(int out=0; out<VM_queue[j][k]->assigned_tasks.size(); out++){
									int taskend = VM_queue[j][k]->assigned_tasks[out].size() - 1;
									sort(VM_queue[j][k]->assigned_tasks[out].begin(),VM_queue[j][k]->assigned_tasks[out].end(),taskfunction);
									//merge with the one after it or in front of it									
									double t1 = VM_queue[j][k]->assigned_tasks[out][taskend]->end_time - VM_queue[j][k]->assigned_tasks[out][0]->start_time;
									double t2 = VM_queue[j][k]->life_time - VM_queue[j][k]->resi_time;
									double t3;
									if(VM_queue[j][k]->start_time >= VM_queue[j][k]->assigned_tasks[out][taskend]->end_time)
										t3 = VM_queue[j][k]->end_time - VM_queue[j][k]->assigned_tasks[out][0]->start_time;
									else if(VM_queue[j][k]->end_time <=VM_queue[j][k]->assigned_tasks[out][0]->start_time)
										t3 = VM_queue[j][k]->assigned_tasks[out][taskend]->end_time - VM_queue[j][k]->start_time;
									else{ continue;}
									//move VM k to the end/head of the out-th queue of VM i
									costdemote += move(j,t1,t2,t3);
									if(costdemote>1e-12){
										demote_operation(jobs,VM_queue[i][iter],j);																													
										//then do move
										move_operation2(jobs,VM_queue[j][k],VM_queue[i][iter],out);
										
										bool update = updateVMqueue(VM_queue);
										if(!update){
											for(int t=0; t<types; t++)
												for(int s=0; s<VM_queue_state[t].size(); s++)
												{
													VM_queue[t][s]->capacity=VM_queue_state[t][s]->capacity;
													VM_queue[t][s]->start_time=VM_queue_state[t][s]->start_time;
													VM_queue[t][s]->end_time=VM_queue_state[t][s]->end_time;
													VM_queue[t][s]->resi_time=VM_queue_state[t][s]->resi_time;
													VM_queue[t][s]->life_time=VM_queue_state[t][s]->life_time;
													VM_queue[t][s]->type=VM_queue_state[t][s]->type;
													for(int out=0; out<VM_queue_state[t][s]->assigned_tasks.size(); out++)
														for(int in=0; in<VM_queue_state[t][s]->assigned_tasks[out].size(); in++)
															VM_queue[t][s]->assigned_tasks[out][in]=VM_queue_state[t][s]->assigned_tasks[out][in];
												}
											continue;//continue to the next out queue
										}
										VM_queue[j].push_back(VM_queue[i][iter]);
										VM_queue[i].erase(VM_queue[i].begin()+iter);
										delete VM_queue[j][k];
										VM_queue[j][k] = NULL;
										VM_queue[j].erase(VM_queue[j].begin()+k);
										
										for(int t=0; t<types; t++){
											for(int s=0; s<VM_queue_backup[t].size(); s++){	
												for(int out=0; out<VM_queue_backup[t][s]->assigned_tasks.size();out++)
													for(int in=0; in<VM_queue_backup[t][s]->assigned_tasks[out].size(); in++)
													{
														delete VM_queue_backup[t][s]->assigned_tasks[out][in];
														VM_queue_backup[t][s]->assigned_tasks[out][in] = NULL;
													}
												delete VM_queue_backup[t][s];
												VM_queue_backup[t][s] = NULL;
											}
											for(int s=0; s<VM_queue_state[t].size(); s++){
												for(int out=0; out<VM_queue_state[t][s]->assigned_tasks.size();out++)
													for(int in=0; in<VM_queue_state[t][s]->assigned_tasks[out].size(); in++)
													{
														delete VM_queue_state[t][s]->assigned_tasks[out][in];
														VM_queue_state[t][s]->assigned_tasks[out][in] = NULL;
													}
												delete VM_queue_state[t][s];
												VM_queue_state[t][s] = NULL;									
											}	
										}								
										printf("demote operation and move operation 2\n");
										return costdemote;										
									}else{
										for(int t=0; t<types; t++)
											for(int s=0; s<VM_queue_backup[t].size(); s++)	{
												VM_queue[t][s]->capacity=VM_queue_state[t][s]->capacity;
												VM_queue[t][s]->start_time=VM_queue_state[t][s]->start_time;
												VM_queue[t][s]->end_time=VM_queue_state[t][s]->end_time;
												VM_queue[t][s]->resi_time=VM_queue_state[t][s]->resi_time;
												VM_queue[t][s]->life_time=VM_queue_state[t][s]->life_time;
												VM_queue[t][s]->type=VM_queue_state[t][s]->type;
												for(int out=0; out<VM_queue_state[t][s]->assigned_tasks.size(); out++)
													for(int in=0; in<VM_queue_state[t][s]->assigned_tasks[out].size(); in++)
														VM_queue[t][s]->assigned_tasks[out][in]=VM_queue_state[t][s]->assigned_tasks[out][in];
											}							
										//break;
									}
								}//next out queue
							}
						}//next task of type j
						for(int t=0; t<types; t++){
							for(int s=0; s<VM_queue_backup[t].size(); s++)
							{	
								VM_queue[t][s]->capacity=VM_queue_backup[t][s]->capacity;
								VM_queue[t][s]->start_time=VM_queue_backup[t][s]->start_time;
								VM_queue[t][s]->end_time=VM_queue_backup[t][s]->end_time;
								VM_queue[t][s]->resi_time=VM_queue_backup[t][s]->resi_time;
								VM_queue[t][s]->life_time=VM_queue_backup[t][s]->life_time;
								VM_queue[t][s]->type=VM_queue_backup[t][s]->type;
								for(int out=0; out<VM_queue_backup[t][s]->assigned_tasks.size(); out++)
									for(int in=0; in<VM_queue_backup[t][s]->assigned_tasks[out].size(); in++)
										VM_queue[t][s]->assigned_tasks[out][in]=VM_queue_backup[t][s]->assigned_tasks[out][in];
							}
							for(int s=0; s<VM_queue_state[t].size(); s++){
								for(int out=0; out<VM_queue_state[t][s]->assigned_tasks.size();out++)
									for(int in=0; in<VM_queue_state[t][s]->assigned_tasks[out].size(); in++)
									{
										delete VM_queue_state[t][s]->assigned_tasks[out][in];
										VM_queue_state[t][s]->assigned_tasks[out][in] = NULL;
									}
								delete VM_queue_state[t][s];
								VM_queue_state[t][s] = NULL;
							}
						}
					}else{//deadline is not ok, go to j+1
						for(int t=0; t<types; t++)
							for(int s=0; s<VM_queue_backup[t].size(); s++){
								VM_queue[t][s]->capacity=VM_queue_backup[t][s]->capacity;
								VM_queue[t][s]->start_time=VM_queue_backup[t][s]->start_time;
								VM_queue[t][s]->end_time=VM_queue_backup[t][s]->end_time;
								VM_queue[t][s]->resi_time=VM_queue_backup[t][s]->resi_time;
								VM_queue[t][s]->life_time=VM_queue_backup[t][s]->life_time;
								VM_queue[t][s]->type=VM_queue_backup[t][s]->type;
								for(int out=0; out<VM_queue_backup[t][s]->assigned_tasks.size(); out++)
									for(int in=0; in<VM_queue_backup[t][s]->assigned_tasks[out].size(); in++)
										VM_queue[t][s]->assigned_tasks[out][in]=VM_queue_backup[t][s]->assigned_tasks[out][in];
							}
					}
				}//capacity ok
			}//next j type
		}
	}
	return 0;
}