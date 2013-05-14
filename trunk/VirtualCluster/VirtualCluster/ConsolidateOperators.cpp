#include "stdafx.h"
#include "Provision.h"
#include "ConsolidateOperators.h"
#include <boost/graph/topological_sort.hpp>

//Breath-first traverse to update the start and end time of jobs
void BFS_update(Job* testJob)
{
	std::deque<int> topo_order;
	topological_sort(testJob->g,std::front_inserter(topo_order));
	//tasks ordered in front are parents to tasks ordered behind them
	for(int i=0; i < topo_order.size(); i++) {
		testJob->g[i].start_time = testJob->g[i].end_time = 0;
	}
	for(std::deque<int>::const_iterator i = topo_order.begin(); i != topo_order.end(); ++i) {
		taskVertex* visited_task = &testJob->g[*i];
		visited_task->end_time = visited_task->start_time + visited_task->estTime[visited_task->assigned_type];
		//get child vertices
		out_edge_iterator out_i, out_end;
		edge_descriptor e;
		for (boost::tie(out_i, out_end) = out_edges(visited_task->name, testJob->g); out_i != out_end; ++out_i) {
			e = *out_i;
			Vertex tgt = target(e,testJob->g);
			if(testJob->g[tgt].start_time < visited_task->end_time) 
				testJob->g[tgt].start_time = visited_task->end_time;
		}
	}
}

bool time_flood(Job* testJob, int task_no)
{
	taskVertex* start_task = &testJob->g[task_no];
    start_task->end_time = start_task->start_time + start_task->estTime[start_task->assigned_type];
    std::queue<taskVertex*> children;
    std::vector<int> children_name; //to check whether the task exist in the queue already
    children.push(start_task);      //the tasks pushed are already updated
    children_name.push_back(start_task->name);
    while(!children.empty()) {
            taskVertex* curr_task = children.front();
            //get children vertices
            out_edge_iterator out_i, out_end;
            edge_descriptor e;
            for (boost::tie(out_i, out_end) = out_edges(curr_task->name, testJob->g); out_i != out_end; ++out_i) 
            {
                    e = *out_i;
                    Vertex tgt = target(e,testJob->g);
                    taskVertex* child = &testJob->g[tgt];
                    double executetime = child->end_time - child->start_time;
					//consider start time move forward!
					in_edge_iterator in_i, in_end;
					edge_descriptor ine;
					double endtime=0;
					for(boost::tie(in_i, in_end) = in_edges(child->name,testJob->g); in_i != in_end; ++in_i){
						ine = *in_i;
						Vertex src = source(ine, testJob->g);
						if(testJob->g[src].end_time > endtime) endtime = testJob->g[src].end_time;
					}
                    child->start_time = endtime;//(curr_task->end_time > child->start_time)?curr_task->end_time : child->start_time;
                    child->end_time = child->start_time + executetime;
					if(child->end_time > child->sub_deadline)
						return false;
                    std::vector<int>::iterator p = std::find(children_name.begin(),children_name.end(),child->name);
                    if(p == children_name.end()) {
                            children.push(child); 
                            children_name.push_back(child->name);
                    }
            }
            children.pop();
    }  
	return true;
}
//cost estimation of move operator, return the gain of the operation
double move(int type, double x, double y, double total) {
	double p = priceOnDemand[type];
	double save = (std::ceil(x/60.0) + std::ceil(y/60.0) - std::ceil(total/60.0))*p;
	return save;
}
double split(int type, double t, double t1, double t2) {
	double p = priceOnDemand[type];
	double save = (std::ceil(t/60.0) - std::ceil(t1/60.0) - std::ceil(t2/60.0))*p;
	return save;
}
double demote(int type1, double t1, int type2, double t2) {	
	double p1 = priceOnDemand[type1], p2 = priceOnDemand[type2];
	double save = p1*std::ceil(t1/60.0) - p2*std::ceil(t2/60.0);
	return save;
}
double ppromote(int type1, double t1, int type2, double t2){
	double p1 = priceOnDemand[type1], p2 = priceOnDemand[type2];
	double save = p1*std::ceil(t1/60.0) - p2*std::ceil(t2/60.0);
	//find the critical path save

	return save;
}

