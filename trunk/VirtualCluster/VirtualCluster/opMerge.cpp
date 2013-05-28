#include "stdafx.h"
#include "ConsolidateOperators.h"

double opMerge(vector<VM*>*  VM_queue, vector<Job*> jobs, bool checkcost, bool estimate, bool timeorcost)
{
	if(timeorcost&&estimate&&checkcost)
		return 0;//does not reduce time

	double domerge = false;
	double costmerge = 0;
	double timemerge = 0;

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
					if(task->end_time < task->start_time)
						printf("");
				}
				vm->assigned_tasks.push_back(tasks);				
			}
			vms.push_back(vm);
		}
		VM_queue_backup.push_back(vms);
	}

	//merge
	double initialcost=0;
	for(int ttype=0; ttype<types; ttype++)
		for(int tsize=0; tsize<VM_queue[ttype].size(); tsize++)
			initialcost += priceOnDemand[VM_queue[ttype][tsize]->type]*VM_queue[ttype][tsize]->life_time /60.0;

	for(int i=0; i<types; i++){
		for(int k=0; k<VM_queue[i].size(); k++){
			//k-th vm merge with j-th vm
			for(int j=0; j<VM_queue[i].size(); j++){
				if(k!=j && VM_queue[i][j]->assigned_tasks.size() == 1){
					for(int out=0; out<VM_queue[i][k]->assigned_tasks.size(); out++){
						//int taskend = VM_queue[i][k]->assigned_tasks[out].size()-1;
						//sort(VM_queue[i][k]->assigned_tasks[out].begin(),VM_queue[i][k]->assigned_tasks[out].end(),taskfunction);
						//VMs[i] merge with the one after it or in front of it						
						
						double outendtime = 0;
						double outstarttime = VM_queue[i][k]->assigned_tasks[out][0]->start_time;
						for(int in=0; in<VM_queue[i][k]->assigned_tasks[out].size(); in++){
							if(VM_queue[i][k]->assigned_tasks[out][in]->end_time > outendtime)
								outendtime = VM_queue[i][k]->assigned_tasks[out][in]->end_time;
							if(VM_queue[i][k]->assigned_tasks[out][in]->start_time < outstarttime)
								outstarttime =VM_queue[i][k]->assigned_tasks[out][in]->start_time;// = outstarttime;
						}
						double t1= VM_queue[i][j]->end_time - VM_queue[i][j]->start_time;
						double t2 = outendtime-outstarttime;//VM_queue[i][k]->assigned_tasks[out][taskend]->end_time - VM_queue[i][k]->assigned_tasks[out][0]->start_time;
						double t3;
						if(VM_queue[i][j]->start_time >= outendtime)//VM_queue[i][k]->assigned_tasks[out][taskend]->end_time)
							t3 = VM_queue[i][j]->end_time -outstarttime;//VM_queue[i][k]->assigned_tasks[out][0]->start_time;
						else if(VM_queue[i][j]->end_time <=outstarttime)//VM_queue[i][k]->assigned_tasks[out][0]->start_time)
							t3 = outendtime-VM_queue[i][j]->start_time;//VM_queue[i][k]->assigned_tasks[out][taskend]->end_time - VM_queue[i][j]->start_time;
						else continue;//to the next VM queue
						double costmove = move(i,t1,t2,t3);
						double timemerge = 0;
						costmerge = costmove;	
						if(costmerge>10)
							printf("");
						
						if(checkcost && estimate && costmerge>1e-12 &&!timeorcost){
							for(int t=0; t<types; t++)
								for(int s=0; s<VM_queue_backup[t].size(); s++)
									//*VM_queue[t][s] = *VM_queue_state[t][s];
									deepcopy(VM_queue[t][s], VM_queue_backup[t][s]);
							deepdelete(VM_queue_backup);
							return costmerge;
						}else if(checkcost && estimate &&!timeorcost){
							for(int t=0; t<types; t++){
								for(int s=0; s<VM_queue_backup[t].size(); s++)	{
									deepcopy(VM_queue[t][s],VM_queue_backup[t][s]);
								}					
							}
							continue;
						}else if(!checkcost && estimate ){//since do not change time, when cost lower, do it
							if(costmerge>1e-12){
								double cost1=0;
								for(int ttype=0; ttype<types; ttype++)
									for(int tsize=0; tsize<VM_queue[ttype].size(); tsize++)
										cost1 += priceOnDemand[VM_queue[ttype][tsize]->type]*VM_queue[ttype][tsize]->life_time /60.0;
								//do merge
								int numoftasks = VM_queue[i][j]->assigned_tasks[0].size();//how many tasks are merged to vm[i][iiter]
								move_operation2(jobs,VM_queue[i][j],VM_queue[i][k],out);
								bool update = true;
								updateVMqueue(VM_queue);
								//check budget
								if(timeorcost){
									bool checkbudget=true;
									double totalcost=0;
									for(int iindex=0; iindex<types; iindex++)
										for(int jindex=0; jindex <VM_queue[iindex].size(); jindex++)
											totalcost+= ceil((VM_queue[iindex][jindex]->end_time-VM_queue[iindex][jindex]->start_time)/60)*priceOnDemand[VM_queue[iindex][jindex]->type];
									if(totalcost/jobs.size() > jobs[0]->budget) checkbudget = false;
									update = checkbudget;
								}
								double cost2=0;
								for(int ttype=0; ttype<types; ttype++)
									for(int tsize=0; tsize<VM_queue[ttype].size(); tsize++)
										cost2 += priceOnDemand[VM_queue[ttype][tsize]->type]*VM_queue[ttype][tsize]->life_time /60.0;
								if(cost2>cost1 || !update){ 
									//new cost is larger, so go back to backup
									vector<taskVertex*> tasks;
									for(int taskiter=0; taskiter<numoftasks; taskiter++){
										taskVertex* task = VM_queue[i][k]->assigned_tasks[out].back();
										VM_queue[i][k]->assigned_tasks[out].pop_back();
										tasks.push_back(task);
									}
									VM_queue[i][j]->assigned_tasks.push_back(tasks);
									if(numoftasks==0)
										printf("");
									for(int t=0; t<types; t++)
										for(int s=0; s<VM_queue_backup[t].size(); s++)
											//*VM_queue[t][s] = *VM_queue_state[t][s];
											deepcopy(VM_queue[t][s], VM_queue_backup[t][s]);
									continue;//break to the next out queue
								}else{	
									VM_queue[i].erase(VM_queue[i].begin()+j);
									deepdelete(VM_queue_backup);
									printf("merge operation\n");
									return costmerge;
								}
							}else{
								for(int t=0; t<types; t++){
									for(int s=0; s<VM_queue_backup[t].size(); s++)	{
										deepcopy(VM_queue[t][s],VM_queue_backup[t][s]);
									}					
								}
							}
						}else if(!estimate){//&&!timeorcost){
							if(costmerge > 1e-12){
								double cost1=0;
								for(int ttype=0; ttype<types; ttype++)
									for(int tsize=0; tsize<VM_queue[ttype].size(); tsize++)
										cost1 += priceOnDemand[VM_queue[ttype][tsize]->type]*VM_queue[ttype][tsize]->life_time /60.0;
								double time1 = 0;
								for(int ttype=0; ttype<jobs.size(); ttype++){
									pair<vertex_iter, vertex_iter> vp;
									vp = vertices(jobs[ttype]->g);
									time1 += jobs[ttype]->g[*(vp.second-1)].end_time;
								}
								time1 /= jobs.size();
								//do merge
								int numoftasks = VM_queue[i][j]->assigned_tasks[0].size();//how many tasks are merged to vm[i][iiter]
								move_operation2(jobs,VM_queue[i][j],VM_queue[i][k],out);
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
								if(!timeorcost) failcondition=cost2>=cost1;
								else {
									if(time2>time1) failcondition = true;
									else if(time2==time1 && cost2>=cost1)
										failcondition = true;
								}
								if(failcondition){ 
									//new cost is larger, so go back to backup
									vector<taskVertex*> tasks;
									for(int taskiter=0; taskiter<numoftasks; taskiter++){
										taskVertex* task = VM_queue[i][k]->assigned_tasks[out].back();
										VM_queue[i][k]->assigned_tasks[out].pop_back();
										tasks.push_back(task);
									}
									VM_queue[i][j]->assigned_tasks.push_back(tasks);
									if(numoftasks==0)
										printf("");
									for(int t=0; t<types; t++)
										for(int s=0; s<VM_queue_backup[t].size(); s++)
											//*VM_queue[t][s] = *VM_queue_state[t][s];
											deepcopy(VM_queue[t][s], VM_queue_backup[t][s]);
									continue;//break to the next out queue
								}else{
									if(checkcost){
										vector<taskVertex*> tasks;
										for(int taskiter=0; taskiter<numoftasks; taskiter++){
											taskVertex* task = VM_queue[i][k]->assigned_tasks[out].back();
											VM_queue[i][k]->assigned_tasks[out].pop_back();
											tasks.push_back(task);
										}
										VM_queue[i][j]->assigned_tasks.push_back(tasks);
										for(int t=0; t<types; t++)
											for(int s=0; s<VM_queue_backup[t].size(); s++)
												//*VM_queue[t][s] = *VM_queue_state[t][s];
												deepcopy(VM_queue[t][s], VM_queue_backup[t][s]);
										deepdelete(VM_queue_backup);
										if(!timeorcost)
											return cost1-cost2;
										else
											return time1-time2;
									}
									//VM_queue[j].push_back(VM_queue[i][iter]);
									//VM_queue[i].erase(VM_queue[i].begin()+iter);
									//delete VM_queue[j][k];
									//VM_queue[j][k] = NULL;
									VM_queue[i].erase(VM_queue[i].begin()+j);
									deepdelete(VM_queue_backup);
									printf("merge operation\n");
									if(!timeorcost)
										return cost1-cost2;
									else
										return time1-time2;
								}
							}else{
								for(int t=0; t<types; t++){
									for(int s=0; s<VM_queue_backup[t].size(); s++)	{
										deepcopy(VM_queue[t][s],VM_queue_backup[t][s]);
									}					
								}
							}
						}						
					}//out-th task queue
				}
			}//j-th vm
		}//k-th vm
	}
	for(int t=0; t<types; t++){
		for(int s=0; s<VM_queue_backup[t].size(); s++)	{
			deepcopy(VM_queue[t][s],VM_queue_backup[t][s]);
		}
	}
	deepdelete(VM_queue_backup);
	return 0;
}