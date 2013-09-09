#include "stdafx.h"
#include "ConsolidateOperators.h"
#include "Algorithms.h"
#include <fstream>
#include <string>
#include <sstream>
#include <time.h>
//#include <boost/graph/topological_sort.hpp>

extern double max_t;
extern bool DynamicSchedule;
extern double deadline;
extern double chk_period;
extern bool sharing;
extern bool on_off;
extern double lamda;
extern double QoS;
extern double budget;

void GanttConsolidation::Initialization(Job* testJob, int policy, bool timeorcost)
{
	std::pair<vertex_iter, vertex_iter> vp;
	//vp=vertices(testJob->g);
	//for(; vp.first!=vp.second; ++vp.first){
	//	//find the children of all tasks
	//	//get children vertices
	//	out_edge_iterator out_i, out_end;
	//	edge_descriptor e;
	//	for (boost::tie(out_i, out_end) = out_edges(testJob->g[*vp.first].name, testJob->g); out_i != out_end; ++out_i) 
	//	{
	//		e = *out_i;
	//		Vertex tgt = target(e,testJob->g);
	//		taskVertex* child = &testJob->g[tgt];
	//		testJob->g[*vp.first].children.push_back(child);
	//	}
	//}

	vp=vertices(testJob->g);
	if(policy == 1) {//best fit
		for(; vp.first!=vp.second; ++vp.first) {
			Vertex v1 = *vp.first;
			testJob->g[v1].assigned_type = types-1;
		}
	}
	else if(policy == 2) {//worst fit
		for(; vp.first!=vp.second; ++vp.first) {
			Vertex v1 = *vp.first;
			testJob->g[v1].assigned_type = 0;
		}
	}
	else if(policy == 3) {//most efficient
		/*for(; vp.first!=vp.second; ++vp.first) {
			Vertex v1 = *vp.first;
			int max_type=0; double max=std::ceil(testJob->g[v1].estTime[0]/60.0)*priceOnDemand[0];
			for(int i=1; i<types; i++) {
				double tmp = std::ceil(testJob->g[v1].estTime[i]/60.0)*priceOnDemand[i];
				if(tmp<max) max_type = i;
			}
			testJob->g[v1].assigned_type = max_type;
		}*/
		//use autoscaling as input
		for(; vp.first!=vp.second; ++vp.first) {//edge weight for communication cost
			Vertex v1 = *vp.first;
			testJob->g[v1].prefer_type = types-1;
		}
			
		testJob->deadline_assign();
		//vp = vertices(testJob->g);
		//for(; vp.first !=vp.second; *vp.first ++)
		//	printf("LFT is %f, EST is %f\n",testJob->g[*vp.first].LFT,testJob->g[*vp.first].EST);
		//task configuration, find the prefered VM type for tasks
		vp = vertices(testJob->g);
		for(; vp.first != vp.second; ++vp.first)
			testJob->g[*vp.first].instance_config();
		vp = vertices(testJob->g);
		for(; vp.first != vp.second; ++vp.first)
			testJob->g[*vp.first].assigned_type = testJob->g[*vp.first].prefer_type;
	}
	
	//make sure the deadline, update start time and end time
	BFS_update(testJob);
	vp = vertices(testJob->g);
	Vertex vend = *(vp.second-1);
	double totalcost = 0;		
	for(; vp.first != vp.second; ++vp.first)
		totalcost += ceil(testJob->g[*vp.first].estTime[testJob->g[*vp.first].assigned_type]/60)*priceOnDemand[testJob->g[*vp.first].assigned_type];

	
	if(policy == 1) {
		if(!timeorcost){
			if(testJob->g[vend].end_time> testJob->deadline) {
				std::cout<<"deadline too tight to finish!"<<std::endl;
				return;
			}
		}
		else {
			//make sure cost less than budget
			double costopt=totalcost;
			while(costopt > testJob->budget){
				vp = vertices(testJob->g);
				int size_tasks = *vp.second - *vp.first - 1;
				int task1 = (double)rand() / (RAND_MAX+1) * size_tasks;	
				//std::cout<<task1<<", "<<task2;
				if(testJob->g[task1].assigned_type >0)
					testJob->g[task1].assigned_type -= 1;
				
				vp = vertices(testJob->g);
				costopt=0;
				for(; vp.first != vp.second; ++vp.first)
					costopt += ceil(testJob->g[*vp.first].estTime[testJob->g[*vp.first].assigned_type]/60.0)*priceOnDemand[testJob->g[*vp.first].assigned_type];
			}
			BFS_update(testJob);
		}
	}
	else if(policy == 2){//worst fit
		if(!timeorcost){
			//make sure of deadline
			while(testJob->g[vend].end_time> testJob->deadline) { //most gain to satisfy deadline			
				/* only change one task at a time, do not work for DAGs like Ligo
					because Ligo may have two parallel critical path*/
				/*vp = vertices(testJob->g);
				std::vector<double> gains;
				int size_tasks = *vp.second - *vp.first - 1;
				gains.resize(size_tasks);

				//promote VM type of the tasks on CP and find the most gain one
				for(; vp.first!=(vp.second-1); ++vp.first) {				
					if(testJob->g[*vp.first].assigned_type < types-1) {
						testJob->g[*vp.first].prefer_type = testJob->g[*vp.first].assigned_type;
						testJob->g[*vp.first].assigned_type += 1;
				
						double t_old = testJob->g[vend].end_time;
						double cost_old = std::ceil(testJob->g[*vp.first].estTime[testJob->g[*vp.first].prefer_type]/60.0) *
							priceOnDemand[testJob->g[*vp.first].prefer_type];
						BFS_update(testJob);
						double t_new = testJob->g[vend].end_time;
						double cost_new = std::ceil(testJob->g[*vp.first].estTime[testJob->g[*vp.first].assigned_type]/60.0) *
							priceOnDemand[testJob->g[*vp.first].assigned_type];
						if(t_new < t_old && cost_new != cost_old)
							gains[*vp.first]= (t_old - t_new)/(cost_new - cost_old);
						//only let one task change type at a time
						testJob->g[*vp.first].assigned_type -= 1;
					}
				}
				double max = gains[0]; int max_id = 0;
				for(int i=1; i<size_tasks; i++)	{
					if(gains[i] > max) {
						max_id = i;
						max = gains[i];
					}
				}
				testJob->g[max_id].assigned_type += 1;
				BFS_update(testJob);*/

				//change two tasks at the same time
				vp = vertices(testJob->g);
				int size_tasks = *vp.second - *vp.first - 1;
				int task1 = (double)rand() / (RAND_MAX+1) * size_tasks;
				int task2 = (double)rand() / (RAND_MAX+1) * size_tasks;
				//std::cout<<task1<<", "<<task2;
				if(testJob->g[task1].assigned_type<types-1 && testJob->g[task2].assigned_type<types-1&&task1!=task2) {
					testJob->g[task1].assigned_type += 1;
					testJob->g[task2].assigned_type += 1;
					double t_old = testJob->g[vend].end_time;
					BFS_update(testJob);
					double t_new = testJob->g[vend].end_time;
					if(!(t_new < t_old)) {
						testJob->g[task1].assigned_type -= 1;
						testJob->g[task2].assigned_type -= 1;
					}
				}
			}
		}
		else {
			//make sure of budget
			if(totalcost > testJob->budget )
				printf("budget is too low for worst-fit algorithm\n");
			exit(1);
		}
	}
	else{//most efficient
		if(timeorcost){
			//make sure cost less than budget			
			double costopt=totalcost;
			while(costopt > testJob->budget){
				testJob->deadline += 20;
				testJob->deadline_assign();
				vp = vertices(testJob->g);
				for(; vp.first != vp.second; ++vp.first){
					testJob->g[*vp.first].instance_config();
					testJob->g[*vp.first].assigned_type = testJob->g[*vp.first].prefer_type;
				}
				/*vp = vertices(testJob->g);
				int size_tasks = *vp.second - *vp.first - 1;
				int task1 = (double)rand() / (RAND_MAX+1) * size_tasks;	
				//std::cout<<task1<<", "<<task2;
				if(testJob->g[task1].assigned_type >0)
					testJob->g[task1].assigned_type -= 1;
				*/

				vp = vertices(testJob->g);
				costopt=0;
				for(; vp.first != vp.second; ++vp.first)
					costopt += ceil(testJob->g[*vp.first].estTime[testJob->g[*vp.first].assigned_type]/60)*priceOnDemand[testJob->g[*vp.first].assigned_type];
			}
			BFS_update(testJob);
		}else{
			//make sure of deadline
			while(testJob->g[vend].end_time> testJob->deadline) { //most gain to satisfy deadline			
				//change two tasks at the same time
				vp = vertices(testJob->g);
				int size_tasks = *vp.second - *vp.first - 1;
				int task1 = (double)rand() / (RAND_MAX+1) * size_tasks;
				int task2 = (double)rand() / (RAND_MAX+1) * size_tasks;
				//std::cout<<task1<<", "<<task2;
				if(testJob->g[task1].assigned_type<types-1 && testJob->g[task2].assigned_type<types-1&&task1!=task2) {
					testJob->g[task1].assigned_type += 1;
					testJob->g[task2].assigned_type += 1;
					double t_old = testJob->g[vend].end_time;
					BFS_update(testJob);
					double t_new = testJob->g[vend].end_time;
					if(!(t_new < t_old)) {
						testJob->g[task1].assigned_type -= 1;
						testJob->g[task2].assigned_type -= 1;
					}
				}
			}
		}
	}
	/*vp = vertices(testJob->g);
	for(; vp.first !=vp.second; *vp.first ++)
		printf("assigned type is: %d\n",testJob->g[*vp.first].assigned_type);*/
}
//jobs are currently waiting in the queue for planning
vector<vector<pair<double, double> > > GanttConsolidation::Planner(std::vector<Job*> jobs, int timer, bool rule, bool estimate, bool timeorcost, vector<VM*>* VM_queue, int* operationcount)
{
	vector<vector<pair<double, double> > > VM_requests(types); //records the vm type with pair<starttime, endtime>
	if(jobs.size() == 0){
		/*pair<double,pair<double,double> > result;
		result.first = result.second.first = result.second.second = 0;
		return result;*/
		return VM_requests;
	}
	int count = 0;
	double cost = 0;
	//the number of deadline violations in this plan
	double violate = 0;
	double totaltime = 0;
	//pair<double, pair<double,double> > result;
	//construct a priority queue to store VMs that hourly covers tasks' requests
	//vector<VM*> VM_queue[types];	
	//pair<vertex_iter, vertex_iter> vp; 
	////std::priority_queue<VM*> VM_pque;
	//for(int i=0; i<jobs.size(); i++) {		
	////	vector<taskVertex*> alltasks;
	//	vp = vertices(jobs[i]->g);
	//	for(; vp.first!=vp.second; ++vp.first) {
	//		if(jobs[i]->g[*vp.first].start_time!=jobs[i]->g[*vp.first].end_time){
	//			VM* vm = new VM();
	//			/*int pointersize = sizeof(VM);
	//			int pointersize2 = sizeof(taskVertex);*/
	//			vm->type = jobs[i]->g[*vp.first ].assigned_type;
	//			vector<taskVertex*> cotasks;
	//			cotasks.push_back(&jobs[i]->g[*vp.first]);
	//			vm->assigned_tasks.push_back(cotasks);
	//			//vm->assigned_tasks.push_back(&jobs[i]->g[*vp.first]);
	//			vm->start_time = jobs[i]->g[*vp.first ].start_time;
	//			vm->end_time = jobs[i]->g[*vp.first ].end_time;
	//			vm->life_time = std::ceil((jobs[i]->g[*vp.first ].end_time-jobs[i]->g[*vp.first ].start_time)/60)*60;			
	//			vm->resi_time = vm->life_time +vm->start_time - vm->end_time;				
	//			
	//			//vm->dependentVMs.insert();
	//			//vm->price = priceOnDemand[vm->type];
	//			VM_queue[vm->type].push_back(vm);
	//		}
	//	}
	//}

	int num_vms = 0;
	double originalcost = 0;
	//define capacity of each type of VMs
	for(int i=0; i<types; i++) {
		for(int j=0; j<VM_queue[i].size(); j++) {
			VM_queue[i][j]->capacity = std::pow(2.0,i) - 1; //already 1 in the vm
			num_vms += 1;
			originalcost += priceOnDemand[i]*VM_queue[i][j]->life_time/60.0;
		}
	}	
	if(true){
		do{	
			//sort according to the start time of each VM
			//for(int i=0; i<types; i++) 
				//std::sort(VM_queue[i].begin(), VM_queue[i].end(),vmfunction);
			double savethisround=0;
			//start the operators 
			//if this condition, do move or split
			double savemove = 0;
			double savedemote = 0;
			double savepromote = 0;
			double savemerge = 0;
			double savesplit = 0;
			double savecoschedule = 0;
				

			if(false){
				//try the 5 operations with rule				
				savemove = opMove(VM_queue,jobs,true,estimate,timeorcost);
				savepromote = opPromote(VM_queue,jobs,true,estimate,timeorcost);
				savedemote = opDemote(VM_queue,jobs,true,estimate,timeorcost);
				savemerge = opMerge(VM_queue,jobs,true,estimate,timeorcost);
				savesplit = opSplit(VM_queue, jobs,true,estimate,timeorcost);
				//savecoschedule = opCoschedule(VM_queue, jobs,true,estimate,timeorcost);

				if(savemove>=savepromote && savemove>=savedemote && savemove>=savemerge && savemove>=savesplit && savemove>= savecoschedule){
					savethisround = opMove(VM_queue,jobs,false,estimate,timeorcost);
					printf("real save is: %4f\n",savethisround);
					printf("estimated save is: %4f\n", savemove);
				}
				else if(savepromote>=savemove && savepromote>=savedemote && savepromote>=savemerge && savepromote>=savesplit && savepromote>= savecoschedule){
					savethisround = opPromote(VM_queue,jobs,false,estimate,timeorcost);
					printf("real save is: %4f\n",savethisround);
					printf("estimated save is: %4f\n", savepromote);

				}
				else if(savedemote>=savemove && savedemote>=savepromote && savedemote>=savemerge && savedemote>=savesplit && savedemote>= savecoschedule){
					savethisround = opDemote(VM_queue,jobs,false,estimate,timeorcost);				
					printf("real save is: %4f\n",savethisround);
					printf("estimated save is: %4f\n", savedemote);

				}
				else if(savemerge>=savemove && savemerge>=savepromote && savemerge>=savedemote && savemerge>=savesplit && savemerge>= savecoschedule){
					savethisround = opMerge(VM_queue,jobs,false,estimate,timeorcost);				
					printf("real save is: %4f\n",savethisround);
					printf("estimated save is: %4f\n", savemerge);

				}
				else if(savesplit>=savemove && savesplit>=savepromote && savesplit>=savedemote && savesplit>=savemerge && savesplit>= savecoschedule){
					savethisround = opSplit(VM_queue,jobs,false,estimate,timeorcost);	
					printf("real save is: %4f\n",savethisround);
					printf("estimated save is: %4f\n", savesplit);

				}else if(savecoschedule>=savemove && savecoschedule>=savepromote && savecoschedule>=savedemote && savecoschedule>= savemerge &&savecoschedule>=savesplit){
					//savethisround = opCoschedule(VM_queue,jobs,false,estimate,timeorcost);
				}
			}
			if(rule){
				if(timeorcost){
					savepromote = opPromote(VM_queue,jobs,true,estimate, timeorcost);
					if(savepromote > 0){
						savethisround = opPromote(VM_queue,jobs,false,estimate,timeorcost);
						operationcount[3]++;
					}
					savemove = opMove(VM_queue,jobs,true,estimate,timeorcost);
					savedemote = opDemote(VM_queue,jobs,true,estimate,timeorcost);
					savemerge = opMerge(VM_queue,jobs,true,estimate,timeorcost);
					savesplit = opSplit(VM_queue, jobs,true,estimate,timeorcost);
					//savecoschedule = opCoschedule(VM_queue, jobs,true,estimate,timeorcost);

					if(savemove>=savedemote && savemove>=savemerge && savemove>=savesplit && savemove>= savecoschedule){
						savethisround += opMove(VM_queue,jobs,false,estimate,timeorcost);
						operationcount[2]++;
					}
					else if(savedemote>=savemove && savedemote>=savemerge && savedemote>=savesplit && savedemote>= savecoschedule){
						savethisround += opDemote(VM_queue,jobs,false,estimate,timeorcost);				
						operationcount[1]++;
					}
					else if(savemerge>=savemove && savemerge>=savedemote && savemerge>=savesplit && savemerge>= savecoschedule){
						savethisround += opMerge(VM_queue,jobs,false,estimate,timeorcost);				
						operationcount[0]++;
					}
					else if(savesplit>=savemove && savesplit>=savedemote && savesplit>=savemerge && savesplit>= savecoschedule){
						savethisround += opSplit(VM_queue,jobs,false,estimate,timeorcost);	
						operationcount[4]++;
					}

				}else{
					savedemote = opDemote(VM_queue,jobs,true,estimate,timeorcost);
					savemerge = opMerge(VM_queue,jobs,true,estimate,timeorcost);

					if(savedemote>=savemerge){
					   savethisround = opDemote(VM_queue,jobs,false,estimate,timeorcost);
					   operationcount[1]++;
					}
					else {
					   savethisround = opMerge(VM_queue,jobs,false,estimate,timeorcost);
					   operationcount[0]++;
					}

					savesplit = opSplit(VM_queue, jobs,true,estimate,timeorcost);
				 //   savecoschedule = opCoschedule(VM_queue, jobs,true,estimate,timeorcost);
					savemove =  opMove(VM_queue,jobs,true,estimate,timeorcost);
					savepromote = opPromote(VM_queue,jobs,true,estimate,timeorcost);

					if(savemove>=savepromote && savemove>=savesplit && savemove>= savecoschedule ){
							savethisround += opMove(VM_queue,jobs,false,estimate,timeorcost);
							operationcount[2]++;
					}
					else if(savepromote>=savemove && savepromote>=savesplit && savepromote>= savecoschedule){
							savethisround += opPromote(VM_queue,jobs,false,estimate,timeorcost);
							operationcount[3]++;
					}
					else if(savesplit>=savemove && savesplit>=savepromote && savesplit>= savecoschedule ){
							savethisround += opSplit(VM_queue,jobs,false,estimate,timeorcost);
							operationcount[4]++;
					}else if(savecoschedule>=savemove && savecoschedule>=savepromote && savecoschedule>=savesplit){
						  //  savethisround += opCoschedule(VM_queue,jobs,false,estimate,timeorcost);					
					}
				}
			}
			else{
				////do the five operators randomly		
				//int select = (double)rand() / (RAND_MAX+1) * 5;
				//if(select==0)
				//	savethisround = opMove(VM_queue,jobs,false,estimate,timeorcost);
				//else if(select==1)
				//	savethisround = opPromote(VM_queue,jobs,false,estimate,timeorcost);
				//else if(select ==2)
				//	savethisround = opDemote(VM_queue,jobs,false,estimate,timeorcost);
				//else if(select == 3)
				//	savethisround = opMerge(VM_queue,jobs,false,estimate,timeorcost);
				//else if(select == 4)
				//	savethisround = opSplit(VM_queue,jobs,false,estimate,timeorcost);
				int select1 = (double)rand() /(RAND_MAX+1) * 2;
				if(select1 == 0)
					savethisround = opMerge(VM_queue,jobs,false,estimate,timeorcost);
				if(select1 == 1)
					savethisround = opDemote(VM_queue,jobs,false,estimate,timeorcost);

				int select2 = (double)rand() / (RAND_MAX+1) * 4;
				if(select2==0)
					savethisround += opMove(VM_queue,jobs,false,estimate,timeorcost);
				else if(select2==1)
					savethisround += opPromote(VM_queue,jobs,false,estimate,timeorcost);
				else if(select2 ==2)
					savethisround += opSplit(VM_queue,jobs,false,estimate,timeorcost);
				else if(select2 == 3)
					savethisround += opCoschedule(VM_queue,jobs,false,estimate,timeorcost);
			}
			//savethisround = savemove+savedemote+savepromote+savemerge+savemove;
			if(savethisround <= 0) count++;

			//cost saved in this iteration
			double iteratecost = 0;
			for(int i=0; i<types; i++)
				for(int j=0; j<VM_queue[i].size(); j++)
					iteratecost += priceOnDemand[VM_queue[i][j]->type]*VM_queue[i][j]->life_time/60.0;
					//iteratecost+= priceOnDemand[i]*(VM_queue[i][j]->end_time - VM_queue[i][j]->start_time)/60.0;

			double costsaved = originalcost - iteratecost;
			printf("cost saved in this iteration is: %4f\n", savethisround);
			if(costsaved < 0)
				printf("cost < 0, stop here\n");
			originalcost = iteratecost;
		}while(count<1);

		//at the end, merge everything
		for(int i=0; i<types; i++){
			for(int k=0; k<VM_queue[i].size(); k++){
				for(int j=0; j<VM_queue[i].size(); j++)
				double starttime = VM_queue[i][k]->start_time;
			}
		}
	}
	for(int i=0; i<types; i++) {
		int sizei = VM_queue[i].size();
		for(int j=0; j<sizei; j++){
			pair<double, double> request;
			request.first = VM_queue[i][j]->start_time;
			request.second = VM_queue[i][j]->end_time;
			VM_requests[i].push_back(request);
		}
	}
	if(false){
	//for(int i=0; i<types; i++)
	//	for(int j=0; j<VM_queue[i].size(); j++)
	//		cost += priceOnDemand[i]*VM_queue[i][j]->life_time/60.0;

	//for(int i=0; i<types; i++){
	//	for(int j=0; j<VM_queue[i].size(); j++){
	//		//for(int out=0;out<VM_queue[i][j]->assigned_tasks.size(); out++)
	//		//	for(int in=0; in<VM_queue[i][j]->assigned_tasks[out].size(); in++){
	//		//		/*delete VM_queue[i][j]->assigned_tasks[out][in];
	//		//		VM_queue[i][j]->assigned_tasks[out][in] = NULL;*/
	//		//		VM_queue[i][j]->assigned_tasks[out].pop_back();
	//		//		in--;
	//		//	}
	//		VM_queue[i][j]->assigned_tasks.clear();
	//		delete VM_queue[i][j];
	//		VM_queue[i][j] = NULL;
	//	}
	//	VM_queue[i].clear();
	//}

	//return cost;//result;
	}
	return VM_requests;
}
void GanttConsolidation::Simulate(Job testJob, Job testJob1, int input, int period, bool rule, bool estimate, bool timeorcost,bool baseline)
{
	std::vector<Job*> workflows; //continuous workflow
	double arrival_time = 0;
	Job* job = new Job(pipeline, deadline, lamda);
	Job* job1 = new Job(pipeline, deadline, lamda);
	if(testJob1.deadline != 0)
	{
		job1->g = testJob1.g;
		job1->type = testJob1.type;
		job1->budget = testJob1.budget;
		job1->arrival_time = 0;
		workflows.push_back(job1);
	}
	job->g = testJob.g;
	job->type = testJob.type;
	job->budget = testJob.budget;
	job->arrival_time = 0;
	workflows.push_back(job);
	double t = 0; bool condition = false;
	double moneycost = 0;
	std::ifstream infile;
	std::string a = "arrivaltime_integer_", b, c = ".txt";
	std::ostringstream strlamda;
	strlamda << lamda;
	b = strlamda.str();
	std::string fname = a + b + c;
	infile.open(fname.c_str());
	char times[256];
	if(infile==NULL){
		printf("cannot find input file!\n");
		return;
	}
	infile.getline(times,256); //jump the lamda line
	infile.getline(times,256); //jump the 0 line
	//incomming jobs, read the first 100 workflows

	while(workflows.size()<(int)max_t){
		infile.getline(times,256);
		arrival_time = atof(times);

		Job* job = new Job(pipeline,deadline+arrival_time,lamda);
		job->g = testJob.g;
		job->budget = testJob.budget;
		job->arrival_time = arrival_time;
		//std::pair<vertex_iter, vertex_iter> vp = vertices(job->g);
		workflows.push_back(job);
		if(testJob1.deadline != 0){
			Job* job1 = new Job(pipeline,deadline+arrival_time,lamda);
			job1->g = testJob1.g;
			job1->budget = testJob1.budget;
			job1->arrival_time = arrival_time;
			workflows.push_back(job1);
		}
	}
	//while(arrival_time < max_t){
	//	infile.getline(time,256);
	//	arrival_time = atof(time);

	//	Job* job = new Job(pipeline,deadline+arrival_time,lamda);
	//	job->g = testJob.g;
	//	job->budget = testJob.budget;
	//	job->arrival_time = arrival_time;
	//	//std::pair<vertex_iter, vertex_iter> vp = vertices(job->g);
	//	workflows.push_back(job);
	//}

	//start simulation
	int pointer = 0;
	//int period = 60; //planning every an hour
	//int threshold = 10; //when more than thrshold times no gain, stop the operations
	int timer = 0;
	double totalcost = 0;
	int violate = 0;
	double totaltime = 0;
	//maintain two vm queues for simulation
	vector<VM> VM_pool[types];//use its in_use time, start time, end time
	//vector<pair<double, pair<double, double> > > VM_pool[types];//first is useful time;first is lifetime, second is the time for 
	vector<pair<double, double> > VM_request[types];
	double* numvms = new double[types];
	double* utilizations = new double[types];
	double* totaltimes = new double[types];
	int counts[5] = {0,0,0,0,0};//merge, demote, move, promote, split
	for(int i=0; i<types; i++)
		utilizations[i]=totaltimes[i]=numvms[i]=0;
	bool stopcondition = false;
	while(!stopcondition) {
		stopcondition = true;
		std::vector<Job*> jobs;
		//pop all jobs waiting and start planning
		//if(baseline) period = 1;		
		
		if(timer%period == 0 ) {
			//printf("pointer:%d, arrive_time:%f\n", pointer, workflows[pointer]->arrival_time);
			for(int i=pointer; i<workflows.size(); i++) {
				//printf("i is:%d ",i);
				if(workflows[i]->arrival_time <= timer){
					jobs.push_back(workflows[i]);
					if(i==(workflows.size()-1))
						pointer=workflows.size();
				}
				else {pointer = i; break;}
			}
			srand(time(NULL));
			for(int i=0; i<jobs.size(); i++){				
				jobs[i]->deadline -= timer;
				Initialization(jobs[i],input,timeorcost);
				jobs[i]->deadline += timer;				
				jobs[i]->g[0].status = scheduled;
			}
					
			//for tasks to refer to the job they belong to 
			for(int i=0; i<jobs.size(); i++) {
				jobs[i]->name = i;
				std::pair<vertex_iter, vertex_iter> vp = vertices(jobs[i]->g);
				for(; vp.first != vp.second; ++vp.first) {
					jobs[i]->g[*vp.first].job_id = i;
					//all jobs in the queue start at the same time when it's later than their arrival time
					jobs[i]->g[*vp.first].start_time += timer; //jobs[i]->arrival_time;
					jobs[i]->g[*vp.first].end_time += timer; //jobs[i]->arrival_time;
					jobs[i]->g[*vp.first].sub_deadline = jobs[i]->deadline;
				}
			}
			printf("current time is: %d\n", timer);

			/*pair<double, pair<double,double> >cost = Planner(jobs, timer);
			totalcost += cost.first;
			violate += cost.second.first;
			totaltime += cost.second.second;*/
			/*unsigned seed = timer;
			srand( (unsigned)time( NULL ) );*/
			vector<VM*> VM_queue[types];	
			pair<vertex_iter, vertex_iter> vp; 
			//std::priority_queue<VM*> VM_pque;
			for(int i=0; i<jobs.size(); i++) {		
			//	vector<taskVertex*> alltasks;
				vp = vertices(jobs[i]->g);
				for(; vp.first!=vp.second; ++vp.first) {
					if(jobs[i]->g[*vp.first].start_time!=jobs[i]->g[*vp.first].end_time){
						VM* vm = new VM();
						/*int pointersize = sizeof(VM);
						int pointersize2 = sizeof(taskVertex);*/
						vm->type = jobs[i]->g[*vp.first ].assigned_type;
						vector<taskVertex*> cotasks;
						cotasks.push_back(&jobs[i]->g[*vp.first]);
						vm->assigned_tasks.push_back(cotasks);
						//vm->assigned_tasks.push_back(&jobs[i]->g[*vp.first]);
						vm->start_time = jobs[i]->g[*vp.first ].start_time;
						vm->end_time = jobs[i]->g[*vp.first ].end_time+OnDemandLag;
						vm->life_time = std::ceil((jobs[i]->g[*vp.first ].end_time-jobs[i]->g[*vp.first ].start_time)/60)*60;			
						vm->resi_time = vm->life_time +vm->start_time - vm->end_time;				
				
						//vm->dependentVMs.insert();
						//vm->price = priceOnDemand[vm->type];
						VM_queue[vm->type].push_back(vm);
					}
				}
			}
			if(baseline){
				for(int i=0; i<types; i++) {
					int sizei = VM_queue[i].size();
					for(int j=0; j<sizei; j++){
						pair<double, double> request;
						request.first = VM_queue[i][j]->start_time;
						request.second = VM_queue[i][j]->end_time;
						VM_request[i].push_back(request);
					}
				}
			}else{
				vector<vector<pair<double, double> > > planrequests;
				planrequests = Planner(jobs, timer, rule, estimate,timeorcost, VM_queue, counts);
				for(int i=0; i<types; i++){
					while(planrequests[i].size() > 0){
						pair<double, double> request = planrequests[i].back();
						planrequests[i].pop_back();
						VM_request[i].push_back(request);
					}
				}
			}
			//clean up
			for(int i=0; i<types; i++){
				for(int j=0; j<VM_queue[i].size(); j++){
					VM_queue[i][j]->assigned_tasks.clear();
					delete VM_queue[i][j];
					VM_queue[i][j] = NULL;
				}
				VM_queue[i].clear();
			}
			//totalcost += cost;
		}
		//after planning, requesting VMs
		for(int i=0; i<types; i++){
			for(int j=0; j<VM_request[i].size(); j++){
				if((int)VM_request[i][j].first == timer){//scheduled to execute
					bool found = false; //if found a existing vm to execute on
					for(int k=0; k<VM_pool[i].size(); k++){//look into the VM pool
						//if a vm has partial hour
						//if((int)VM_pool[i][k].second.second <= timer){
						if((int)VM_pool[i][k].end_time <= timer ){
							found = true;
							/*VM_pool[i][k].second.first += VM_request[i][j].second - VM_request[i][j].first;
							VM_pool[i][k].first += VM_request[i][j].second - VM_request[i][j].first;
							VM_pool[i][k].second.second = VM_request[i][j].second;*/
							VM_pool[i][k].end_time += VM_request[i][j].second - VM_request[i][j].first;
							VM_pool[i][k].in_use += VM_request[i][j].second - VM_request[i][j].first;
							break;
						}
					}
					if(!found){
						//request a new VM
						//pair<double, pair<double, double> > newvm;
						//newvm.second.second = VM_request[i][j].second;
						//newvm.first = newvm.second.first = VM_request[i][j].second - VM_request[i][j].first + OnDemandLag;
						//VM_pool[i].push_back(newvm);
						VM newvm;
						newvm.start_time = VM_request[i][j].first;
						newvm.end_time = VM_request[i][j].second + OnDemandLag;
						newvm.in_use += VM_request[i][j].second - VM_request[i][j].first;
						VM_pool[i].push_back(newvm);
					}
				
					VM_request[i].erase(VM_request[i].begin()+j);
					j--;
				}
			}
		}		
		//finishing one time step
		for(int i=0; i<types; i++)
			for(int j=0; j<VM_pool[i].size(); j++){
				//if(VM_pool[i][j].second.second <= timer){
				if(VM_pool[i][j].end_time <= timer){					
					//if((int)VM_pool[i][j].second.first % 60 == 0){
					if((int)(VM_pool[i][j].end_time - VM_pool[i][j].start_time) % 60 == 0){
						totalcost += priceOnDemand[i]* (VM_pool[i][j].end_time-VM_pool[i][j].start_time) / 60.0;
						utilizations[i] += VM_pool[i][j].in_use;
						totaltimes[i] += VM_pool[i][j].end_time - VM_pool[i][j].start_time;
						numvms[i]++;
						VM_pool[i].erase(VM_pool[i].begin()+j);
						j--;
					}else{
						//VM_pool[i][j].second.first++;
						//VM_pool[i][j].second.second++;
						VM_pool[i][j].end_time ++;
					}
				}
			}
		for(int i=0; i<workflows.size(); i++){
			std::pair<vertex_iter, vertex_iter> vp;
			vp = vertices(workflows[i]->g);
			int vend = *(vp.second -1);
			in_edge_iterator in_i, in_end;
			edge_descriptor e;
			double endtime=0;
			for (boost::tie(in_i, in_end) = in_edges(vend, workflows[i]->g); in_i != in_end; ++in_i) {
				e = *in_i;
				Vertex src = source(e, workflows[i]->g);					
				if(workflows[i]->g[src].end_time > endtime)	{
					endtime = workflows[i]->g[src].end_time;
				}
			}
			if(workflows[i]->g[0].status == not_ready || endtime > timer){
				stopcondition = false;
				break;
			}
		}
		timer ++;
	}
	//the end of simulation
	//reporting the num of VMs and utilization
	for(int i=0; i<types; i++){
		for(int j=0; j<VM_pool[i].size();j++){
			utilizations[i] += VM_pool[i][j].in_use;
			totaltimes[i] += ceil((VM_pool[i][j].end_time-VM_pool[i][j].start_time)/60.0)*60.0;
			numvms[i]++;
			totalcost += priceOnDemand[i]*ceil((VM_pool[i][j].end_time-VM_pool[i][j].start_time)/60.0);
		}
		utilizations[i] /= totaltimes[i];
		printf("total number of vm type %d is %f,utilization is %f, total lifetime is %f\n",i,numvms[i],utilizations[i], totaltimes[i]);
	}
	printf("number of merge operations:%d\n",counts[0]);
	printf("number of demote operations:%d\n",counts[1]);
	printf("number of move operations:%d\n",counts[2]);
	printf("number of promote operations:%d\n",counts[3]);
	printf("number of split operations:%d\n",counts[4]);

	//calculate the deadline violation rate and average execution time	
	for(int i=0; i<workflows.size(); i++) {
		std::pair<vertex_iter, vertex_iter> vp;
		vp = vertices(workflows[i]->g);
		int vend = *(vp.second -1);
		in_edge_iterator in_i, in_end;
		edge_descriptor e;
		double endtime=0;
		for (boost::tie(in_i, in_end) = in_edges(vend, workflows[i]->g); in_i != in_end; ++in_i) 
		{
			e = *in_i;
			Vertex src = source(e, workflows[i]->g);					
			if(workflows[i]->g[src].end_time > endtime)
			{
				endtime = workflows[i]->g[src].end_time;
			}
		}
		workflows[i]->g[vend].end_time = workflows[i]->g[vend].start_time = endtime;
		if(workflows[i]->g[vend].end_time > workflows[i]->deadline)
			violate += 1;
		totaltime += (workflows[i]->g[vend].end_time - workflows[i]->g[0].start_time); //workflows[i]->arrival_time);
	}
	double averagetime = (double)totaltime/workflows.size();
	double violate_rate = (double)violate/workflows.size();
	printf("deadline violation rate is: %4f\n",violate_rate);
	printf("total cost is: %4f\n",totalcost);
	printf("average execution time is: %4f\n",averagetime);
}