void move_operation1(std::vector<Job*> jobs, VM* vm1, VM* vm2, int outiter, double moveafter) {
	//move 1 to 2's outiter task queue, 1 starts before the end of 2
	//vm1 only has one task queue
	//printf("move operation 1\n");
	//for(int taskiter=0; taskiter < vm1->assigned_tasks[0].size(); taskiter++) {
	//	double exetime = vm1->assigned_tasks[0][taskiter]->end_time - vm1->assigned_tasks[0][taskiter]->start_time;
	//	double oldestTime = vm1->assigned_tasks[0][taskiter]->estTime[vm1->assigned_tasks[0][taskiter]->assigned_type];
	//	vm1->assigned_tasks[0][taskiter]->start_time += moveafter;//do not reflect on jobs
	//	vm1->assigned_tasks[0][taskiter]->estTime[vm1->assigned_tasks[0][taskiter]->assigned_type] = exetime;
	//	time_flood(vm1->assigned_tasks[0][taskiter]);		
	//	vm1->assigned_tasks[0][taskiter]->estTime[vm1->assigned_tasks[0][taskiter]->assigned_type] = oldestTime;
	//	//test deadline
	//	/*std::pair<vertex_iter, vertex_iter> vp = vertices(jobs[vm1->assigned_tasks[0][taskiter]->job_id]->g);
	//	if(jobs[vm1->assigned_tasks[0][taskiter]->job_id]->g[*(vp.second-1)].end_time > vm1->assigned_tasks[0][taskiter]->sub_deadline)
	//		printf("deadline violated!---------------------------------------------------\n");*/
	//}
	for(int taskiter=0; taskiter < vm1->assigned_tasks[0].size(); taskiter++) {
		taskVertex* task = vm1->assigned_tasks[0].back();
		vm1->assigned_tasks[0].pop_back();
		taskiter --;
		//*task = *vm1->assigned_tasks[0][taskiter];
		vm2->assigned_tasks[outiter].push_back(task);
	}
	//sort(vm2->assigned_tasks[outiter].begin(),vm2->assigned_tasks[outiter].end(),taskfunction);
	vector<taskVertex*>::iterator taskiterator = vm2->assigned_tasks[outiter].end() - 1;
	if((*taskiterator)->end_time > vm2->end_time) vm2->end_time = (*taskiterator)->end_time;
	vm2->life_time = std::ceil((vm2->end_time - vm2->start_time)/60.0 )*60.0;
	vm2->resi_time = vm2->life_time - vm2->end_time + vm2->start_time;	
	//vm1->start_time = vm1->end_time = vm1->resi_time = vm1->life_time = 0;
	//vm1->assigned_tasks.clear();	
	/*for(int out=0; out<vm1->assigned_tasks.size();out++)
		for(int in=0; in<vm1->assigned_tasks[out].size(); in++){
			delete vm1->assigned_tasks[out][in];
			vm1->assigned_tasks[out][in] = NULL;
		}*/
	vm1->assigned_tasks.clear();
	/*delete vm1;
	vm1 = NULL;*/	
	vm1->life_time = vm1->start_time = vm1->end_time = vm1->resi_time =0;
}
void move_operation2(std::vector<Job*> jobs, VM* v1, VM* v2, int outiter){ //do not need to update v1
	//printf("move operation 2\n");
	for(int taskiter=0; taskiter < v1->assigned_tasks[0].size(); taskiter++){
		taskVertex* task = v1->assigned_tasks[0].back();
		v1->assigned_tasks[0].pop_back();
		taskiter--;
		//*task = *v1->assigned_tasks[0][taskiter];
		v2->assigned_tasks[outiter].push_back(task);
	}
	//sort(v2->assigned_tasks[outiter].begin(),v2->assigned_tasks[outiter].end(),taskfunction);
	v2->end_time = (v2->end_time > v1->end_time)?v2->end_time:v1->end_time;
	v2->start_time = (v2->start_time < v1->start_time )?v2->start_time:v1->start_time;
	v2->life_time = std::ceil((v2->end_time - v2->start_time)/60.0)*60.0;
	v2->resi_time = v2->life_time + v2->start_time - v2->end_time; 
	//v1->start_time = v1->end_time = v1->resi_time = v1->life_time = 0;
	//v1->assigned_tasks.clear();
	//v1->assigned_tasks[0][0]->cost = 100000;//for debug only
	
		/*for(int in=0; in<v1->assigned_tasks[0].size(); in++){
			delete v1->assigned_tasks[0][in];
			v1->assigned_tasks[0][in] = NULL;
		}*/
	v1->assigned_tasks.clear();
	v1->life_time = v1->start_time = v1->end_time = v1->resi_time =0;
	//delete v1;
	//v1 = NULL;	
}
void move_operation3(vector<Job*> jobs, VM* v1, int outiter, VM* v2, double moveafter) {
	//v2.start<v1.start,move v1.outiter to the end of v2, but delete v2
	//printf("move operation 3\n");
	//for(int i=0; i<v1->assigned_tasks[outiter].size(); i++)	{
	//	double exetime = v1->assigned_tasks[outiter][i]->end_time - v1->assigned_tasks[outiter][i]->start_time;
	//	int assignedtype = v1->assigned_tasks[outiter][i]->assigned_type;
	//	double oldestTime = v1->assigned_tasks[outiter][i]->estTime[assignedtype];
	//	v1->assigned_tasks[outiter][i]->start_time += moveafter;
	//	v1->assigned_tasks[outiter][i]->estTime[assignedtype] = exetime;
	//	time_flood(v1->assigned_tasks[outiter][i]);
	//	v1->assigned_tasks[outiter][i]->estTime[assignedtype] = oldestTime;
	//	//test deadline
	//	/*pair<vertex_iter,vertex_iter> vp = vertices(jobs[v1->assigned_tasks[outiter][i]->job_id]->g);
	//	if(jobs[v1->assigned_tasks[outiter][i]->job_id]->g[*(vp.second-1)].end_time > v1->assigned_tasks[outiter][i]->sub_deadline)
	//		printf("deadline violated!---------------------------------------------------\n");*/
	//}

	for(int i=0; i<v2->assigned_tasks[0].size(); i++){
		taskVertex* task = v2->assigned_tasks[0].back();
		v2->assigned_tasks[0].pop_back();
		i--;
		//*task = *v2->assigned_tasks[0][i];
		v1->assigned_tasks[outiter].push_back(task);
	}
	//sort(v1->assigned_tasks[outiter].begin(),v1->assigned_tasks[outiter].end(),taskfunction);
	vector<taskVertex*>::iterator enditerator = v1->assigned_tasks[outiter].end() -1;
	if(v1->assigned_tasks[outiter][0]->start_time < v1->start_time) v1->start_time = v1->assigned_tasks[outiter][0]->start_time;
	if((*enditerator)->end_time > v1->end_time) v1->end_time = (*enditerator)->end_time;
	v1->life_time = ceil((v1->end_time - v1->start_time)/60.0)*60.0;
	v1->resi_time = v1->start_time + v1->life_time - v1->end_time;
	/*for(int out=0; out<v2->assigned_tasks.size(); out++)
		for(int in=0; in<v2->assigned_tasks[out].size(); in++){
			delete v2->assigned_tasks[out][in];
			v2->assigned_tasks[out][in] = NULL;
		}*/
	v2->assigned_tasks.clear();
	v2->life_time = v2->start_time = v2->end_time = v2->resi_time =0;
	/*delete v2;
	v2 = NULL;*/	
}
void demote_operation(std::vector<Job*> jobs, VM* vm, int j) {
	//printf("demote operation\n");
	int oldtype = vm->type;
	vm->type = j;
	double lastend=0;
	for(int out=0; out<vm->assigned_tasks.size(); out++){
		//for(int in=0; in<vm->assigned_tasks[out].size(); in++){
		//	double exetime = vm->assigned_tasks[out][in]->end_time - vm->assigned_tasks[out][in]->start_time;
		//	double oldestTime = vm->assigned_tasks[out][in]->estTime[j];
		//	vm->assigned_tasks[out][in]->estTime[j] *= exetime/vm->assigned_tasks[out][in]->estTime[oldtype];
		//	vm->assigned_tasks[out][in]->assigned_type = j;
		//	if(in!=0 && vm->assigned_tasks[out][in-1]->end_time > vm->assigned_tasks[out][in]->start_time)	
		//		vm->assigned_tasks[out][in]->start_time = vm->assigned_tasks[out][in-1]->end_time;
		//	time_flood(jobs[vm->assigned_tasks[out][in]->job_id],vm->assigned_tasks[out][in]->name);
		//	vm->assigned_tasks[out][in]->estTime[j] = oldestTime;
		//	//test deadline
		//	std::pair<vertex_iter, vertex_iter> vp = vertices(jobs[vm->assigned_tasks[out][in]->job_id]->g);
		//	if(jobs[vm->assigned_tasks[out][in]->job_id]->g[*(vp.second-1)].end_time > jobs[vm->assigned_tasks[out][in]->job_id]->deadline)
		//		printf("deadline violated!---------------------------------------------------\n");
		//}
		sort(vm->assigned_tasks[out].begin(),vm->assigned_tasks[out].end(),taskfunction);
		vector<taskVertex*>::iterator taskiterator = vm->assigned_tasks[out].end()-1;
		if((*taskiterator)->end_time > lastend) lastend = (*taskiterator)->end_time;
	}
	vm->end_time = lastend;
	vm->life_time = std::ceil((vm->end_time - vm->start_time)/60.0 )*60.0;
	vm->resi_time = vm->start_time + vm->life_time - vm->end_time;
	vm->capacity -= (std::pow(2.0, oldtype) - std::pow(2.0, j));
}
void promote_operation(std::vector<Job*> jobs, VM* vm, int j) {
	//printf("promote operation\n");
	int oldtype = vm->type;
	vm->type = j;
	double lastend = 0;
	for(int out=0; out<vm->assigned_tasks.size(); out++) {
		//for(int in=0; in<vm->assigned_tasks[out].size(); in++) {
		//	double exetime = vm->assigned_tasks[out][in]->end_time - vm->assigned_tasks[out][in]->start_time;
		//	double oldestTime = vm->assigned_tasks[out][in]->estTime[j];
		//	vm->assigned_tasks[out][in]->estTime[j] *= exetime/vm->assigned_tasks[out][in]->estTime[oldtype];
		//	vm->assigned_tasks[out][in]->assigned_type = j;
		//	time_flood(jobs[vm->assigned_tasks[out][in]->job_id],vm->assigned_tasks[out][in]->name);
		//	vm->assigned_tasks[out][in]->estTime[j] = oldestTime;
		//	//test deadline
		//	std::pair<vertex_iter, vertex_iter> vp = vertices(jobs[vm->assigned_tasks[out][in]->job_id]->g);
		//	if(jobs[vm->assigned_tasks[out][in]->job_id]->g[*(vp.second-1)].end_time > jobs[vm->assigned_tasks[out][in]->job_id]->deadline)
		//		printf("deadline violated!---------------------------------------------------\n");
		//}
		sort(vm->assigned_tasks[out].begin(),vm->assigned_tasks[out].end(),taskfunction);
		vector<taskVertex*>::iterator taskiterator = vm->assigned_tasks[out].end()-1;
		if((*taskiterator)->end_time > lastend) lastend = (*taskiterator)->end_time;
	}
	vm->end_time = lastend;
	vm->life_time = std::ceil((vm->end_time - vm->start_time)/60.0 )*60.0;
	vm->resi_time = vm->start_time + vm->life_time - vm->end_time;
	vm->capacity += (std::pow(2.0, j) - std::pow(2.0, oldtype));
}
void coschedule_operation(std::vector<Job*> jobs, VM* vm1, VM* vm2, double degradation) {
	printf("coscheduling operation\n");
	vm1->capacity -= (std::pow(2.0,vm2->type) - vm2->capacity);
	double lastend = 0;
	for(int out=0; out<vm1->assigned_tasks.size(); out++){
		for(int in=0; in<vm1->assigned_tasks[out].size(); in++) {
			taskVertex* task = vm1->assigned_tasks[out][in]; //.back(), pop_back()
			double oldestTime = task->estTime[vm1->type];
			task->estTime[vm1->type] *= (task->end_time - task->start_time)/task->estTime[task->assigned_type] *(1+degradation);
			time_flood(jobs[task->job_id],task->name);
			//test deadline
			/*std::pair<vertex_iter, vertex_iter> vp = vertices(jobs[task->job_id]->g);
			if(jobs[task->job_id]->g[*(vp.second-1)].end_time > task->sub_deadline)
				printf("deadline violated!---------------------------------------------------\n");*/
			lastend = (lastend>task->end_time)?lastend:task->end_time;
			task->estTime[vm1->type] = oldestTime;
		}
	}
	for(int out=0; out<vm2->assigned_tasks.size(); out++){
		for(int in=0; in<vm2->assigned_tasks[out].size(); in++) {
			taskVertex* task = vm2->assigned_tasks[out][in];
			double oldestTime = task->estTime[vm1->type];
			task->estTime[vm1->type] *= (task->end_time - task->start_time)/task->estTime[task->assigned_type] *(1+degradation);
			task->assigned_type = vm1->type;
			time_flood(jobs[task->job_id],task->name);
			//test deadline
			/*std::pair<vertex_iter, vertex_iter> vp = vertices(jobs[task->job_id]->g);
			if(jobs[task->job_id]->g[*(vp.second-1)].end_time > jobs[task->job_id]->deadline)
				printf("deadline violated!---------------------------------------------------\n");*/
			lastend = (lastend>task->end_time)?lastend:task->end_time;
			task->estTime[vm1->type] = oldestTime;
		}
		vm1->assigned_tasks.push_back(vm2->assigned_tasks[out]);
	}
	vm1->end_time = lastend;
	vm1->life_time = std::ceil((vm1->end_time - vm1->start_time)/60.0)*60.0;
	vm1->resi_time = vm1->start_time+vm1->life_time-vm1->end_time;
	//vm2->assigned_tasks.clear();
	//vm2->start_time = vm2->end_time = vm2->life_time = vm2->resi_time = 0;
	delete vm2;
	vm2 = NULL;
}

