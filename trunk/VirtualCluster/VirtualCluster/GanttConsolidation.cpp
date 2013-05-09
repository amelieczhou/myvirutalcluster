#include "stdafx.h"
#include "ConsolidateOperators.h"
#include "Algorithms.h"
#include <fstream>
#include <string>
#include <sstream>
//#include <boost/graph/topological_sort.hpp>

extern double max_t;
extern bool DynamicSchedule;
extern double deadline;
extern double chk_period;
extern bool sharing;
extern bool on_off;
extern double lamda;
extern double QoS;

void GanttConsolidation::Initialization(Job* testJob, int policy)
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
		for(; vp.first!=vp.second; ++vp.first) //edge weight for communication cost
		{
			Vertex v1 = *vp.first;
			testJob->g[v1].prefer_type = types-1;
		}
			
		testJob->deadline_assign();

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
	if(policy == 1) {
		if(testJob->g[vend].end_time> testJob->deadline) {
			std::cout<<"deadline too tight to finish!"<<std::endl;
			return;
		}
	}
	else {
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
			if(testJob->g[task1].assigned_type<types-1 && testJob->g[task2].assigned_type<types-1) {
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
//jobs are currently waiting in the queue for planning
double GanttConsolidation::Planner(std::vector<Job*> jobs, int timer)
{
	if(jobs.size() == 0){
		/*pair<double,pair<double,double> > result;
		result.first = result.second.first = result.second.second = 0;
		return result;*/
		return 0;
	}
	int count = 0;
	double cost = 0;
	//the number of deadline violations in this plan
	double violate = 0;
	double totaltime = 0;
	//pair<double, pair<double,double> > result;
	//construct a priority queue to store VMs that hourly covers tasks' requests
	vector<VM*> VM_queue[types];	
	pair<vertex_iter, vertex_iter> vp; 
	//std::priority_queue<VM*> VM_pque;
	for(int i=0; i<jobs.size(); i++) {		
	//	vector<taskVertex*> alltasks;
		vp = vertices(jobs[i]->g);
	//	for(; vp.first!=vp.second; ++vp.first)
	//		alltasks.push_back(&jobs[i]->g[*vp.first]);
	//	for(int j=0; j<alltasks.size(); j++){
	//		//find the children of all tasks
	//		//get children vertices
	//		out_edge_iterator out_i, out_end;
	//		edge_descriptor e;
	//		for (boost::tie(out_i, out_end) = out_edges(jobs[i]->g[j].name, jobs[i]->g); out_i != out_end; ++out_i) 
	//		{
	//			e = *out_i;
	//			Vertex tgt = target(e,jobs[i]->g);				
	//			alltasks[j]->children.push_back(alltasks[tgt]);
	//		}
	//	}
	//	for(int j=0; j<alltasks.size(); j++){
	//		if(alltasks[j]->start_time!=alltasks[j]->end_time){
	//			VM* vm = new VM();
	//			vm->type = alltasks[j]->assigned_type;
	//			vector<taskVertex*> cotasks;
	//			cotasks.push_back(alltasks[j]);
	//			vm->assigned_tasks.push_back(cotasks);
	//			vm->start_time = alltasks[j]->start_time;
	//			vm->life_time = ceil((alltasks[j]->end_time - alltasks[j]->start_time)/60)*60;			
	//			vm->resi_time = vm->life_time - (alltasks[j]->end_time - alltasks[j]->start_time);
	//			vm->end_time = vm->start_time + vm->life_time - vm->resi_time;
	//			VM_queue[vm->type].push_back(vm);
	//		}
	//	}
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
				vm->end_time = jobs[i]->g[*vp.first ].end_time;
				vm->life_time = std::ceil((jobs[i]->g[*vp.first ].end_time-jobs[i]->g[*vp.first ].start_time)/60)*60;			
				vm->resi_time = vm->life_time +vm->start_time - vm->end_time;				
				
				//vm->dependentVMs.insert();
				//vm->price = priceOnDemand[vm->type];
				VM_queue[vm->type].push_back(vm);
			}
		}
	}

	/*for(int i=0; i<types; i++)
		for(int j=0; j<VM_queue[i].size(); j++)
		{
			delete VM_queue[i][j];
		}*/
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
	double savethisround=0;
	do{	
		//sort according to the start time of each VM
		for(int i=0; i<types; i++) 
			std::sort(VM_queue[i].begin(), VM_queue[i].end(),vmfunction);
		
		//start the operators 
		//if this condition, do move or split
		double savemove = 0;
		double savedemote = 0;
		double savepromote = 0;
		double savemerge = 0;
		double mergecount = 0;
		double demotecount = 0;
		double movecount = 0;
		double promotecount = 0;
		/*while(demotecount<10){
			double save=opDemote(VM_queue,jobs);
			savedemote += save;
			if(save==0) demotecount++;
		}
		while(movecount<10){
			double save=opMove(VM_queue,jobs);
			savemove += save;
			if(save==0) movecount++;
		}*/
		//savemove = opMove(VM_queue,jobs);
		//savepromote = opPromote(VM_queue,jobs);
		//savedemote = opDemote(VM_queue,jobs);
		savemerge = opMerge(VM_queue,jobs);
		//the number of VMs saved by move
		/*int num_vms_move = 0;
		for(int i=0; i<types; i++)
			for(int j=0; j<VM_queue[i].size(); j++)
				if(!VM_queue[i][j]->assigned_tasks.empty()) num_vms_move += 1;
		int num_vms_saved = num_vms - num_vms_move;*/
		//printf("The overall number of VMs saved by move operation is %d\n",num_vms_saved);

		
		savethisround = savemove+savedemote+savepromote+savemerge;
		if(savethisround <= 0) count++;

		//cost saved in this iteration
		double iteratecost = 0;
		for(int i=0; i<types; i++)
			for(int j=0; j<VM_queue[i].size(); j++)
				iteratecost += priceOnDemand[i]*VM_queue[i][j]->life_time/60.0;

		double costsaved = originalcost - iteratecost;
		printf("cost saved in this iteration is: %4f\n", costsaved);
		if(costsaved < 0)
			printf("cost < 0, stop here\n");
		originalcost = iteratecost;
	}while(count<2);//threshold);
	for(int i=0; i<types; i++)
		for(int j=0; j<VM_queue[i].size(); j++)
			cost += priceOnDemand[i]*VM_queue[i][j]->life_time/60.0;

	/*vp = vertices(jobs[0]->g);
	int numtasks = *vp.second - *vp.first -2;
	for(int i=0; i< types; i++)
		for(int j=0; j<VM_queue[i].size(); j++)
			for(int out=0; out<VM_queue[i][j]->assigned_tasks.size(); out++)
				for(int in=0; in<VM_queue[i][j]->assigned_tasks[out].size(); in++)
					if(VM_queue[i][j]->assigned_tasks[out][in]->name == numtasks){
						if(VM_queue[i][j]->assigned_tasks[out][in]->end_time > VM_queue[i][j]->assigned_tasks[out][in]->sub_deadline)
							violate += 1;
						totaltime += VM_queue[i][j]->assigned_tasks[out][in]->end_time;
					}*/

	/*result.first= cost;
	result.second.first = violate;
	result.second.second = totaltime;*/
	for(int i=0; i<types; i++){
		for(int j=0; j<VM_queue[i].size(); j++){
			//for(int out=0;out<VM_queue[i][j]->assigned_tasks.size(); out++)
			//	for(int in=0; in<VM_queue[i][j]->assigned_tasks[out].size(); in++){
			//		/*delete VM_queue[i][j]->assigned_tasks[out][in];
			//		VM_queue[i][j]->assigned_tasks[out][in] = NULL;*/
			//		VM_queue[i][j]->assigned_tasks[out].pop_back();
			//		in--;
			//	}
			VM_queue[i][j]->assigned_tasks.clear();
			delete VM_queue[i][j];
			VM_queue[i][j] = NULL;
		}
		VM_queue[i].clear();
	}

	return cost;//result;
}
void GanttConsolidation::Simulate(Job testJob, int input)
{
	std::vector<Job*> workflows; //continuous workflow
	double arrival_time = 0;
	Job* job = new Job(pipeline, deadline, lamda);
	job->g = testJob.g;
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
	char time[256];
	infile.open(fname.c_str());
	infile.getline(time,256); //jump the lamda line
	infile.getline(time,256); //jump the 0 line
	//incomming jobs
	while(arrival_time < max_t){
		infile.getline(time,256);
		arrival_time = atof(time);

		Job* job = new Job(pipeline,deadline+arrival_time,lamda);
		job->g = testJob.g;
		job->arrival_time = arrival_time;
		//std::pair<vertex_iter, vertex_iter> vp = vertices(job->g);
		workflows.push_back(job);
	}

	//start simulation
	int pointer = 0;
	int period = 60; //planning every an hour
	int threshold = 10; //when more than thrshold times no gain, stop the operations
	int timer = 0;
	double totalcost = 0;
	int violate = 0;
	double totaltime = 0;
	while(timer < max_t) {
		std::vector<Job*> jobs;
		//pop all jobs waiting and start planning
		if(timer%period == 0) {
			for(int i=pointer; i<workflows.size(); i++) {
				if(workflows[i]->arrival_time < timer)
					jobs.push_back(workflows[i]);
				else {pointer = i; break;}
			}
			for(int i=0; i<jobs.size(); i++){
				jobs[i]->deadline -= timer;
				Initialization(jobs[i],input);
				jobs[i]->deadline += timer;
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
			double cost = Planner(jobs, timer);
			totalcost += cost;
		}
		timer ++;
	}
	//the end of simulation
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