#include "stdafx.h"
#include "Provision.h"
#include "InstanceConfig.h"
#include "Algorithms.h"
#include <fstream>
#include <string>
#include <sstream>

extern double max_t;
extern bool DynamicSchedule;
extern double deadline;
extern double chk_period;
extern bool sharing;
extern bool on_off;
extern double lamda;
extern double QoS;

void VirtualCluster::Simulate(Job testJob){
	//offline determine the server groups
	int* VMs = testJob.find_ServerGroup();
	for(int i=0; i<types; i++)
		printf("%d, ",VMs[i]);
	//scheduling strategy to determine the serving rate of server groups, miu = 1/t
	//where t is the execution time of a job on a single server group
	double miu = 0, t = 0;
	bool condition = false;
	std::vector<VM*> VMTP[types];
	for(int i=0; i<types; i++)
	{
		for(int j=0; j<VMs[i]; j++)
			VMTP[i].push_back(new VM());
	}
	testJob.g[0].status = ready;
	std::pair<vertex_iter, vertex_iter> vp; 
	do{
		//step1: get all the ready tasks
		std::vector<taskVertex*> ready_task;
		vp = vertices(testJob.g);
		for(int i=0; i<(*vp.second - *vp.first); i++)
		{
			bool tag = true;
			//get parent vertices
			in_edge_iterator in_i, in_end;
			edge_descriptor e;
			for (boost::tie(in_i, in_end) = in_edges(i, testJob.g); in_i != in_end; ++in_i) 
			{
				e = *in_i;
				Vertex src = source(e, testJob.g);					
				if(testJob.g[src].status != finished)
				{
					tag = false;
					break;
				}
			}
			if(testJob.g[i].status == ready || (testJob.g[i].status != scheduled && testJob.g[i].status != finished && tag)){
				ready_task.push_back(&testJob.g[i]);							
			}
		}

		//step2: find available VMs for ready tasks according to EDF
		std::sort(ready_task.begin(),ready_task.end(), myfunction);
		for(int i=0; i<ready_task.size(); i++)//earliest deadline first
		{
			taskVertex* curr_task = ready_task[i];
			//if(curr_task->readyCountdown == -1)//
			int config = curr_task->prefer_type;
			bool find = false;
			//check VM list for available machine
			int VM_size = VMTP[config].size();
			for(int j=0; j<VM_size; j++)
			{
				if(VMTP[config][j]->curr_task == NULL)
				{
					find = true;
					curr_task->assigned_type = config;
					VMTP[config][j]->curr_task = curr_task;
					break;
				}
			}
			if(find) {
				curr_task->status = scheduled;
				curr_task->start_time = t;
				curr_task->restTime = curr_task->estTime[curr_task->assigned_type];
			}
			else {
				if(DynamicSchedule) {
					//if not find the prefered type, find a better type
					//if cannot, randomly select one, if still cannot, wait in the queue
					for(int new_config = config+1; new_config<types; new_config++) {
						int VM_size = VMTP[new_config].size();
						for(int j=0; j<VM_size; j++) {
							if(VMTP[new_config][j]->curr_task == NULL)	{
								find = true;
								curr_task->assigned_type = new_config;
								VMTP[new_config][j]->curr_task = curr_task;
								break;
							}
						}
					}
					if(find) { //found a better type
						curr_task->status = scheduled;
						curr_task->start_time = t;
						curr_task->restTime = curr_task->estTime[curr_task->assigned_type];
					}
					else { // cannot find a better type, then randomly find another type
						for(int new_config = config-1; new_config>=0; new_config--)	{
							int VM_size = VMTP[new_config].size();
							for(int j=0; j<VM_size; j++) {
								if(VMTP[new_config][j]->curr_task == NULL)	{
									find = true;
									curr_task->assigned_type = new_config;
									VMTP[new_config][j]->curr_task = curr_task;
									break;
								}
							}
						}
						if(find) { //found a worse type
							curr_task->status = scheduled;
							curr_task->start_time = t;
							curr_task->restTime = curr_task->estTime[curr_task->assigned_type];
						}
						else { //wait in the queue of type config
							curr_task->readyCountDown += 1;
						}
					}					
				}
				else { //static scheduling, if cannot be assigned, just wait
					curr_task->readyCountDown += 1;
				}
			}			
		}

		//step3: find scheduled tasks and execute them
		vp = vertices(testJob.g);
		std::vector<taskVertex*> scheduled_task;
		for(int i=0; i<(*vp.second - *vp.first ); i++)
			if(testJob.g[i].status == scheduled)
				scheduled_task.push_back(&testJob.g[i]);
		for(int i=0; i<scheduled_task.size(); i++)	{
			scheduled_task[i]->restTime -= 1;////////////////////////////
			if(scheduled_task[i]->restTime <= 0) {
				scheduled_task[i]->status = finished;
				scheduled_task[i]->end_time = t+1;
				scheduled_task[i]->cost = (scheduled_task[i]->end_time - scheduled_task[i]->start_time) * priceOnDemand[scheduled_task[i]->assigned_type] /60.0;
				//make the vm.task = NULL
				for(int j=0; j<VMTP[scheduled_task[i]->assigned_type].size(); j++)
					if(VMTP[scheduled_task[i]->assigned_type][j]->curr_task == scheduled_task[i]) {
						VMTP[scheduled_task[i]->assigned_type][j]->curr_task = NULL;
						//VMTP[scheduled_task[i]->assigned_type][j]->in_use += scheduled_task[i]->estTime[scheduled_task[i]->assigned_type];
						break;
					}
			}
		}				
		//step 4
		for(int i=0; i<types; i++)	{
			int size1 = VMTP[i].size();						
			for(int j=0; j<size1; j++)	{
				if(VMTP[i][j]->curr_task != NULL) VMTP[i][j]->in_use += 1;
				VMTP[i][j]->life_time += 1;				
			}
		}

		//ending, decide to continue while or not
		t += 1;
		condition = false;
		int unfinishednum = 0;
		for(int i=0; i < (*vp.second - *vp.first ); i++){
			if(testJob.g[i].status!= finished)
			{
				condition = true;
				unfinishednum += 1;
			}					
		}	
	}while(condition);//if there is a task unfinished
	double* single_utilization = new double[types]; //average utilization of each instance type
	for(int i=0; i<types; i++)	{
		single_utilization[i] = 0;
		int size = VMTP[i].size();
		for(int j=0; j<size; j++)	{
			single_utilization[i] += VMTP[i][j]->in_use ;
//			VMTP[i][j]->in_use = 0;
		}
		single_utilization[i] /= t*size;
		std::cout<< "utilization of one server is "<<single_utilization[i]<<std::endl;
	}

	miu = 1/t;
	printf("miu is %4f\n",miu);
	int m = provision(lamda, miu, testJob.deadline, QoS);
	double ru = lamda/(miu*m);
	for(int i=0; i<types; i++)
		std::cerr<<"no sharing utilization is "<<lamda/m/miu*single_utilization[i]<<std::endl;

	//real simulation with m*VMs number of machines
	for(int i=0; i<types; i++) {
		VMTP[i].clear();
		for(int j=0; j<VMs[i]*m; j++)
			VMTP[i].push_back(new VM());
	}

	//get ready the test job and use it to copy the other jobs
	vp = vertices(testJob.g);
	for(int i=0; i<(*vp.second - *vp.first); i++){
		testJob.g[i].end_time = testJob.g[i].start_time = testJob.g[i].readyCountDown 
			= testJob.g[i].restTime = testJob.g[i].cost = 0;
		testJob.g[i].status = not_ready;
	}
	std::vector<Job*> workflows; //continuous workflow
	double arrival_time = 0;
	Job* job = new Job(pipeline, deadline, lamda);
	job->g = testJob.g;
	job->arrival_time = 0;
	workflows.push_back(job);
	t = 0; condition = false;
	double moneycost = 0;

	//incomming jobs
	while(arrival_time < max_t){
		double interarrival = (int)rnd_exponential(lamda, t);//make time be integers
		if(interarrival < 1.0) interarrival = 1.0;
		arrival_time += interarrival;
		Job* job = new Job(pipeline,deadline+arrival_time,lamda);
		job->g = testJob.g;
		job->arrival_time = arrival_time;
		vp = vertices(job->g);
		for(int i=0; i<(*vp.second - *vp.first); i++)
			job->g[i].sub_deadline += arrival_time;
		workflows.push_back(job);
	}
	do{
		//if a new job has come
		for(int i=0; i<workflows.size(); i++) {
			if((int)t == (int)workflows[i]->arrival_time) {
				vp = vertices(workflows[i]->g);
				workflows[i]->g[*vp.first].status = ready;
				break;
			}
		}

		//step1: get all the ready tasks
		std::vector<taskVertex*> ready_task;
		int num_jobs = workflows.size();
		for(int i=0; i<num_jobs; i++) {
			vp = vertices(workflows[i]->g);
			for(int j=0; j<(*vp.second - *vp.first); j++){
				bool tag = true;
				//get parent vertices
				in_edge_iterator in_i, in_end;
				edge_descriptor e;
				boost::tie(in_i, in_end) = in_edges(j, workflows[i]->g);
				if(in_i == in_end) tag = false;
				else {
					for (; in_i != in_end; ++in_i) {
						e = *in_i;
						Vertex src = source(e, workflows[i]->g);					
						if(workflows[i]->g[src].status != finished)	{
							tag = false;
							break;
						}
					}
				}
				if(workflows[i]->g[j].status == ready || (workflows[i]->g[j].status != scheduled && workflows[i]->g[j].status != finished && tag)){
					workflows[i]->g[j].status = ready;
					ready_task.push_back(&workflows[i]->g[j]);							
				}
			}
		}

		//step2: find available VMs for ready tasks according to EDF
		std::sort(ready_task.begin(),ready_task.end(), myfunction);
		for(int i=0; i<ready_task.size(); i++)//earliest deadline first
		{
			taskVertex* curr_task = ready_task[i];
			//if(curr_task->readyCountdown == -1)//
			int config = curr_task->prefer_type;
			bool find = false;
			//check VM list for available machine
			int VM_size = VMTP[config].size();
			for(int j=0; j<VM_size; j++)
			{
				if(VMTP[config][j]->curr_task == NULL)
				{
					find = true;
					curr_task->assigned_type = config;
					VMTP[config][j]->curr_task = curr_task;
					break;
				}
			}
			if(find) {
				curr_task->status = scheduled;
				curr_task->start_time = t;
				curr_task->restTime = curr_task->estTime[curr_task->assigned_type];
			}
			else {
				if(DynamicSchedule) {
					//if not find the prefered type, find a cheaper type that can finish before sub-dl
                    //if cannot, find a better on that can finish before dl, if still cannot, wait in the queue
					for(int new_config = 0; new_config<config; new_config++) {
						if((curr_task->estTime[new_config] + t) < curr_task->sub_deadline){
							int VM_size = VMTP[new_config].size();
							for(int j=0; j<VM_size; j++) {
								if(VMTP[new_config][j]->curr_task == NULL)	{
									find = true;
									curr_task->assigned_type = new_config;
									VMTP[new_config][j]->curr_task = curr_task;
									break;
								}
							}
							if(find == true) break;
						}						
					}
					if(find) { //found a cheaper type
						curr_task->status = scheduled;
						curr_task->start_time = t;
						curr_task->restTime = curr_task->estTime[curr_task->assigned_type];
					}
					else { // cannot find a cheaper type, then find a better type
						for(int new_config = config+1; new_config<types; new_config++)	{
							int VM_size = VMTP[new_config].size();
							for(int j=0; j<VM_size; j++) {
								if(VMTP[new_config][j]->curr_task == NULL)	{
									find = true;
									curr_task->assigned_type = new_config;
									VMTP[new_config][j]->curr_task = curr_task;
									break;
								}
							}
							if(find == true) break;
						}
						if(find) { //found a better type
							curr_task->status = scheduled;
							curr_task->start_time = t;
							curr_task->restTime = curr_task->estTime[curr_task->assigned_type];
						}
						else { //wait in the queue of type config
							curr_task->readyCountDown += 1;
						}
					}					
				}
				else { //static scheduling, if cannot be assigned, just wait
					curr_task->readyCountDown += 1;
				}
			}			
		}

		//step3: find scheduled tasks and execute them
		std::vector<taskVertex*> scheduled_task;
		for(int i=0; i<num_jobs; i++) {
			vp = vertices(workflows[i]->g);
			for(int j=0; j<(*vp.second - *vp.first); j++){
				if(workflows[i]->g[j].status == scheduled)
					scheduled_task.push_back(&workflows[i]->g[j]);
			}
		}		
		for(int i=0; i<scheduled_task.size(); i++)	{
			scheduled_task[i]->restTime -= 1;////////////////////////////
			if(scheduled_task[i]->restTime <= 0) {
				scheduled_task[i]->status = finished;
				scheduled_task[i]->end_time = t;
				scheduled_task[i]->cost = (scheduled_task[i]->end_time - scheduled_task[i]->start_time) * priceOnDemand[scheduled_task[i]->assigned_type] /60.0;
				//make the vm.task = NULL
				for(int j=0; j<VMTP[scheduled_task[i]->assigned_type].size(); j++)
					if(VMTP[scheduled_task[i]->assigned_type][j]->curr_task == scheduled_task[i]) {
						VMTP[scheduled_task[i]->assigned_type][j]->curr_task = NULL;
						//VMTP[scheduled_task[i]->assigned_type][j]->in_use += scheduled_task[i]->estTime[scheduled_task[i]->assigned_type];
						break;
					}
			}
		}				
		//step 4
		for(int i=0; i<types; i++)	{
			int size1 = VMTP[i].size();						
			for(int j=0; j<size1; j++)	{
				if(VMTP[i][j]->curr_task != NULL) VMTP[i][j]->in_use += 1;
				VMTP[i][j]->life_time += 1;				
			}
		}

		double* utilization = new double[types]; //average utilization of each instance type
		//step 5: adjust the number of VMs according to their utilization
		if((int)t % (int)chk_period == 0 && t>0) {
			for(int i=0; i<types; i++)	{
				utilization[i] = 0;
				int size = VMTP[i].size();
				for(int j=0; j<size; j++)	{
					utilization[i] += VMTP[i][j]->in_use/VMTP[i][j]->life_time ;
				//	VMTP[i][j]->in_use = 0;
					
				}
				utilization[i] /= size;
				std::cout<<"at time "<<t<<" utilization of type "<< i << " VM is "<< utilization[i]<<", VM: "<<size<<std::endl;

				double no_sharing_utilization = ru * single_utilization[i];
				int to_shut_down = size - ceil(m*miu*utilization[i]/no_sharing_utilization/lamda);
//				int to_shut_down = size - ceil(utilization[i]/no_sharing_utilization*VMs[i]);
//				int to_shut_down = (int)((no_sharing_utilization - utilization[i])*size/(no_sharing_utilization*VMs[i]));
				for(int j=0; j<size; j++)
				{
					if(to_shut_down <= 0)
						break;
					else if(VMTP[i][j]->curr_task == NULL) {
						double runtime = VMTP[i][j]->life_time;
						//moneycost += (priceOnDemand[i] * ceil(runtime/60.0));
						moneycost += (priceOnDemand[i] * ceil(runtime/60.0));
						VMTP[i].erase(VMTP[i].begin()+j);
						size --;
						to_shut_down --;
					}
				}
			}
		}

		//ending, decide to continue while or not
		t += 1;
		condition = false;
		int unfinishednum = 0;
		for(int j=0; j<workflows.size(); j++) {
			vp = vertices(workflows[j]->g);
			for(int i=0; i < (*vp.second - *vp.first ); i++) {
				if(workflows[j]->g[i].status!= finished) {
					condition = true;
					unfinishednum += 1;
				}					
			}
		}
	}while(t < max_t);

	for(int i=0; i<types; i++)	{
		int size1 = VMTP[i].size();						
		for(int j=0; j<size1; j++)	{
			double runtime = VMTP[i][j]->life_time;
			moneycost += (priceOnDemand[i] * ceil(runtime/60.0));
		}
	}
	printf("Money Cost: %.4f\n ", moneycost);

	int num_jobs = workflows.size();
	double violation = 0;
	double count = 0;
	double ave_exeT = 0;
	double cost_task = 0;
	for(int i=0; i<num_jobs; i++)	{
		vp = vertices(workflows[i]->g);
		if(workflows[i]->g[*(vp.second -1)].status == finished) {
			ave_exeT += (workflows[i]->g[*(vp.second -1)].end_time - workflows[i]->arrival_time);
			if(workflows[i]->g[*(vp.second -1)].end_time > workflows[i]->deadline)
				violation += 1;
			count += 1;
		}
		for(; vp.first != vp.second; ++vp.first)
			cost_task += workflows[i]->g[*vp.first].cost;
	}
	std::cout<<"Money cost spent on executing tasks: "<<cost_task<<std::endl;
	std::cout<<"average execution time of "<<count<<" jobs is "<<(ave_exeT/count)<<std::endl;
	std::cout<<"deadline violation rate: "<<(violation/count)<<std::endl;
	std::cout<<"end of execution."<<std::endl;
}