double coschedule(int type1, int type2, int type, double t1, double t2, double t3, double t4, double degradation){
	double p1 = priceOnDemand[type1], p2 = priceOnDemand[type2], p = priceOnDemand[type];
	double cost_old = std::ceil(t1/60.0)*p1 + std::ceil(t2/60.0)*p2;
	double cost3 = std::ceil(t3*(1+degradation)/60.0)*p; 
	double cost4 = std::ceil(t4*(1+degradation)/60.0)*p;
	double cost_new = (cost3 > cost4) ? cost3 : cost4;
	return cost_old - cost_new;
}

double merge(int type, double* times, int size) {
	double p = priceOnDemand[type];
	double save=0, sum=0;
	for(int i=0; i<size; i++) {
		sum += times[i];
		save += std::ceil(times[i]/60.0);
	}
	save = (save - std::ceil(sum/60.0))*p;
	return save;
}

bool updateVMqueue(vector<VM*>* VM_queue)
{
	double oldcost=0, newcost=0;
	for(int s=0; s<types; s++)
		for(int t=0; t<VM_queue[s].size(); t++){
			oldcost += priceOnDemand[VM_queue[s][t]->type]*VM_queue[s][t]->life_time /60.0;
		}
	for(int s=0; s<types; s++)
		for(int t=0; t<VM_queue[s].size(); t++){
			//if(s==3 && t==18)
				//printf("hold here\n");
			//for debuging
			double oldstarttime = VM_queue[s][t]->start_time;
			double oldendtime = VM_queue[s][t]->end_time;
			if(VM_queue[s][t]->assigned_tasks.size()>0){
				double newstartime = VM_queue[s][t]->assigned_tasks[0][0]->start_time;
				double newendtime = 0;
				for(int out=0; out<VM_queue[s][t]->assigned_tasks.size(); out++){
					for(int in=0; in<VM_queue[s][t]->assigned_tasks[out].size(); in++){
						if(VM_queue[s][t]->assigned_tasks[out][in]->start_time < newstartime)
							newstartime = VM_queue[s][t]->assigned_tasks[out][in]->start_time;
						if(VM_queue[s][t]->assigned_tasks[out][in]->end_time > newendtime)
							newendtime = VM_queue[s][t]->assigned_tasks[out][in]->end_time;
					}
				}
				VM_queue[s][t]->start_time = newstartime;
				VM_queue[s][t]->end_time = newendtime;
				VM_queue[s][t]->life_time = ceil((newendtime - newstartime)/60.0)*60.0;
				VM_queue[s][t]->resi_time = VM_queue[s][t]->life_time - VM_queue[s][t]->end_time + VM_queue[s][t]->start_time;
			}

		//	double starttime = VM_queue[s][t]->start_time;
		//	double endtime = VM_queue[s][t]->end_time;
		//	int size =  VM_queue[s][t]->assigned_tasks[0].size();
		//	vector<pair<double, double> > old_start_end;
		//	for(int i=0; i<size; i++){
		//		pair<double, double> startend;
		//		startend.first =  VM_queue[s][t]->assigned_tasks[0][i]->start_time;
		//		startend.second = VM_queue[s][t]->assigned_tasks[0][i]->end_time;
		//		old_start_end.push_back(startend);
		//	}			
		//	if(size>0){
		//		double oldcost = priceOnDemand[VM_queue[s][t]->type]*VM_queue[s][t]->life_time/60.0;
		//		double newcost = 0;			
		//		VM_queue[s][t]->start_time = VM_queue[s][t]->assigned_tasks[0][0]->start_time;
		//		VM_queue[s][t]->end_time = VM_queue[s][t]->assigned_tasks[0][0]->end_time;
		//		for(int outiter=0; outiter<VM_queue[s][t]->assigned_tasks.size(); outiter++){
		//			sort(VM_queue[s][t]->assigned_tasks[outiter].begin(),VM_queue[s][t]->assigned_tasks[outiter].end(),taskfunction);
		//			if(VM_queue[s][t]->start_time > VM_queue[s][t]->assigned_tasks[outiter][0]->start_time) VM_queue[s][t]->start_time = VM_queue[s][t]->assigned_tasks[outiter][0]->start_time;
		//			int size = VM_queue[s][t]->assigned_tasks[outiter].size();
		//			if(VM_queue[s][t]->end_time < VM_queue[s][t]->assigned_tasks[outiter][size-1]->end_time) VM_queue[s][t]->end_time = VM_queue[s][t]->assigned_tasks[outiter][size-1]->end_time;
		//		}
		//		VM_queue[s][t]->life_time = ceil((VM_queue[s][t]->end_time - VM_queue[s][t]->start_time)/60.0)*60.0;
		//		VM_queue[s][t]->resi_time = VM_queue[s][t]->start_time + VM_queue[s][t]->life_time - VM_queue[s][t]->end_time;
		//		newcost = priceOnDemand[VM_queue[s][t]->type]*VM_queue[s][t]->life_time/60.0;
		//		if(newcost>oldcost) {
		//			//printf("newcost > oldcost!\n");
		//			return false;
		//		}
		//	}
		}
	for(int s=0; s<types; s++)
		for(int t=0; t<VM_queue[s].size(); t++){
			newcost += priceOnDemand[VM_queue[s][t]->type]*VM_queue[s][t]->life_time /60.0;
		}
	if(newcost > oldcost) return false;
	else return true;
}

