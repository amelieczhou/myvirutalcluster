#include "stdafx.h"
#include "ConsolidateOperators.h"

double opMove(vector<VM*>*  VM_queue, vector<Job*> jobs)
{
	double residualtime = 0; 
	bool domove = false;
	double costmove = 0;
	//save context for site recovery to the very beginning
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
	////////////////////////////////////same type move//////////////////////////////////////////////////
	for(int i=0; i<types; i++){
		for(int j=0; j<VM_queue[i].size(); j++) {
			bool condition = false;
			int outiter;
			double endtime;
			double starttime;
			double leftovertime;
			//find the queue with the largest leftover time, index outiter
			for(int out=0; out<VM_queue[i][j]->assigned_tasks.size(); out++){
				int insize = VM_queue[i][j]->assigned_tasks[out].size();
				leftovertime = VM_queue[i][j]->start_time + VM_queue[i][j]->life_time - VM_queue[i][j]->assigned_tasks[out][insize-1]->end_time;
				if(leftovertime > residualtime)	{
					condition = true;
					outiter = out;
					endtime = VM_queue[i][j]->assigned_tasks[out][insize-1]->end_time;
					starttime = VM_queue[i][j]->assigned_tasks[out][0]->start_time;
					residualtime = leftovertime;
					//break;
				}
			}
			//move with the out-th task queue of the ij vm
			if(condition) { //out-th queue residual time longer than threshold
				for(int k=0; k<VM_queue[i].size(); k++){
					if(k!=j && VM_queue[i][k]->assigned_tasks.size()==1){//only merge with a VM without coscheduling tasks
						// task k starts before the end of task j.out, then move k to the end of task j and merge them
						// ik:        |----------         |
						// ij: --------------
						if(VM_queue[i][k]->start_time <= endtime){// && VM_queue[i][k]->start_time >= starttime) {//can merge
							double moveafter = endtime - VM_queue[i][k]->start_time;
							bool deadlineok = true;
							for(int in = 0; in<VM_queue[i][k]->assigned_tasks[0].size(); in++){
								double exetime = VM_queue[i][k]->assigned_tasks[0][in]->end_time - VM_queue[i][k]->assigned_tasks[0][in]->start_time;
								double oldestTime = VM_queue[i][k]->assigned_tasks[0][in]->estTime[VM_queue[i][k]->assigned_tasks[0][in]->assigned_type];
								VM_queue[i][k]->assigned_tasks[0][in]->start_time += moveafter;
								VM_queue[i][k]->assigned_tasks[0][in]->estTime[VM_queue[i][k]->assigned_tasks[0][in]->assigned_type] = exetime;
								deadlineok = time_flood(jobs[VM_queue[i][k]->assigned_tasks[0][in]->job_id],VM_queue[i][k]->assigned_tasks[0][in]->name);//if true, do the move; else timeflood again	
								VM_queue[i][k]->assigned_tasks[0][in]->estTime[VM_queue[i][k]->assigned_tasks[0][in]->assigned_type] = oldestTime;
								if(!deadlineok) break;
														
							}
							if(deadlineok) {//deadline is long enough to move k afterwards
								double t1 = endtime - starttime;
								double t2 = VM_queue[i][k]->life_time - VM_queue[i][k]->resi_time;
								double t3 = VM_queue[i][k]->end_time + moveafter - starttime;
								costmove = move(i,t1,t2, t3);
								if(costmove > 0) {		
									domove = true;
									move_operation1(jobs,VM_queue[i][k],VM_queue[i][j],outiter,moveafter);	
									//VM_queue[i].erase(VM_queue[i].begin()+k);
									bool update = updateVMqueue(VM_queue);//false means do not do the move
									if(!update){
										//site recovery 
										for(int t=0; t<types; t++){
											//VM_queue[t].resize(VM_queue_backup[t].size());
											for(int s=0; s<VM_queue_backup[t].size(); s++){
												*VM_queue[t][s] = *VM_queue_backup[t][s];												
											}
										}
										
										return 0;
									}
									delete VM_queue[i][k];
									VM_queue[i][k]=NULL;
									VM_queue[i].erase(VM_queue[i].begin()+k);
									printf("move operation 1\n");
									return costmove;
								}
								//break;//or k--;
							}
							else if(VM_queue[i][k]->start_time < starttime )//move vm j afterward to the end of vm k
							{//if at least one task in VM k cannot cannot finish before deadline, need to split VM j
								bool deadlineok = true;
								double jmoveafter = VM_queue[i][k]->end_time - starttime;
								if(jmoveafter < 0) jmoveafter = 0;
								for(int in=0; in<VM_queue[i][j]->assigned_tasks[outiter].size();in++){
									double exetime = VM_queue[i][j]->assigned_tasks[outiter][in]->end_time - VM_queue[i][j]->assigned_tasks[outiter][in]->start_time;
									double oldestTime = VM_queue[i][j]->assigned_tasks[outiter][in]->estTime[VM_queue[i][j]->assigned_tasks[outiter][in]->assigned_type];
									VM_queue[i][j]->assigned_tasks[outiter][in]->start_time += jmoveafter;
									VM_queue[i][j]->assigned_tasks[outiter][in]->estTime[VM_queue[i][j]->assigned_tasks[outiter][in]->assigned_type] = exetime;
									deadlineok = time_flood(jobs[VM_queue[i][j]->assigned_tasks[outiter][in]->job_id],VM_queue[i][j]->assigned_tasks[outiter][in]->name);//if true, do the move; else timeflood again	
									VM_queue[i][j]->assigned_tasks[outiter][in]->estTime[VM_queue[i][j]->assigned_tasks[outiter][in]->assigned_type] = oldestTime;
									if(!deadlineok) break;									
								}
								
								if(deadlineok){ //move vm j to the end of vm k do not violdate deadline							
									double t1 = endtime - starttime;
									double t2 = VM_queue[i][k]->life_time - VM_queue[i][k]->resi_time;
									double t3 = endtime + jmoveafter - VM_queue[i][k]->start_time;
									costmove = move(i,t1,t2,t3);
									if(costmove > 0) {
										domove = true;
										move_operation3(jobs,VM_queue[i][j],outiter,VM_queue[i][k],jmoveafter);	
										//VM_queue[i].erase(VM_queue[i].begin()+k);
										double update = updateVMqueue(VM_queue);
										if(!update){
											for(int t=0; t<types; t++)
												for(int s=0; s<VM_queue[t].size(); s++){
													*VM_queue[t][s] = *VM_queue_backup[t][s];												
												}
											return 0;
										}
										delete VM_queue[i][k];
										VM_queue[i][k] = NULL;
										VM_queue[i].erase(VM_queue[i].begin()+k);
										printf("move operation 3\n");
										return costmove;
									}
									//break; //or k--;
								}
							}								
						}
						else if(VM_queue[i][k]->start_time >= endtime ){//&& (60.0 - VM_queue[i][k]->resi_time)<= VM_queue[i][j]->start_time +VM_queue[i][j]->life_time - VM_queue[i][k]->start_time
							//VM k starts before the lifetime of j, but after the end of j
							//ik:              |-------
							//ij: |-----------
							double t1 = endtime - starttime;
							double t2 = VM_queue[i][k]->life_time - VM_queue[i][k]->resi_time;
							double t3 = VM_queue[i][k]->end_time - starttime; 
							costmove = move(i,t1,t2,t3);
							//do not need to update any task
							if(costmove > 0) {
								domove = true;
								//save context before move operation
								VM* vmik = new VM();
								*vmik = *VM_queue[i][k];
								VM* vmij = new VM();
								*vmij = *VM_queue[i][j];

								move_operation2(jobs, VM_queue[i][k], VM_queue[i][j],outiter);
								VM_queue[i].erase(VM_queue[i].begin()+k);
								bool update = updateVMqueue(VM_queue);
								if(!update){
									*VM_queue[i][j] = *vmij;
									VM_queue[i].push_back(vmik);

									return 0;
								}
								printf("move operation 2\n");
								return costmove;
							}
							//break; //or k--
						}
					}					
				}
			}	
		}
	}

	////////////////////////////////////////////different type move///////////////////////////////////////////////
	bool dopromote = false, dodemote = false;
	for(int i=0; i<types; i++){
		for(int ik=0; ik<VM_queue[i].size(); ik++){
			for(int j=0; j<types; j++) {
				if(j>i) dopromote = true;
				if(j<i) dodemote = true;
				if(j==i) break;
				for(int jk=0; jk<VM_queue[j].size(); jk++){
					if(VM_queue[j][jk]->assigned_tasks.size() == 1&&VM_queue[i][ik]->assigned_tasks.size() == 1){
						double moveafter = 0;
						double deadlineok = true;						
						double originaltime = VM_queue[j][jk]->end_time - VM_queue[j][jk]->start_time;						
						//jk demote or promote to move with ik	
						//first check demote violate deadline or not			
						for(int in=0; in<VM_queue[j][jk]->assigned_tasks[0].size(); in++){	
							VM_queue[j][jk]->assigned_tasks[0][in]->assigned_type = i;
							VM_queue[j][jk]->start_time += moveafter;	
							deadlineok = time_flood(jobs[VM_queue[j][jk]->assigned_tasks[0][in]->job_id],VM_queue[j][jk]->assigned_tasks[0][in]->name);
							if(!deadlineok) break;

							if(in<VM_queue[j][jk]->assigned_tasks[0].size()-1){
								double blank = VM_queue[j][jk]->assigned_tasks[0][in+1]->start_time - VM_queue[j][jk]->assigned_tasks[0][in]->end_time;
								moveafter += VM_queue[j][jk]->assigned_tasks[0][in]->estTime[i]	- VM_queue[j][jk]->assigned_tasks[0][in]->estTime[j] - blank;
								if(moveafter < 0) moveafter = 0;
							}
						}										
											
						if(deadlineok){
							bool movedeadline = true;
							//then check move violate deadline or not
							if(VM_queue[j][jk]->start_time>=VM_queue[i][ik]->start_time){								
								//check move deadline
								//move jk to the end of ik
								double moveafter = VM_queue[i][ik]->end_time - VM_queue[j][jk]->start_time;
								if(moveafter < 0) moveafter = 0;
								//check deadline
								for(int in=0; in<VM_queue[j][jk]->assigned_tasks[0].size(); in++){											
									VM_queue[j][jk]->assigned_tasks[0][in]->start_time += moveafter;
									movedeadline = time_flood(jobs[VM_queue[j][jk]->assigned_tasks[0][in]->job_id],VM_queue[j][jk]->assigned_tasks[0][in]->name);
									if(!movedeadline) break;
								}
								if(movedeadline){
									//do the demote
									double newtime = VM_queue[j][jk]->end_time - VM_queue[j][jk]->start_time;
									double costdemote = 0;
									if(dodemote) {
										costdemote = demote(j,originaltime,i,newtime);
										demote_operation(jobs,VM_queue[j][jk],i);
									}
									if(dopromote) {
										costdemote = ppromote(j,originaltime,i,newtime);
										promote_operation(jobs,VM_queue[j][jk],i);
									}
									
									//do the move, move jk to the end of ik
									double t1 = VM_queue[i][ik]->end_time - VM_queue[i][ik]->start_time;
									double t2 = VM_queue[j][jk]->end_time - VM_queue[j][jk]->start_time;
									double t3 = VM_queue[j][jk]->end_time + moveafter - VM_queue[i][ik]->start_time;
									costmove = move(i, t1,t2,t3);
									costmove += costdemote;
									if(costmove>0){
										move_operation1(jobs,VM_queue[j][jk],VM_queue[i][ik],0,moveafter);
										VM_queue[j].erase(VM_queue[j].begin()+jk);
										bool update = updateVMqueue(VM_queue);
										if(!update){
											for(int t=0; t<types; t++)
												for(int s=0; s<VM_queue[t].size(); s++){
													*VM_queue[t][s] = *VM_queue_backup[t][s];												
												}

											return 0;
										}
										printf("demote/promote operation and move operation 1\n");
										return costmove;
									}else {										
										for(int t=0; t<types; t++)
											for(int s=0; s<VM_queue[t].size(); s++){
												*VM_queue[t][s] = *VM_queue_backup[t][s];												
											}

										return 0;
									}									
								}
								else{	//deadline not allowd, replay the context
									for(int t=0; t<types; t++)
										for(int s=0; s<VM_queue[t].size(); s++){
											*VM_queue[t][s] = *VM_queue_backup[t][s];												
										}

									return 0;
								}
							}
							else if(VM_queue[j][jk]->start_time < VM_queue[i][ik]->start_time){
								//move jk in front of ik
								//check feadline for ik
								double moveafter = VM_queue[j][jk]->end_time - VM_queue[i][ik]->start_time;
								if(moveafter < 0) moveafter = 0;
								//check deadline
								for(int in=0; in<VM_queue[i][ik]->assigned_tasks[0].size(); in++){											
									VM_queue[i][ik]->assigned_tasks[0][in]->start_time += moveafter;
									movedeadline = time_flood(jobs[VM_queue[i][ik]->assigned_tasks[0][in]->job_id],VM_queue[i][ik]->assigned_tasks[0][in]->name);
									if(!movedeadline) break;
								}
								if(movedeadline){
									//do the demote or promote
									//do the demote
									double newtime = VM_queue[j][jk]->end_time - VM_queue[j][jk]->start_time;
									double costdemote = 0;
									if(dodemote) {
										costdemote = demote(j,originaltime,i,newtime);
										demote_operation(jobs,VM_queue[j][jk],i);
									}
									if(dopromote) {
										costdemote = ppromote(j,originaltime,i,newtime);
										promote_operation(jobs,VM_queue[j][jk],i);
									}
									
									//do the move, move ik to the end of jk
									double t1 = VM_queue[i][ik]->end_time - VM_queue[i][ik]->start_time;
									double t2 = VM_queue[j][jk]->end_time - VM_queue[j][jk]->start_time;
									double t3 = VM_queue[j][jk]->end_time + moveafter - VM_queue[i][ik]->start_time;
									costmove = move(i, t1,t2,t3);
									costmove += costdemote;
									if(costmove > 0){
										move_operation3(jobs,VM_queue[i][ik],0,VM_queue[j][jk],moveafter);
										VM_queue[j].erase(VM_queue[j].begin()+jk);
										bool update = updateVMqueue(VM_queue);
										if(!update){
											for(int t=0; t<types; t++)
												for(int s=0; s<VM_queue[t].size(); s++){
													*VM_queue[t][s] = *VM_queue_backup[t][s];												
												}

											return 0;
										}
										printf("demote/promote operation and move operation 3\n");
										return costmove;
									}else{
										/*siteRecovery(jobs, VM_queue[j][jk]);
										updateVMqueue(VM_queue);*/
										for(int t=0; t<types; t++)
											for(int s=0; s<VM_queue[t].size(); s++){
												*VM_queue[t][s] = *VM_queue_backup[t][s];												
											}

										return 0;
									}									
								}
								else{//deadline not allowd, replay the context
									/*siteRecovery(jobs, VM_queue[j][jk]);
									updateVMqueue(VM_queue);*/
									for(int t=0; t<types; t++)
										for(int s=0; s<VM_queue[t].size(); s++){
											*VM_queue[t][s] = *VM_queue_backup[t][s];												
										}

									return 0;
								}
							}
						}
						else{
							//deadline not allowd, replay the context
							/*siteRecovery(jobs, VM_queue[j][jk]);
							updateVMqueue(VM_queue);*/
							for(int t=0; t<types; t++)
								for(int s=0; s<VM_queue[t].size(); s++){
									*VM_queue[t][s] = *VM_queue_backup[t][s];												
								}

							return 0;
						}						
					}														
				}			
			}
		}
	}
	return 0;
}