std::pair<double, double> VirtualCluster::Cal_Cost(Job* testJob){
	for(int i=0; i<types; i++)
		testJob->templates[i].clear();
	double cost_dl = 0;

	std::pair<vertex_iter, vertex_iter> vp; 
	vp = vertices(testJob->g);
	int size = *vp.second - *vp.first;
	
	for(int i=0; i<size; i++){
		if(i == 0 ) { //dummy tasks
			testJob->g[i].start_time = 0;
			testJob->g[i].end_time = testJob->g[i].start_time + testJob->g[i].estTime[testJob->g[i].prefer_type ];
		}
		else {
			double max=0;
			//get max end_time of parent vertices
			in_edge_iterator in_i, in_end;
			edge_descriptor e;
			for (boost::tie(in_i, in_end) = in_edges(i, testJob->g); in_i != in_end; ++in_i) 
			{
				e = *in_i;
				Vertex src = source(e, testJob->g);					
				if(testJob->g[src].end_time > max) max = testJob->g[src].end_time;
			}
			testJob->g[i].start_time = max;
			testJob->g[i].end_time = testJob->g[i].start_time + testJob->g[i].estTime[testJob->g[i].prefer_type ];
		}
		if(testJob->g[i].start_time != testJob->g[i].end_time) {
			std::pair<double, double> pair;
			pair.first = testJob->g[i].start_time;
			pair.second = testJob->g[i].end_time;
			int index = testJob->g[i].prefer_type;
			testJob->templates[index].push_back(pair);
		}
	}
	
	for(int i=0; i<types; i++) {
		int size_slots = testJob->templates[i].size();
		std::sort(testJob->templates[i].begin(),testJob->templates[i].end(),mycompare);
		for(int j=0; j<size_slots-1; j++) {
			if(testJob->templates[i][j+1].first >= testJob->templates[i][j].second) {
				//if <idle_time, merge to one VM
				if((testJob->templates[i][j+1].first - testJob->templates[i][j].second)< 1e-12) {
					testJob->templates[i][j].second = testJob->templates[i][j+1].second;
					testJob->templates[i].erase(testJob->templates[i].begin()+j+1);
					size_slots--;
					j--;
				}
			}
		}
	}
	
	for(int i=0; i<types; i++) {
		int size_slots = testJob->templates[i].size();
		for(int j=0; j<size_slots; j++)
			cost_dl += ceil((testJob->templates[i][j].second-testJob->templates[i][j].first)/60.0) * priceOnDemand[i];
	}
	double exeT_dl = 0;
	for(int i=0; i<size; i++)
		if(testJob->g[i].end_time>exeT_dl) exeT_dl = testJob->g[i].end_time;
	std::pair<double, double> pair;
	pair.first = cost_dl;
	pair.second = exeT_dl;
	return pair;
}

