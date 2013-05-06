#include "stdafx.h"
#include "ConsolidateOperators.h"

double opPromote(vector<VM*>* VM_queue, vector<Job*> jobs)
{
	for(int i=0; i<types; i++)
		for(int j=0; j<VM_queue[i].size(); j++)
			for(int out=0; out<VM_queue[i][j]->assigned_tasks.size(); out++)
				for(int in=0; in<VM_queue[i][j]->assigned_tasks[out].size(); in++)
					VM_queue[i][j]->assigned_tasks[out][in]->start_time=0;
	//Rule 1, promote can merge with left or right
	bool dopromote = false;
	double costpromote = 0;

	//save context for site recovery to the very beginning
	vector<vector<VM*> > VM_queue_backup;
	for(int i=0; i<types; i++){
		vector<VM*> vms;		
		for(int j=0; j<VM_queue[i].size(); j++){
			VM* vm = new VM();
			*vm = *VM_queue[i][j];
			vms.push_back(vm);
		}
		VM_queue_backup.push_back(vms);
	}

	pair<vertex_iter, vertex_iter> vp;
	std::vector<VM*> VMs;
	for(int i=0; i<types; i++) 
		for(int j=0; j<VM_queue[i].size(); j++) 
			VMs.push_back(VM_queue[i][j]);
	std::sort(VMs.begin(),VMs.end(),vmfunction);

	for(int i=0; i<VMs.size(); i++){
		//promote from type+1 to the best
		for(int j=VMs[i]->type+1; j<types; j++){
			//promote to type j, no need to check promote deadline but move deadline
			bool deadlineok;		
			double t1 = VMs[i]->end_time - VMs[i]->start_time;
			//update tasks see if dealine violation
			for(int out=0; out<VMs[i]->assigned_tasks.size(); out++){
				for(int in=0; in<VMs[i]->assigned_tasks[out].size(); in++){
					VMs[i]->assigned_tasks[out][in]->assigned_type = j;
					deadlineok = time_flood(VMs[i]->assigned_tasks[out][in]);
					if(!deadlineok){
						printf("promote violate deadline!\n");
						break;
					}
				}
				if(!deadlineok) break;
			}
			if(!deadlineok) {
				//site recovery
				for(int t=0; t<types; t++)
					for(int s=0; s<VM_queue_backup[t].size(); s++)
						*VM_queue[t][s] = *VM_queue_backup[t][s];												
				break;
			}
			//if deadline is ok, update the VM start and end time accordingly
			updateVMqueue(VM_queue);
			double t2 = VMs[i]->end_time - VMs[i]->start_time;	
			costpromote = ppromote(VMs[i]->type,t1,j,t2);
			//save context at the status when the assigned type of tasks are changed
			vector<vector<VM*> > VM_queue_state;
			for(int i=0; i<types; i++){
				vector<VM*> vms;		
				for(int j=0; j<VM_queue[i].size(); j++){
					VM* vm = new VM();
					*vm = *VM_queue[i][j];			
					vms.push_back(vm);
				}
				VM_queue_state.push_back(vms);
			}
			//check merge
			for(int k=0; k<VM_queue[j].size(); k++){
				if(VM_queue[j][k]->assigned_tasks.size()==1){
					for(int out=0; out<VMs[i]->assigned_tasks.size(); out++){
						int taskend = VMs[i]->assigned_tasks[out].size()-1;	
						sort(VMs[i]->assigned_tasks[out].begin(),VMs[i]->assigned_tasks[out].end(),taskfunction);
						//VMs[i] merge with the one after it or in front of it						
						double t1= VM_queue[j][k]->end_time - VM_queue[j][k]->start_time;
						double t2 = VMs[i]->assigned_tasks[out][taskend]->end_time - VMs[i]->assigned_tasks[out][0]->start_time;
						double t3;
						if(VM_queue[j][k]->start_time >= VMs[i]->assigned_tasks[out][taskend]->end_time)
							t3 = VM_queue[j][k]->end_time - VMs[i]->assigned_tasks[out][0]->start_time;
						else if(VM_queue[j][k]->end_time <= VMs[i]->assigned_tasks[out][0]->start_time)
							t3 = VMs[i]->assigned_tasks[out][taskend]->end_time - VM_queue[j][k]->start_time;
						else break;
						costpromote += move(j,t1,t2,t3);				
						if(costpromote > 1e-12){
							//first do the promote 
							promote_operation(jobs,VMs[i],j);
							//then do merge
							move_operation2(jobs,VM_queue[j][k],VMs[i],out);
							bool update = updateVMqueue(VM_queue);
							if(!update){ //cost saved<0, site recovery to state
								for(int t=0; t<types; t++)
									for(int s=0; s<VM_queue_state[t].size(); s++)
										*VM_queue[t][s] = *VM_queue_state[t][s];
								break;//break to the next out queue
							}
							VM_queue[j].push_back(VMs[i]);
							int demoteiter;
							for(int iter=0; iter<VM_queue[VMs[i]->type].size();iter++)
								if(VM_queue[VMs[i]->type][iter] == VMs[i])
									demoteiter = iter;
							VM_queue[VMs[i]->type].erase(VM_queue[VMs[i]->type].begin()+demoteiter);
							delete VM_queue[j][k];
							VM_queue[j][k] = NULL;
							VM_queue[j].erase(VM_queue[j].begin()+k);
							for(int t=0; t<types; t++){
								for(int s=0; s<VM_queue_backup[t].size(); s++){		
									delete VM_queue_backup[t][s];
									VM_queue_backup[t][s] = NULL;
								}
								for(int s=0; s<VM_queue_state[t].size(); s++){
									delete VM_queue_state[t][s];
									VM_queue_state[t][s] = NULL;									
								}	
								VM_queue_state[t].clear();
							}
							VM_queue_state.clear();
							printf("promote operation and move operation 2\n");
							return costpromote;
						}else{ //costpromote <= 0
							for(int t=0; t<types; t++)
								for(int s=0; s<VM_queue_backup[t].size(); s++)
									*VM_queue[t][s] = *VM_queue_state[t][s];							
							break;
						}
					}//out-th queue of VMs[i]
				}
			}//check next task of type j
			for(int t=0; t<types; t++){
				for(int s=0; s<VM_queue_backup[t].size(); s++)
					*VM_queue[t][s] = *VM_queue_backup[t][s];
				for(int s=0; s<VM_queue_state[t].size(); s++){
					delete VM_queue_state[t][s];
					VM_queue_state[t][s] = NULL;
				}
				VM_queue_state[t].clear();
			}
			VM_queue_state.clear();
		}//next j type
	}
	for(int t=0; t<types; t++){
		for(int s=0; s<VM_queue_backup[t].size(); s++){		
			delete VM_queue_backup[t][s];
			VM_queue_backup[t][s] = NULL;
		}
		VM_queue_backup[t].clear();
	}
	
	return 0;
}