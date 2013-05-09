#include "stdafx.h"
#include "ConsolidateOperators.h"

double opMerge(vector<VM*>*  VM_queue, vector<Job*> jobs)
{
	double domerge = false;
	double costmerge = 0;

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
	//merge
	/*for(int i=0; i<types; i++) {
		for(int k=0; k< VM_queue[i].size(); k++) {
			//do until nothing can be done, merge with the VM after it
			for(int j=0; j<VM_queue[i].size(); j++){
				if(VM_queue[i][k]->end_time < VM_queue[i][j]->start_time &&
					VM_queue[i][k]->resi_time + VM_queue[i][j]->resi_time < 60 &&
					VM_queue[i][k]->assigned_tasks.size() == 1 && VM_queue[i][j]->assigned_tasks.size() == 1) {// can merge
					domerge = true;
					double t1 = VM_queue[i][k]->end_time - VM_queue[i][k]->start_time;
					double t2 = VM_queue[i][j]->end_time - VM_queue[i][j]->start_time;
					double t3 = VM_queue[i][j]->end_time - VM_queue[i][k]->start_time;
					costmerge += move(i,t1,t2, t3);
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
	*/
	for(int i=0; i<types; i++){
		for(int k=0; k<VM_queue[i].size(); k++){
			//k-th vm merge with j-th vm
			for(int j=0; j<VM_queue[i].size(); j++){
				if(k!=j && VM_queue[i][j]->assigned_tasks.size() == 1){
					for(int out=0; out<VM_queue[i][k]->assigned_tasks.size(); out++){
						int taskend = VM_queue[i][k]->assigned_tasks[out].size()-1;
						sort(VM_queue[i][k]->assigned_tasks[out].begin(),VM_queue[i][k]->assigned_tasks[out].end(),taskfunction);
						//VMs[i] merge with the one after it or in front of it						
						double t1= VM_queue[i][j]->end_time - VM_queue[i][j]->start_time;
						double t2 = VM_queue[i][k]->assigned_tasks[out][taskend]->end_time - VM_queue[i][k]->assigned_tasks[out][0]->start_time;
						double t3;
						if(VM_queue[i][j]->start_time >= VM_queue[i][k]->assigned_tasks[out][taskend]->end_time)
							t3 = VM_queue[i][j]->end_time -VM_queue[i][k]->assigned_tasks[out][0]->start_time;
						else if(VM_queue[i][j]->end_time <=VM_queue[i][k]->assigned_tasks[out][0]->start_time)
							t3 = VM_queue[i][k]->assigned_tasks[out][taskend]->end_time - VM_queue[i][j]->start_time;
						else continue;//to the next VM queue
						double costmove = move(j,t1,t2,t3);
						costmerge = costmove;		
						if(costmerge > 1e-12){
							double cost1=0;
							for(int ttype=0; ttype<types; ttype++)
								for(int tsize=0; tsize<VM_queue[ttype].size(); tsize++)
									cost1 += priceOnDemand[VM_queue[ttype][tsize]->type]*VM_queue[ttype][tsize]->life_time /60.0;
							//do merge
							int numoftasks = VM_queue[i][j]->assigned_tasks[0].size();//how many tasks are merged to vm[i][iiter]
							move_operation2(jobs,VM_queue[i][j],VM_queue[i][k],out);
							bool update = updateVMqueue(VM_queue);
							double cost2=0;
							for(int ttype=0; ttype<types; ttype++)
								for(int tsize=0; tsize<VM_queue[ttype].size(); tsize++)
									cost2 += priceOnDemand[VM_queue[ttype][tsize]->type]*VM_queue[ttype][tsize]->life_time /60.0;
							if(cost2>cost1){ 
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
								//VM_queue[j].push_back(VM_queue[i][iter]);
								//VM_queue[i].erase(VM_queue[i].begin()+iter);
								//delete VM_queue[j][k];
								//VM_queue[j][k] = NULL;
								VM_queue[i].erase(VM_queue[i].begin()+j);
								for(int t=0; t<types; t++){
									for(int s=0; s<VM_queue_backup[t].size(); s++){	
										for(int out=0;out<VM_queue_backup[t][s]->assigned_tasks.size(); out++)
											for(int in=0; in<VM_queue_backup[t][s]->assigned_tasks[out].size(); in++){
												delete VM_queue_backup[t][s]->assigned_tasks[out][in];
												VM_queue_backup[t][s]->assigned_tasks[out][in] = NULL;
											}
										delete VM_queue_backup[t][s];
										VM_queue_backup[t][s] = NULL;
									}
									VM_queue_backup[t].clear();
								}
								VM_queue_backup.clear();
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
					}//out-th task queue
				}
			}//j-th vm
		}//k-th vm
	}
	return 0;
}