double* VirtualCluster::sharing_rate(Job job) {
	std::vector<std::pair<double, double> > templates[types];
	//a set of incoming jobs to calculate the sharing rate
	int num = 100;
	int count = 0;
	double arrival_time = 0;
	while(count < num){
		for(int i=0; i<types; i++) {
			int size = job.templates[i].size();
			for(int j=0; j<size; j++) {
				std::pair<double, double> pair;
				pair.first = job.templates[i][j].first + arrival_time;
				pair.second = job.templates[i][j].second + arrival_time;
				templates[i].push_back(pair);
			}
		}
		count++;
		double interarrival = rnd_exponential(lamda, count);
		if(interarrival < 1.0) interarrival = 1.0;
		arrival_time += interarrival;
	}
	for(int i=0; i<types; i++)
		std::sort(templates[i].begin(),templates[i].end(),mycompare);

	double* original_time = new double [types];
	double* merge_time = new double[types];
	for(int i=0; i<types; i++) {
		original_time[i] = 0;
		int size_slot = job.templates[i].size();
		for(int j=0; j<size_slot; j++) {
			original_time[i] += ceil((job.templates[i][j].second - job.templates[i][j].first)/60.0) * 60.0;
		}
		original_time[i] *= num;
	}
	//do the merge and calculate the time after merge
	for(int i=0; i<types; i++) {
		int size_slots = templates[i].size();
		std::sort(templates[i].begin(),templates[i].end(),mycompare);
		for(int j=0; j<size_slots-1; j++) {
			for(int k=j+1; k<size_slots; k++) {
				if(templates[i][k].first >= templates[i][j].second) {
					//if <idle_time, merge to one VM
					if((templates[i][k].first - templates[i][j].second)< inter_time) {
						templates[i][j].second = templates[i][k].second;
						templates[i].erase(templates[i].begin()+k);
						size_slots--;
						k--;
					}
				}
			}
		}
	}
	for(int i=0; i<types; i++) {
		merge_time[i] = 0;
		int size_slot = templates[i].size();
		for(int j=0; j<size_slot; j++) {
			merge_time[i] += ceil((templates[i][j].second - templates[i][j].first)/60.0) * 60.0;
		}
	}
	double* rate = new double[types];
	for(int i=0; i<types; i++) {
		rate[i] = merge_time[i]/original_time[i];
	}
	return rate;
}
void VirtualCluster::Simulate_SA(Job testJob){
	//output cost for the deadline assign method	
	/*for(int i=0; i<size; i++) {
		int type = testJob.g[i].prefer_type;
		cost_dl += testJob.g[i].estTime[type] * priceOnDemand[type];
	}
	//assign the weight for edges first
	property_map<Graph, edge_weight_t>::type weightmap = get(edge_weight, testJob.g);
	for(int i=0; i<size; i++) //edge weight for communication cost
	{
		in_edge_iterator in_i, in_end;
		for (boost::tie(in_i, in_end) = in_edges(i, testJob.g); in_i != in_end; ++in_i)
		{
			edge_descriptor e = *in_i;
			weightmap[e] = -1* (testJob.g[i].estTime[testJob.g[i].prefer_type]);//deadline assign using minimum execution time
		}
	}
	std::vector<int> CP = testJob.find_CP();
	double exeT_dl=0;
	for(int i=0; i<CP.size(); i++)
		exeT_dl += testJob.g[CP[i]].estTime[testJob.g[i].prefer_type];
	*/
	std::pair<double, double> dl_result = Cal_Cost(&testJob);
	std::cout<<"cost of the deadline assign method is "<<dl_result.first<<", exeT is "<<dl_result.second<<std::endl;

	//STEP 1: use the simulated anealing version
	double Temp = dl_result.first / 10;//value of Temp can be set according to the cost value
	//double Temp = 1;
	double cool_r = 0.9;
	double T_limit = 1e-10;
	Job tmpJob = testJob;
	//initial solution
	
	int size = num_vertices(testJob.g);
	double cost_sa = dl_result.first;
	double exeT_sa = dl_result.second;
	do{
		//generate a neighbor
		int rnd_tk = (double)rand() / (RAND_MAX + 1)*size;
		int rnd_tp = (double)rand() / (RAND_MAX + 1)*types;
		int old_type = testJob.g[rnd_tk].prefer_type;
		testJob.g[rnd_tk].prefer_type = rnd_tp;

		//change config of rnd_tk, then need to change the config for all subsequent tasks
		std::pair<double, double> new_result = Cal_Cost(&testJob);
		double delta_cost = new_result.first - cost_sa;
		bool accept = false;
		if(delta_cost<=0) 
			accept = true;
		else {
			double rnd = (double)rand() / (RAND_MAX + 1);
			if(std::exp(-delta_cost/Temp)>rnd)
				accept = true; //true, what if do not allow acceptance?
		}
		if(!accept) {
			//testJob.g[rnd_tk].prefer_type = old_type;
			testJob = tmpJob;
		} else {
			if(new_result.second <= testJob.deadline*0.9) {//yes OnDemandLag
				cost_sa = new_result.first;
				exeT_sa = new_result.second;
				tmpJob = testJob;
			}
			else //no
				//testJob.g[rnd_tk].prefer_type = old_type;			
				testJob = tmpJob;
		}
		Temp *= cool_r;
	}while(Temp > T_limit);
	std::cout<<"cost of the SA method is "<<cost_sa<<", exeT is "<<exeT_sa<<std::endl;
	
	//STEP 2: reassign the sub-deadline for each task according to the SA solution
	testJob.deadline_assign();

	//remove the dummy tasks which are added for deadline assign
	vertex_iter vi, vi_end, next;
	tie(vi, vi_end) = vertices(testJob.g);
	clear_vertex(*vi,testJob.g);
	remove_vertex(*vi,testJob.g);
	tie(vi, vi_end) = vertices(testJob.g);
	clear_vertex(*(--vi_end),testJob.g);
	remove_vertex(*(vi_end),testJob.g);
	
	size = num_vertices(testJob.g);
	//STEP 3: simulate sharing rate
	int* VMs = testJob.find_ServerGroup_SA();
	//int* VMs = new int[types];
	//multiply VMs with the utilization
	double* d_VMs = new double[types];
	if(true) {
		for(int i=0; i<types; i++) {
			d_VMs[i] = (double)VMs[i];
			double sum = 0;
			int templatesize = testJob.templates[i].size();
			for(int j=0; j<templatesize; j++)
				sum += (testJob.templates[i][j].second - testJob.templates[i][j].first);
			d_VMs[i] = sum/testJob.g[size-1].end_time;
		}

	}
	if(!sharing)
		for(int i=0; i<types; i++)
			std::cout<<"number of type "<<i<<" VM is: "<<VMs[i]<<"d_VM is: "<<d_VMs[i]<<std::endl;
	double* share_rate;
	if(sharing) {
		share_rate = sharing_rate(testJob);
		for(int i=0; i<types; i++)
			std::cout<<"number of type "<<i<<" VM is: "<<VMs[i]<<"d_VM is: "<<d_VMs[i]<<", sharing rate is: "<< share_rate[i]<<std::endl;
	}
	double miu = 1.0/(exeT_sa);	//OnDemandLag
	int m = provision(lamda, miu, testJob.deadline,QoS);
	printf("lambda is %4f\t miu is %4f\t m is %d\n",lamda,miu,m);

	//runtime adjustment of the VMs	
	double ru = lamda/(miu*m);
	std::vector<VM*> VMTP[types];
	for(int i=0; i<types; i++) {
		VMTP[i].clear();
		if(sharing && share_rate[i] < 1.0)
			VMs[i] = ceil(d_VMs[i]*m*share_rate[i])+1;
		else VMs[i] = ceil(d_VMs[i]*m)+1;
		for(int j=0; j<VMs[i]; j++)
			VMTP[i].push_back(new VM());
	}
	for(int i=0; i<types; i++)
		std::cout<<"finally, the number of type "<<i<<" VM is: "<<VMs[i]<<std::endl;
	//STEP 4: get ready for real simulation
	//get ready the test job and use it to copy the other jobs
	std::pair<vertex_iter, vertex_iter> vp; 
	vp = vertices(testJob.g);
	for(int i=0; i<(*vp.second - *vp.first); i++){
		testJob.g[i].end_time = testJob.g[i].start_time = testJob.g[i].readyCountDown 
			= testJob.g[i].restTime = testJob.g[i].cost = 0;
		testJob.g[i].status = not_ready;
	}
	std::vector<Job*> workflows; //continuous workflow
	double arrival_time = 0;
	Job* job = new Job(pipeline, deadline, lamda);
	job->g = testJob.g;
	job->arrival_time = 0;
	workflows.push_back(job);
	double t = 0; bool condition = false;
	double moneycost = 0;
	std::ifstream infile;
	std::string a = "arrivaltime_integer_";
	std::string b;
	std::ostringstream strlamda;
	strlamda << lamda;
	b = strlamda.str();
	std::string c = ".txt";
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
		vp = vertices(job->g);
		for(int i=0; i<(*vp.second - *vp.first); i++)
			job->g[i].sub_deadline += arrival_time;
		workflows.push_back(job);
	}

	int pointer = 0; // point to the position of current arrival job
	do{
		//if a new job has come
		for(int i=pointer; i<workflows.size(); i++) { 
			if((int)t == (int)workflows[i]->arrival_time) {
				vp = vertices(workflows[i]->g);
				if(testJob.type == single || testJob.type == pipeline || testJob.type == hybrid)
					workflows[i]->g[*vp.first].status = ready;
				else if(testJob.type == Montage)
					for(int j=0; j<4; j++)
						workflows[i]->g[j].status = ready;
				else if(testJob.type == Ligo)
					for(int j=0; j<9; j++)
						workflows[i]->g[j].status = ready;
				else if(testJob.type == Cybershake) {
					workflows[i]->g[0].status = ready;
					workflows[i]->g[1].status = ready;
				}
				pointer = i;
				break;
			}
		}

		//step1: get all the ready tasks
		std::vector<taskVertex*> ready_task;
		int num_jobs = workflows.size();
		for(int i=0; i<num_jobs; i++) {
			vp = vertices(workflows[i]->g);
			if(workflows[i]->g[*(vp.second -1)].status != finished) {
				for(int j=0; j<(*vp.second - *vp.first); j++){
					bool tag = true;
					//get parent vertices
					in_edge_iterator in_i, in_end;
					edge_descriptor e;
					boost::tie(in_i, in_end) = in_edges(j, workflows[i]->g);
					if(in_i == in_end) tag = false;
					else {
						for (; in_i != in_end; ++in_i) {
							e = *in_i;
							Vertex src = source(e, workflows[i]->g);					
							if(workflows[i]->g[src].status != finished)	{
								tag = false;
								break;
							}
						}
					}
					if(workflows[i]->g[j].status == ready || (workflows[i]->g[j].status != scheduled && workflows[i]->g[j].status != finished && tag)){
						workflows[i]->g[j].status = ready;
						ready_task.push_back(&workflows[i]->g[j]);							
					}
				}
			}
		}

		//step2: find available VMs for ready tasks according to EDF
		std::sort(ready_task.begin(),ready_task.end(), myfunction);
		for(int i=0; i<ready_task.size(); i++)//earliest deadline first
		{
			taskVertex* curr_task = ready_task[i];
			//if(curr_task->readyCountdown == -1)//
			int config = curr_task->prefer_type;
			bool find = false;
			//check VM list for available machine
			int VM_size = VMTP[config].size();
			for(int j=0; j<VM_size; j++)
			{
				if(VMTP[config][j]->curr_task == NULL)
				{
					find = true;
					curr_task->assigned_type = config;
					VMTP[config][j]->curr_task = curr_task;
					VMTP[config][j]->idle_time = 0;
					break;
				}
			}
			if(find) {
				curr_task->status = scheduled;
				curr_task->start_time = t;
				curr_task->restTime = curr_task->estTime[curr_task->assigned_type];
			}
			else {
				if(DynamicSchedule) {
					//if not find the prefered type, find a cheaper type that can finish before sub-dl
                    //if cannot, find a better on that can finish before dl, if still cannot, wait in the queue
					for(int new_config = 0; new_config<config; new_config++) {
						if((curr_task->estTime[new_config] + t) < curr_task->sub_deadline){
							int VM_size = VMTP[new_config].size();
							for(int j=0; j<VM_size; j++) {
								if(VMTP[new_config][j]->curr_task == NULL)	{
									find = true;
									curr_task->assigned_type = new_config;
									VMTP[new_config][j]->curr_task = curr_task;
									VMTP[new_config][j]->idle_time = 0;
									break;
								}
							}
							if(find == true) break;
						}						
					}
					if(find) { //found a cheaper type
						curr_task->status = scheduled;
						curr_task->start_time = t;
						curr_task->restTime = curr_task->estTime[curr_task->assigned_type];
					}
					else { // cannot find a cheaper type, then find a better type
						for(int new_config = config+1; new_config<types; new_config++)	{
							int VM_size = VMTP[new_config].size();
							for(int j=0; j<VM_size; j++) {
								if(VMTP[new_config][j]->curr_task == NULL)	{
									find = true;
									curr_task->assigned_type = new_config;
									VMTP[new_config][j]->curr_task = curr_task;
									VMTP[new_config][j]->idle_time = 0;
									break;
								}
							}
							if(find == true) break;
						}
						if(find) { //found a better type
							curr_task->status = scheduled;
							curr_task->start_time = t;
							curr_task->restTime = curr_task->estTime[curr_task->assigned_type];
						}
						else { //wait in the queue of type config
							curr_task->readyCountDown += 1;
							if(on_off) {
								if((curr_task->estTime[curr_task->prefer_type] +1 + OnDemandLag > curr_task->sub_deadline - t) && VMTP[curr_task->prefer_type].size() < VMs[curr_task->prefer_type]){
									find = true;
									VM* new_vm = new VM();
									new_vm->curr_task = curr_task;
									curr_task->assigned_type = curr_task->prefer_type;
									curr_task->start_time = t;
									curr_task->status = scheduled;
									curr_task->restTime = curr_task->estTime[curr_task->prefer_type] + OnDemandLag;
									VMTP[curr_task->prefer_type].push_back(new_vm);
								}
								//else if((curr_task->estTime[curr_task->prefer_type] + 1 + OnDemandLag > curr_task->sub_deadline - t) && VMTP[curr_task->prefer_type].size() == VMs[curr_task->prefer_type])
								//	printf("time out, gonna violate deadline!!!\n");
							}
						}
					}					
				}
				else { //static scheduling, if cannot be assigned, just wait
					curr_task->readyCountDown += 1;
					if(on_off) {
						if((curr_task->estTime[curr_task->prefer_type] + 1 + OnDemandLag > curr_task->sub_deadline - t) && VMTP[curr_task->prefer_type].size() < VMs[curr_task->prefer_type]){
							find = true;
							VM* new_vm = new VM();
							new_vm->curr_task = curr_task;
							curr_task->assigned_type = curr_task->prefer_type;
							curr_task->start_time = t;
							curr_task->status = scheduled;
							curr_task->restTime = curr_task->estTime[curr_task->prefer_type] + OnDemandLag;
							VMTP[curr_task->prefer_type].push_back(new_vm);
						}
						//else if((curr_task->estTime[curr_task->prefer_type] + 1 + OnDemandLag > curr_task->sub_deadline - t) && VMTP[curr_task->prefer_type].size() == VMs[curr_task->prefer_type])
						//	printf("time out, gonna violate deadline!!!\n");
					}
				}
			}			
		}

		//step3: find scheduled tasks and execute them
		std::vector<taskVertex*> scheduled_task;
		for(int i=0; i<num_jobs; i++) {
			vp = vertices(workflows[i]->g);
			for(int j=0; j<(*vp.second - *vp.first); j++){
				if(workflows[i]->g[j].status == scheduled)
					scheduled_task.push_back(&workflows[i]->g[j]);
			}
		}		
		for(int i=0; i<scheduled_task.size(); i++)	{
			scheduled_task[i]->restTime -= 1;////////////////////////////
			if(scheduled_task[i]->restTime <= 0) {
				scheduled_task[i]->status = finished;
				scheduled_task[i]->end_time = t;
				scheduled_task[i]->cost = (scheduled_task[i]->end_time - scheduled_task[i]->start_time)/60.0 * priceOnDemand[scheduled_task[i]->assigned_type];
				//make the vm.task = NULL
				for(int j=0; j<VMTP[scheduled_task[i]->assigned_type].size(); j++)
					if(VMTP[scheduled_task[i]->assigned_type][j]->curr_task == scheduled_task[i]) {
						VMTP[scheduled_task[i]->assigned_type][j]->curr_task = NULL;
						//VMTP[scheduled_task[i]->assigned_type][j]->in_use += scheduled_task[i]->estTime[scheduled_task[i]->assigned_type];
						break;
					}
			}
		}				
		//step 4
		for(int i=0; i<types; i++)	{
			int size1 = VMTP[i].size();						
			for(int j=0; j<size1; j++)	{
				if(VMTP[i][j]->curr_task != NULL) VMTP[i][j]->in_use += 1;
				else VMTP[i][j]->idle_time += 1;
				VMTP[i][j]->life_time += 1;				
			}			
		}

		double* utilization = new double[types]; //average utilization of each instance type
		//step 5: adjust the number of VMs according to their utilization
		if((int)t % (int)chk_period == 0 && t>0) {
			for(int i=0; i<types; i++)	{
				utilization[i] = 0;
				int size = VMTP[i].size();
				for(int j=0; j<size; j++)	{
					utilization[i] += VMTP[i][j]->in_use/VMTP[i][j]->life_time ;
				}
				utilization[i] /= size;
				std::cout<<"at time "<<t<<" utilization of type "<< i << " VM is "<< utilization[i]<<", VM: "<<size<<std::endl;
			}
		}
		if(on_off) {
			for(int i=0; i<types; i++) {
				int size = VMTP[i].size();
				for(int j=0; j<size; j++) {
					if(VMTP[i][j]->idle_time > idle_interval) {
						//turn off this VM
						double runtime = VMTP[i][j]->life_time;
						moneycost += (priceOnDemand[i] * ceil(runtime/60.0));
						VMTP[i].erase(VMTP[i].begin()+j);
						j--;
						size--;
					}
				}
			}			
		}

		//ending, decide to continue while or not
		t += 1;
		condition = false;
		int unfinishednum = 0;
		for(int j=0; j<workflows.size(); j++) {
			vp = vertices(workflows[j]->g);
			for(int i=0; i < (*vp.second - *vp.first ); i++) {
				if(workflows[j]->g[i].status!= finished) {
					condition = true;
					unfinishednum += 1;
				}					
			}
		}
	}while(t < max_t);

	for(int i=0; i<types; i++)	{
		int size1 = VMTP[i].size();						
		for(int j=0; j<size1; j++)	{
			double runtime = VMTP[i][j]->life_time;
			moneycost += (priceOnDemand[i] * ceil(runtime/60.0));
		}
	}
	printf("Money Cost: %.4f\n ", moneycost);

	int num_jobs = workflows.size();
	double violation = 0;
	double count = 0;
	double ave_exeT = 0;
	double cost_task = 0;
	for(int i=0; i<num_jobs; i++)	{
		vp = vertices(workflows[i]->g);
		if(workflows[i]->g[*(vp.second -1)].status == finished) {
			ave_exeT += (workflows[i]->g[*(vp.second -1)].end_time - workflows[i]->arrival_time);
			if(workflows[i]->g[*(vp.second -1)].end_time > workflows[i]->deadline)
				violation += 1;
			count += 1;
		}
		for(; vp.first != vp.second; ++vp.first)
			cost_task += workflows[i]->g[*vp.first].cost;
	}
	std::cout<<"Money cost spent on executing tasks: "<<cost_task<<std::endl;
	std::cout<<"average execution time of "<<count<<" jobs is "<<(ave_exeT/count)<<std::endl;
	std::cout<<"deadline violation rate: "<<(violation/count)<<std::endl;
	std::cout<<"end of execution."<<std::endl;
}
void AutoScaling::Simulate(Job testJob) {
	std::vector<Job*> workflows; //continuous workflow
	double arrival_time = 0;
	Job* job = new Job(pipeline, deadline, lamda);
	job->g = testJob.g;
	job->arrival_time = 0;
	workflows.push_back(job);
	double t = 0; 
	bool condition = false;

	std::pair<vertex_iter, vertex_iter> vp; 
	std::ifstream infile;
	std::string a = "arrivaltime_integer_";
	std::string b;
	std::ostringstream strlamda;
	strlamda << lamda;
	b = strlamda.str();
	std::string c = ".txt";
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
		vp = vertices(job->g);
		for(int i=0; i<(*vp.second - *vp.first); i++)
			job->g[i].sub_deadline += arrival_time;
		workflows.push_back(job);
	}

	std::vector<VM*> VMTP[types];
	int need_VM[types]={0,0,0,0};

	double moneycost = 0;
	do{	
		//if a new job has come
		for(int i=0; i<workflows.size(); i++) {
			if((int)t == (int)workflows[i]->arrival_time) {
				vp = vertices(workflows[i]->g);
				if(testJob.type == single || testJob.type == pipeline || testJob.type == hybrid)
					workflows[i]->g[*vp.first].status = ready;
				else if(testJob.type == Montage)
					for(int j=0; j<4; j++)
						workflows[i]->g[j].status = ready;
				else if(testJob.type == Ligo)
					for(int j=0; j<9; j++)
						workflows[i]->g[j].status = ready;
				else if(testJob.type == Cybershake) {
					workflows[i]->g[0].status = ready;
					workflows[i]->g[1].status = ready;
				}
				break;
			}
		}
		//step1: get all the ready tasks
		std::vector<taskVertex*> ready_task;
		int num_jobs = workflows.size();
		for(int i=0; i<num_jobs; i++) {
			vp = vertices(workflows[i]->g);
			for(int j=0; j<(*vp.second - *vp.first); j++){
				bool tag = true;
				//get parent vertices
				in_edge_iterator in_i, in_end;
				edge_descriptor e;
				boost::tie(in_i, in_end) = in_edges(j, workflows[i]->g);
				if(in_i == in_end) tag = false;
				else {
					for (; in_i != in_end; ++in_i) {
						e = *in_i;
						Vertex src = source(e, workflows[i]->g);					
						if(workflows[i]->g[src].status != finished)	{
							tag = false;
							break;
						}
					}
				}
				if(workflows[i]->g[j].status == ready || (workflows[i]->g[j].status != scheduled && workflows[i]->g[j].status != finished && tag)){
					workflows[i]->g[j].status = ready;
					ready_task.push_back(&workflows[i]->g[j]);							
				}
			}
		}

		std::sort(ready_task.begin(),ready_task.end(), myfunction);
		for(int i=0; i<ready_task.size(); i++)//earliest deadline first
		{
			taskVertex* curr_task=ready_task[i];
			if(curr_task->readyCountDown == -1)//
			{
						
				int _config = curr_task->prefer_type;
				bool find = false;
				//check VM/SpotVM list for available machine
				int size = VMTP[_config].size();
				for(int j=0; j<size; j++)
				{
					if(VMTP[_config][j]->curr_task == NULL)
					{
						find = true;
						VMTP[_config][j]->curr_task = curr_task;
						break;
					}
				}
				if(find) {
					curr_task->status = scheduled;
					curr_task->assigned_type = curr_task->prefer_type;
					curr_task->start_time = t;
					curr_task->restTime =  curr_task->estTime[curr_task->assigned_type] ;
				}
				else 			
				{
					curr_task->readyCountDown = OnDemandLag;
					curr_task->start_time = t;
				}
			}
			else if(curr_task->readyCountDown == 0)
			{
				curr_task->status = scheduled;
				curr_task->assigned_type = curr_task->prefer_type;
				curr_task->restTime = curr_task->estTime[curr_task->assigned_type] ;

				VM* vm = new VM; 
				vm->life_time = OnDemandLag;
				vm->curr_task = curr_task;
				vm->type = curr_task->assigned_type;
				VMTP[curr_task->assigned_type].push_back(vm);
						
			}			
		}
		//delete VMs without task
		for(int i=0; i<types; i++)	{
			int size1 = VMTP[i].size();					
			for(int j=0; j<size1; j++)	{
				if(VMTP[i][j]->curr_task == NULL) {
					double runtime = VMTP[i][j]->life_time;
					moneycost += (priceOnDemand[i] * ceil(runtime/60.0));
					VMTP[i].erase(VMTP[i].begin()+j);
					size1 --;
				}
			}
		}
		//step 2
		std::vector<taskVertex*> scheduled_task;
		for(int i=0; i<num_jobs; i++) {
			vp = vertices(workflows[i]->g);
			for(int j=0; j<(*vp.second - *vp.first); j++){
				if(workflows[i]->g[j].status == scheduled)
					scheduled_task.push_back(&workflows[i]->g[j]);
			}
		}		
		for(int i=0; i<scheduled_task.size(); i++)	{
			scheduled_task[i]->restTime -= 1;////////////////////////////
			if(scheduled_task[i]->restTime <= 0) {
				scheduled_task[i]->status = finished;
				scheduled_task[i]->end_time = t;
				scheduled_task[i]->cost = (scheduled_task[i]->end_time - scheduled_task[i]->start_time) * priceOnDemand[scheduled_task[i]->assigned_type] /60.0;
				//make the vm.task = NULL
				for(int j=0; j<VMTP[scheduled_task[i]->assigned_type].size(); j++)
					if(VMTP[scheduled_task[i]->assigned_type][j]->curr_task == scheduled_task[i]) {
						VMTP[scheduled_task[i]->assigned_type][j]->curr_task = NULL;
						//VMTP[scheduled_task[i]->assigned_type][j]->in_use += scheduled_task[i]->estTime[scheduled_task[i]->assigned_type];
						break;
					}
			}
		}				
		//step 3
		for(int i=0; i<types; i++)	{
			int size1 = VMTP[i].size();						
			for(int j=0; j<size1; j++)	{
				if(VMTP[i][j]->curr_task != NULL) VMTP[i][j]->in_use += 1;
				VMTP[i][j]->life_time += 1;				
			}
		}
		for(int i=0; i<ready_task.size(); i++)//////////////////////////////////if >0
			if(ready_task[i]->readyCountDown > 0)
				ready_task[i]->readyCountDown -= 1;
			
		double* utilization = new double[types]; //average utilization of each instance type
		
		//step 4: calculate VM utilization
		if((int)t % (int)chk_period == 0 && t>0) {
			for(int i=0; i<types; i++)	{
				utilization[i] = 0;
				int size = VMTP[i].size();
				for(int j=0; j<size; j++)	{
					utilization[i] += VMTP[i][j]->in_use/VMTP[i][j]->life_time ;
				//	VMTP[i][j]->in_use = 0;
				}
				utilization[i] /= size;
				std::cout<<"at time "<<t<<" utilization of type "<< i << " VM is "<< utilization[i]<<", VM: "<<size<<std::endl;
			}
		}

		t += 1;
		condition = false;
		int unfinishednum = 0;
		for(int j=0; j<workflows.size(); j++) {
			vp = vertices(workflows[j]->g);
			for(int i=0; i < (*vp.second - *vp.first ); i++) {
				if(workflows[j]->g[i].status!= finished) {
					condition = true;
					unfinishednum += 1;
				}					
			}
		}
	}while(t<max_t);//there is a task not finished

	for(int i=0; i<types; i++)	{
		int size1 = VMTP[i].size();						
		for(int j=0; j<size1; j++)	{
			double runtime = VMTP[i][j]->life_time;
			moneycost += (priceOnDemand[i] * ceil(runtime/60.0));
		}
	}
	printf("Money Cost: %.4f\n ", moneycost);

	int num_jobs = workflows.size();
	double violation = 0;
	double count = 0;
	double ave_exeT = 0;
	double cost_task = 0;
	for(int i=0; i<num_jobs; i++)	{
		vp = vertices(workflows[i]->g);
		if(workflows[i]->g[*(vp.second - 1)].status == finished) {
			ave_exeT += (workflows[i]->g[*(vp.second -1)].end_time - workflows[i]->arrival_time);
			if(workflows[i]->g[*(vp.second -1)].end_time > workflows[i]->deadline)
				violation += 1;
			count += 1;
		}
		for(; vp.first != vp.second; ++vp.first)
                       	cost_task += workflows[i]->g[*vp.first].cost;
	}
	std::cout<<"average execution time of "<<count<<" jobs is "<<(ave_exeT/count)<<std::endl;
	std::cout<<"deadline violation rate: "<<(violation/count)<<std::endl;
	std::cout<<"end of execution."<<std::endl;
}
