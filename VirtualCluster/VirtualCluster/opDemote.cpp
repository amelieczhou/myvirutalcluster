#include "stdafx.h"
#include "ConsolidateOperators.h"

double opDemote(vector<VM*>* VM_queue, vector<Job*> jobs)
{
	bool dodemote = false;
	double costdemote = 0;

	//save context for site recovery to the very beginning
	vector<vector<VM*> > VM_queue_backup;
	/*for(int i=0; i<types; i++){
		vector<VM*> vms;
		for(int j=0; j<VM_queue[i].size(); j++){
			VM vm = *VM_queue[i][j];			
			vms.push_back(&vm);			
		}
		VM_queue_backup.push_back(vms);
	}*/
	for(int i=0; i<types; i++){
		vector<VM*> vms;		
		for(int j=0; j<VM_queue[i].size(); j++){
			VM* vm = new VM();
			*vm = *VM_queue[i][j];	
			//VM_queue[i][j]->start_time = 100000; //do not change along
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
	//VM_queue[3][0]->assigned_tasks[0][0]->start_time = 1000000; //do not change along
	double initialcost = 0;
	for(int i=0; i<types; i++)
		for(int j=0; j<VM_queue[i].size(); j++)
			initialcost += priceOnDemand[VM_queue[i][j]->type]*VM_queue[i][j]->life_time/60.0;

	for(int i=0; i<types; i++){
		for(int j=0; j<i; j++){
			for(int iter=0; iter<VM_queue[i].size(); iter++){
				//demote VM_queue[i][iter] to type j
				//when demote, consider the merge cost		
				if(pow(2.0,i) - std::pow(2.0,j) <= VM_queue[i][iter]->capacity){//leftover capacity can support demotion
					costdemote = 0;
					bool deadlineok = true;
					double originaltime = VM_queue[i][iter]->end_time - VM_queue[i][iter]->start_time;

					for(int out=0; out<VM_queue[i][iter]->assigned_tasks.size(); out++){
						double moveafter = 0;
						for(int in=0; in<VM_queue[i][iter]->assigned_tasks[out].size(); in++){
							double exetime = VM_queue[i][iter]->assigned_tasks[out][in]->end_time - VM_queue[i][iter]->assigned_tasks[out][in]->start_time;
							VM_queue[i][iter]->assigned_tasks[out][in]->assigned_type = j;
							VM_queue[i][iter]->assigned_tasks[out][in]->start_time += moveafter;
							deadlineok = time_flood(jobs[VM_queue[i][iter]->assigned_tasks[out][in]->job_id],VM_queue[i][iter]->assigned_tasks[out][in]->name);
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
						bool update = updateVMqueue(VM_queue);//update the start and endtime of each VM
						if(!update){
							//demote, cost is higher
							for(int t=0; t<types; t++){
								for(int s=0; s<VM_queue_backup[t].size(); s++)	{	
									deepcopy(VM_queue[t][s],VM_queue_backup[t][s]);
								}
							}
							continue;//contiue to the next task in type i
						}
						//save context at the status when the assigned type of tasks are changed
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
						//demote vm i to type j
						int name= VM_queue[i][iter]->assigned_tasks[0][0]->name;//for debug only
						if(VM_queue[i][iter]->assigned_tasks.size()>1 ||VM_queue[i][iter]->assigned_tasks[0].size()>1)//for debug only
							printf("");
						double newtime = VM_queue[i][iter]->end_time - VM_queue[i][iter]->start_time;
						costdemote = demote(i, originaltime, j, newtime);
						if(costdemote>1e-12){
							double cost1=0;
							for(int ttype=0; ttype<types; ttype++)
								for(int tsize=0; tsize<VM_queue[ttype].size(); tsize++)
									cost1 += priceOnDemand[VM_queue[ttype][tsize]->type]*VM_queue[ttype][tsize]->life_time /60.0;
							demote_operation(jobs,VM_queue[i][iter],j);
							bool update = updateVMqueue(VM_queue);
							double cost2=0;
							for(int ttype=0; ttype<types; ttype++)
								for(int tsize=0; tsize<VM_queue[ttype].size(); tsize++)
									cost2 += priceOnDemand[VM_queue[ttype][tsize]->type]*VM_queue[ttype][tsize]->life_time /60.0;
							/*double finalcost = 0;
							for(int iiter=0; iiter<types; iiter++)
								for(int jiter=0; jiter<VM_queue[iiter].size(); jiter++)
									finalcost += priceOnDemand[VM_queue[iiter][jiter]->type]*VM_queue[iiter][jiter]->life_time/60.0;
							if(finalcost>=initialcost)*/
							if(cost1<cost2)
								for(int t=0; t<types; t++)
									for(int s=0; s<VM_queue_backup[t].size(); s++)	
										deepcopy(VM_queue[t][s],VM_queue_state[t][s]);
							else{ //
								//if(update){//new cost less than old cost
									VM_queue[j].push_back(VM_queue[i][iter]);
									VM_queue[i].erase(VM_queue[i].begin()+iter);

									deepdelete(VM_queue_backup);
									deepdelete(VM_queue_state);
									printf("demote operation\n");
									return costdemote;
								}
							}else{
								for(int t=0; t<types; t++)
									for(int s=0; s<VM_queue_state[t].size(); s++){										
										deepcopy(VM_queue[t][s], VM_queue_state[t][s]);//dest,src
									}
							}
						//test for merge
						bool merge = false;
						for(int k=0; k<VM_queue[j].size(); k++){
							if(VM_queue[j][k]->assigned_tasks.size()==1) {
								for(int out=0; out<VM_queue[i][iter]->assigned_tasks.size(); out++){
									int taskend = VM_queue[i][iter]->assigned_tasks[out].size() - 1;
									sort(VM_queue[i][iter]->assigned_tasks[out].begin(),VM_queue[i][iter]->assigned_tasks[out].end(),taskfunction);
									//merge with the one after it or in front of it									
									double t1 = VM_queue[i][iter]->assigned_tasks[out][taskend]->end_time - VM_queue[i][iter]->assigned_tasks[out][0]->start_time;
									double t2 = VM_queue[j][k]->life_time - VM_queue[j][k]->resi_time;
									double t3;
									if(VM_queue[j][k]->start_time >= VM_queue[i][iter]->assigned_tasks[out][taskend]->end_time)
										t3 = VM_queue[j][k]->end_time - VM_queue[i][iter]->assigned_tasks[out][0]->start_time;
									else if(VM_queue[j][k]->end_time <=VM_queue[i][iter]->assigned_tasks[out][0]->start_time)
										t3 = VM_queue[i][iter]->assigned_tasks[out][taskend]->end_time - VM_queue[j][k]->start_time;
									else{ continue;}//continue to the next out queue
									//move VM k to the end/head of the out-th queue of VM i
									double costmove = move(j,t1,t2,t3);
									costdemote += costmove;
									if(costdemote>1e-12){
										double cost1=0;
										for(int ttype=0; ttype<types; ttype++)
											for(int tsize=0; tsize<VM_queue[ttype].size(); tsize++)
												cost1 += priceOnDemand[VM_queue[ttype][tsize]->type]*VM_queue[ttype][tsize]->life_time /60.0;

										demote_operation(jobs,VM_queue[i][iter],j);																													
										//then do move
										int numoftasks = VM_queue[j][k]->assigned_tasks[0].size();//how many tasks are merged to vm[i][iiter]
										move_operation2(jobs,VM_queue[j][k],VM_queue[i][iter],out);
										
										bool update = updateVMqueue(VM_queue);
										double cost2=0;
										for(int ttype=0; ttype<types; ttype++)
											for(int tsize=0; tsize<VM_queue[ttype].size(); tsize++)
												cost2 += priceOnDemand[VM_queue[ttype][tsize]->type]*VM_queue[ttype][tsize]->life_time /60.0;

										//if(!update){
										if(cost1<cost2){
											vector<taskVertex*> tasks;
											for(int taskiter=0; taskiter<numoftasks; taskiter++){
												taskVertex* task = VM_queue[i][iter]->assigned_tasks[out].back();
												VM_queue[i][iter]->assigned_tasks[out].pop_back();
												tasks.push_back(task);
											}
											VM_queue[j][k]->assigned_tasks.push_back(tasks);
											for(int t=0; t<types; t++)
												for(int s=0; s<VM_queue_state[t].size(); s++)	{
													deepcopy(VM_queue[t][s],VM_queue_state[t][s]);
												}
											continue;//continue to the next out queue
										}
										VM_queue[j].push_back(VM_queue[i][iter]);
										VM_queue[i].erase(VM_queue[i].begin()+iter);
										//delete VM_queue[j][k];
										//VM_queue[j][k] = NULL;
										VM_queue[j].erase(VM_queue[j].begin()+k);
																				
										/*double finalcost = 0;
										for(int iiter=0; iiter<types; iiter++)
											for(int jiter=0; jiter<VM_queue[iiter].size(); jiter++)
												finalcost += priceOnDemand[VM_queue[iiter][jiter]->type]*VM_queue[iiter][jiter]->life_time/60.0;
										if(finalcost>=initialcost){
											for(int t=0; t<types; t++){
												for(int s=0; s<VM_queue[t].size(); s++){
													delete VM_queue[t][s];
													VM_queue[t][s] = NULL;
												}
												VM_queue[t].clear();
												vector<VM*> vms;
												for(int s=0; s<VM_queue_state[t].size(); s++)	{
													VM* vm = new VM();
													deepcopy(vm,VM_queue_state[t][s]);
													vms.push_back(vm);
												}
												VM_queue[t] =vms;
											}
										}
										else{*/
											deepdelete(VM_queue_backup);
											deepdelete(VM_queue_state);
											printf("demote operation and move operation 2\n");
											return costdemote;		
										//}
									}else{
										for(int t=0; t<types; t++)
											for(int s=0; s<VM_queue_backup[t].size(); s++)	{
												deepcopy(VM_queue[t][s],VM_queue_state[t][s]);
											}							
										//break;
										costdemote -= costmove;
									}
								}//next out queue
							}
						}//next task of type j
						for(int t=0; t<types; t++){
							for(int s=0; s<VM_queue_backup[t].size(); s++)	{	
								deepcopy(VM_queue[t][s],VM_queue_backup[t][s]);
							}							
						}
						deepdelete(VM_queue_state);
					}else{//deadline is not ok, go to j+1
						for(int t=0; t<types; t++)
							for(int s=0; s<VM_queue_backup[t].size(); s++){
								deepcopy(VM_queue[t][s], VM_queue_backup[t][s]);
							}
					}
				}//capacity ok
			}//next task to promote to j type
		}//next type j
	}//next VM queue

	double finalcost = 0;
	for(int i=0; i<types; i++)
		for(int j=0; j<VM_queue[i].size(); j++)
			finalcost += priceOnDemand[VM_queue[i][j]->type]*VM_queue[i][j]->life_time/60.0;
	if(finalcost>=initialcost){
		for(int t=0; t<types; t++){
			for(int s=0; s<VM_queue_backup[t].size(); s++)	{	
				deepcopy(VM_queue[t][s],VM_queue_backup[t][s]);
			}
		}
		deepdelete(VM_queue_backup);
	}

	return 0;
}