//void siteRecovery(vector<Job*> jobs, VM* vm){
//	for(int out=0; out<vm->assigned_tasks.size(); out++)
//		for(int in=0; in<vm->assigned_tasks[out].size(); in++){	
//			vm->assigned_tasks[out][in]->assigned_type = vm->assigned_tasks[out][in]->prefer_type;
//			vm->assigned_tasks[out][in]->start_time = vm->assigned_tasks[out][in]->restTime;
//			Job* currentJob = jobs[vm->assigned_tasks[out][in]->job_id];
////			time_flood(currentJob,vm->assigned_tasks[out][in]->name);
//		}
//}

void deepcopy(VM* dest, VM* src){
	dest->capacity = src->capacity;
	dest->end_time = src->end_time;
	dest->idle_time = src->idle_time;
	dest->in_use = src->in_use;
	dest->life_time = src->life_time;
	dest->resi_time = src->resi_time;
	dest->start_time = src->start_time;
	dest->type = src->type;
	//if(dest->assigned_tasks.empty()){//where to free such space?
	//	dest->assigned_tasks.clear();
	//	for(int out=0; out<src->assigned_tasks.size(); out++){
	//		vector<taskVertex*> tasks;
	//		for(int in=0; in<src->assigned_tasks[out].size(); in++){
	//			taskVertex* task = new taskVertex();
	//			*task = *src->assigned_tasks[out][in];
	//			task->cost = 100000;//for test only
	//			//task->cost = 0;
	//			tasks.push_back(task);
	//		}
	//		dest->assigned_tasks.push_back(tasks);
	//	}
	//}else{
		for(int out=0; out<src->assigned_tasks.size(); out++)
			for(int in=0; in<src->assigned_tasks[out].size(); in++){
				*dest->assigned_tasks[out][in] = *src->assigned_tasks[out][in];
				dest->assigned_tasks[out][in]->cost = 100000;//for test only
				//dest->assigned_tasks[out][in]->cost = 0;
			}
	//}
}
void deepdelete(vector<vector<VM*> > VM_queue_backup){
	for(int t=0; t<types; t++){
		for(int s=0; s<VM_queue_backup[t].size(); s++){
			for(int out=0; out<VM_queue_backup[t][s]->assigned_tasks.size();out++){
				for(int in=0; in<VM_queue_backup[t][s]->assigned_tasks[out].size(); in++)
				{
					delete VM_queue_backup[t][s]->assigned_tasks[out][in];
					VM_queue_backup[t][s]->assigned_tasks[out][in] = NULL;
				}
			}
			delete VM_queue_backup[t][s];
			VM_queue_backup[t][s] = NULL;
		}
		VM_queue_backup[t].clear();
	}
	VM_queue_backup.clear();
}
bool vmfunction(VM* a, VM* b) {
	return (a->start_time < b->start_time);
}
bool taskfunction(taskVertex* a, taskVertex* b)
{
	return (a->start_time < b->start_time);
}
bool comp_by_first(std::pair<double, std::pair<int,int> > a, std::pair<double, std::pair<int,int> > b) {
	return (a.first < b.first);
}