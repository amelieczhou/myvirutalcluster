#include "stdafx.h"
#include "ConsolidateOperators.h"

double opMerge(vector<VM*>*  VM_queue, vector<Job*> jobs)
{/*
	double domerge = false;
	double costmerge = 0;
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
	return 0;
}