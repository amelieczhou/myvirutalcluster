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
