#include "stdafx.h"
#include "ConsolidateOperators.h"

double opPromote(vector<VM*>* VM_queue, vector<Job*> jobs, bool checkcost, bool estimate, bool timeorcost)
{
	//Rule 1, promote can merge with left or right
	bool dopromote = false;
	double costpromote = 0;
	double timepromote = 0;
	bool testsingle = true; //test the effectiveness of single operation
	//save context for site recovery to the very beginning
	vector<vector<VM*> > VM_queue_backup;
	/*for(int i=0; i<types; i++){
		vector<VM*> vms;		
		for(int j=0; j<VM_queue[i].size(); j++){
			VM* vm = new VM();
			*vm = *VM_queue[i][j];
			vms.push_back(vm);
		}
		VM_queue_backup.push_back(vms);
	}*/
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
					if(task->end_time < task->start_time)
						printf("");
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
			initialcost += priceOnDemand[VM_queue[i][j]->type]*VM_queue[i][j]->life_time/60.0;

	pair<vertex_iter, vertex_iter> vp;
	//std::vector<VM*> VMs;
	//for(int i=0; i<types; i++) 
	//	for(int j=0; j<VM_queue[i].size(); j++) 
	//		VMs.push_back(VM_queue[i][j]);
	//std::sort(VMs.begin(),VMs.end(),vmfunction);

	for(int i=0; i<types; i++){
		for(int j=i+1; j<types; j++){//promote to type j
			for(int iter=0; iter<VM_queue[i].size(); iter++){
				costpromote = 0;
				double time1 = 0;
				for(int ttype=0; ttype<jobs.size(); ttype++){
					pair<vertex_iter, vertex_iter> vp;
					vp = vertices(jobs[ttype]->g);
					time1 += jobs[ttype]->g[*(vp.second-1)].end_time;
				}
				time1 /= jobs.size();

				//promote VM_queue[i][iter] to type j
				bool deadlineok = true;
				double t1 = VM_queue[i][iter]->end_time - VM_queue[i][iter]->start_time;
				//update tasks see if dealine violation
				for(int out=0; out<VM_queue[i][iter]->assigned_tasks.size(); out++){
					for(int in=0; in<VM_queue[i][iter]->assigned_tasks[out].size(); in++){
						double exetime = VM_queue[i][iter]->assigned_tasks[out][in]->end_time - VM_queue[i][iter]->assigned_tasks[out][in]->start_time;
						VM_queue[i][iter]->assigned_tasks[out][in]->assigned_type = j;
						deadlineok = time_flood(jobs[VM_queue[i][iter]->assigned_tasks[out][in]->job_id],VM_queue[i][iter]->assigned_tasks[out][in]->name);
						if(timeorcost)
							deadlineok=true;
						double newexetime = exetime*VM_queue[i][iter]->assigned_tasks[out][in]->estTime[j]/VM_queue[i][iter]->assigned_tasks[out][in]->estTime[i];
						//if(in>0)//for debuging only
						//	printf("");
						if(!deadlineok) {
							//printf("promote violate deadline!\n");
							break;
						}
					}
					if(!deadlineok) break;
				}
				if(!deadlineok) {
					//site recovery
					for(int t=0; t<types; t++)
						for(int s=0; s<VM_queue_backup[t].size(); s++)
							//*VM_queue[t][s] = *VM_queue_backup[t][s];	
							deepcopy(VM_queue[t][s], VM_queue_backup[t][s]);
					continue; //try promote the next task
				}
				dopromote = true;
				
				bool update = updateVMqueue(VM_queue);
				if(timeorcost){
					//budget violated
					bool checkbudget=true;
					double totalcost=0;
					for(int iindex=0; iindex<types; iindex++)
						for(int jindex=0; jindex <VM_queue[iindex].size(); jindex++)
							totalcost+= ((VM_queue[iindex][jindex]->end_time-VM_queue[iindex][jindex]->start_time)/60)*priceOnDemand[VM_queue[iindex][jindex]->type];
					double budgets=0;
					for(int jsize=0; jsize<jobs.size(); jsize++)
					budgets += jobs[jsize]->budget;
					if(totalcost > budgets) checkbudget = false;
					//update = checkbudget;
					//time get longer
					double time2 = 0;
					for(int ttype=0; ttype<jobs.size(); ttype++){
						pair<vertex_iter, vertex_iter> vp;
						vp = vertices(jobs[ttype]->g);
						time2 += jobs[ttype]->g[*(vp.second-1)].end_time;
					}
					time2 /= jobs.size();
					bool timeok = true;
					if(time2>time1) timeok = false;
					else if(time2==time1) timeok = update;
					update = timeok&&checkbudget;
					if(update)
						printf("");
				}
				if(!update){//although deadline is ok, promote is not cost efficient
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
								if(task->start_time > task->end_time)
									printf("");
							}
							vm->assigned_tasks.push_back(tasks);				
						}
						vms.push_back(vm);
					}
					VM_queue_state.push_back(vms);
				}
				//promote i to type j
				double t2 = VM_queue[i][iter]->end_time - VM_queue[i][iter]->start_time;
				costpromote = ppromote(i, t1, j, t2); //<=0
				timepromote = t1-t2;
				if(timeorcost){
					if(checkcost && timepromote>1e-12){
						for(int t=0; t<types; t++)
							for(int s=0; s<VM_queue_backup[t].size(); s++)
								deepcopy(VM_queue[t][s], VM_queue_backup[t][s]);
						deepdelete(VM_queue_backup);
						deepdelete(VM_queue_state);
						return timepromote;
					}else if(checkcost){
						for(int t=0; t<types; t++)
							for(int s=0; s<VM_queue_state[t].size(); s++)	{
								deepcopy(VM_queue[t][s],VM_queue_state[t][s]);
						}
						continue;//
					}
					if(!checkcost && estimate){
						bool condition1;
						condition1 = timepromote>1e-12;
						if(condition1){
							double cost1=0;
							for(int ttype=0; ttype<types; ttype++)
								for(int tsize=0; tsize<VM_queue[ttype].size(); tsize++)
									cost1 += priceOnDemand[VM_queue[ttype][tsize]->type]*VM_queue[ttype][tsize]->life_time /60.0;
							
							double budgets=0;
							for(int jsize=0; jsize<jobs.size(); jsize++)
								budgets += jobs[jsize]->budget;
							bool failcondition = cost1>budgets;
							if(failcondition){ //new cost no less than before																
								for(int t=0; t<types; t++)
									for(int s=0; s<VM_queue_state[t].size(); s++)												
										deepcopy(VM_queue[t][s], VM_queue_state[t][s]);
								continue;//break to the next out queue
							}else{
								VM_queue[j].push_back(VM_queue[i][iter]);
								VM_queue[i].erase(VM_queue[i].begin()+iter);

								deepdelete(VM_queue_backup);
								deepdelete(VM_queue_state);
								printf("promote operation and move operation 2\n");
								return timepromote;
							}
						}else{
							for(int t=0; t<types; t++)
								for(int s=0; s<VM_queue_state[t].size(); s++)	{
									deepcopy(VM_queue[t][s],VM_queue_state[t][s]);
								}
						}
					}
				}
				if(!timeorcost){
				//check merge
				for(int k=0; k<VM_queue[j].size(); k++){
					if(VM_queue[j][k]->assigned_tasks.size() == 1){
						for(int out=0; out<VM_queue[i][iter]->assigned_tasks.size(); out++){
							//int taskend = VM_queue[i][iter]->assigned_tasks[out].size()-1;
							//sort(VM_queue[i][iter]->assigned_tasks[out].begin(),VM_queue[i][iter]->assigned_tasks[out].end(),taskfunction);
							//VMs[i] merge with the one after it or in front of it	
							double outendtime = 0;
							double outstarttime = VM_queue[i][iter]->assigned_tasks[out][0]->start_time;
							for(int in=0; in<VM_queue[i][iter]->assigned_tasks[out].size(); in++){
								if(VM_queue[i][iter]->assigned_tasks[out][in]->end_time > outendtime)
									outendtime = VM_queue[i][iter]->assigned_tasks[out][in]->end_time;
								if(VM_queue[i][iter]->assigned_tasks[out][in]->start_time < outstarttime)
									outstarttime = VM_queue[i][iter]->assigned_tasks[out][in]->start_time;// = outstarttime;
							}
							double t1= VM_queue[j][k]->end_time - VM_queue[j][k]->start_time;
							double t2 = outendtime - outstarttime; //VM_queue[i][iter]->assigned_tasks[out][taskend]->end_time - VM_queue[i][iter]->assigned_tasks[out][0]->start_time;
							double t3;
							if(VM_queue[j][k]->start_time >= outendtime)//VM_queue[i][iter]->assigned_tasks[out][taskend]->end_time)
								t3 = VM_queue[j][k]->end_time -outstarttime;//VM_queue[i][iter]->assigned_tasks[out][0]->start_time;
							else if(VM_queue[j][k]->end_time <=outstarttime)//VM_queue[i][iter]->assigned_tasks[out][0]->start_time)
								t3 = outendtime - VM_queue[j][k]->start_time;//VM_queue[i][iter]->assigned_tasks[out][taskend]->end_time - VM_queue[j][k]->start_time;
							else continue;//to the next VM queue
							double costmove = move(j,t1,t2,t3);
							costpromote += costmove;								
							
							if(checkcost && estimate && costpromote>1e-12&&!timeorcost){
								for(int t=0; t<types; t++)
									for(int s=0; s<VM_queue_backup[t].size(); s++)
										deepcopy(VM_queue[t][s], VM_queue_backup[t][s]);
								deepdelete(VM_queue_backup);
								deepdelete(VM_queue_state);
								return costpromote;
							}else if(checkcost && estimate &&!timeorcost){
								for(int t=0; t<types; t++)
									for(int s=0; s<VM_queue_state[t].size(); s++)	{
										deepcopy(VM_queue[t][s],VM_queue_state[t][s]);
									}
								costpromote -= costmove;
							}else if(!checkcost && estimate){
								bool condition1;
								if(timeorcost) condition1 = timepromote>1e-12;
								else if(!timeorcost) condition1 = costpromote > 1e-12;
								if(condition1){
									//double cost1=0;
									//for(int ttype=0; ttype<types; ttype++)
									//	for(int tsize=0; tsize<VM_queue[ttype].size(); tsize++)
									//		cost1 += priceOnDemand[VM_queue[ttype][tsize]->type]*VM_queue[ttype][tsize]->life_time /60.0;
									double time1 = 0;
									for(int ttype=0; ttype<jobs.size(); ttype++){
										pair<vertex_iter, vertex_iter> vp;
										vp = vertices(jobs[ttype]->g);
										time1 += jobs[ttype]->g[*(vp.second-1)].end_time;
									}
									time1 /= jobs.size();
									//first do the promotion
									promote_operation(jobs,VM_queue[i][iter],j);
									//then do merge
									int numoftasks = VM_queue[j][k]->assigned_tasks[0].size();//how many tasks are merged to vm[i][iiter]
									move_operation2(jobs,VM_queue[j][k],VM_queue[i][iter],out);
									bool update = updateVMqueue(VM_queue);
									double cost2=0;
									for(int ttype=0; ttype<types; ttype++)
										for(int tsize=0; tsize<VM_queue[ttype].size(); tsize++)
											cost2 += priceOnDemand[VM_queue[ttype][tsize]->type]*VM_queue[ttype][tsize]->life_time /60.0;
									double time2 = 0;
									for(int ttype=0; ttype<jobs.size(); ttype++){
										pair<vertex_iter, vertex_iter> vp;
										vp = vertices(jobs[ttype]->g);
										time2 += jobs[ttype]->g[*(vp.second-1)].end_time;
									}
									time2 /= jobs.size();
									bool failcondition = false;		
									if(!timeorcost) failcondition=(cost2>=initialcost);
									else {
										if(time2>time1) failcondition=true;
										else if(time2==time1 && cost2>=initialcost) failcondition=true;
									}
									
									if(failcondition){ //new cost no less than before
										//promote only changes values, move_operation changes vector size
										vector<taskVertex*> tasks;
										for(int taskiter=0; taskiter<numoftasks; taskiter++){
											taskVertex* task = VM_queue[i][iter]->assigned_tasks[out].back();
											VM_queue[i][iter]->assigned_tasks[out].pop_back();
											tasks.push_back(task);										
										}
										VM_queue[j][k]->assigned_tasks.push_back(tasks);										
										for(int t=0; t<types; t++)
											for(int s=0; s<VM_queue_state[t].size(); s++)												
												deepcopy(VM_queue[t][s], VM_queue_state[t][s]);
										continue;//break to the next out queue
									}else{
										VM_queue[j].push_back(VM_queue[i][iter]);
										VM_queue[i].erase(VM_queue[i].begin()+iter);
										VM_queue[j].erase(VM_queue[j].begin()+k);

										deepdelete(VM_queue_backup);
										deepdelete(VM_queue_state);
										printf("promote operation and move operation 2\n");
										if(!timeorcost)
											return costpromote;
										else
											return timepromote;
									}
								}else{
									for(int t=0; t<types; t++)
										for(int s=0; s<VM_queue_state[t].size(); s++)	{
											deepcopy(VM_queue[t][s],VM_queue_state[t][s]);
										}
									if(!timeorcost)
										costpromote -= costmove;
								}
							}else if(!estimate){
								if(costpromote > 1e-12){
									//double cost1=0;
									//for(int ttype=0; ttype<types; ttype++)
									//	for(int tsize=0; tsize<VM_queue[ttype].size(); tsize++)
									//		cost1 += priceOnDemand[VM_queue[ttype][tsize]->type]*VM_queue[ttype][tsize]->life_time /60.0;
									double time1 = 0;
									for(int ttype=0; ttype<jobs.size(); ttype++){
										pair<vertex_iter, vertex_iter> vp;
										vp = vertices(jobs[ttype]->g);
										time1 += jobs[ttype]->g[*(vp.second-1)].end_time;
									}
									time1 /= jobs.size();
									//first do the promotion
									promote_operation(jobs,VM_queue[i][iter],j);
									//then do merge
									int numoftasks = VM_queue[j][k]->assigned_tasks[0].size();//how many tasks are merged to vm[i][iiter]
									move_operation2(jobs,VM_queue[j][k],VM_queue[i][iter],out);
									bool update = updateVMqueue(VM_queue);
									double cost2=0;
									for(int ttype=0; ttype<types; ttype++)
										for(int tsize=0; tsize<VM_queue[ttype].size(); tsize++)
											cost2 += priceOnDemand[VM_queue[ttype][tsize]->type]*VM_queue[ttype][tsize]->life_time /60.0;
									double time2 = 0;
									for(int ttype=0; ttype<jobs.size(); ttype++){
										pair<vertex_iter, vertex_iter> vp;
										vp = vertices(jobs[ttype]->g);
										time2 += jobs[ttype]->g[*(vp.second-1)].end_time;
									}
									time2 /= jobs.size();
									bool failcondition = false;
									if(!timeorcost) failcondition=cost2>=initialcost;
									else {
										if(time2>time1) failcondition = true;
										else if(time2==time1 && cost2>=initialcost)
											failcondition = true;
									}
									if(failcondition){ //new cost no less than before
										//promote only changes values, move_operation changes vector size
										vector<taskVertex*> tasks;
										for(int taskiter=0; taskiter<numoftasks; taskiter++){
											taskVertex* task = VM_queue[i][iter]->assigned_tasks[out].back();
											VM_queue[i][iter]->assigned_tasks[out].pop_back();
											tasks.push_back(task);										
										}
										VM_queue[j][k]->assigned_tasks.push_back(tasks);
										if(numoftasks==0)
											printf("");
										for(int t=0; t<types; t++)
											for(int s=0; s<VM_queue_state[t].size(); s++)
												//*VM_queue[t][s] = *VM_queue_state[t][s];
												deepcopy(VM_queue[t][s], VM_queue_state[t][s]);
										continue;//break to the next out queue
									}
									if(checkcost){
										vector<taskVertex*> tasks;
										for(int taskiter=0; taskiter<numoftasks; taskiter++){
											taskVertex* task = VM_queue[i][iter]->assigned_tasks[out].back();
											VM_queue[i][iter]->assigned_tasks[out].pop_back();
											tasks.push_back(task);										
										}
										VM_queue[j][k]->assigned_tasks.push_back(tasks);
										for(int t=0; t<types; t++)
											for(int s=0; s<VM_queue_backup[t].size(); s++)
												//*VM_queue[t][s] = *VM_queue_state[t][s];
												deepcopy(VM_queue[t][s], VM_queue_backup[t][s]);
										deepdelete(VM_queue_backup);
										deepdelete(VM_queue_state);
										if(!timeorcost)
											return initialcost-cost2;
										else
											return time1-time2;
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
									if(finalcost>initialcost){									
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
										printf("promote operation and move operation 2\n");
										if(!timeorcost)
											return initialcost-cost2;
										else
											return time1-time2;
									//}
								}else{ //
									for(int t=0; t<types; t++)
										for(int s=0; s<VM_queue_state[t].size(); s++)	{
											deepcopy(VM_queue[t][s],VM_queue_state[t][s]);
										}
									costpromote -= costmove;
								}
							}														
						}//the out-th VM queue
					}
				}//the k-th type j task
				}//if !timeorcost
				deepdelete(VM_queue_state);
			}//try to promote next iter task
		}//promote to next type j
	}//next VM queue

	for(int t=0; t<types; t++)
		for(int s=0; s<VM_queue_backup[t].size(); s++)	
			deepcopy(VM_queue[t][s],VM_queue_backup[t][s]);
	deepdelete(VM_queue_backup);
	return 0;
}