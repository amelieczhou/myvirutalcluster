//////////////the use of this file///////////////////
//this file is used to test the correctness of consolidation operators

#include "stdafx.h"
#include "Provision.h"
#include "InstanceConfig.h"
#include "Algorithms.h"
#include <string.h>
#include <ctime>

double max_t;
bool DynamicSchedule;
double deadline;
double chk_period;
bool sharing;
bool on_off = true;
double lamda;
double QoS; //for provisioning

int main(int argc, char** argv)
{
	int depth = atoi(argv[3]);
	int width = atoi(argv[4]);
	deadline = atof(argv[5]);
	if(strcmp(argv[6],"dynamic")==0)
		DynamicSchedule = true;
	else DynamicSchedule = false;
	max_t = atof(argv[7]);
	chk_period = atof(argv[8]);
	if(strcmp(argv[9], "sharing")==0)
		sharing = true;
	lamda = atof(argv[10]);
	QoS = atof(argv[11]);

	//organize the inputs: workflows, VMs, user defined parameters	
	Job testJob(pipeline,deadline,lamda);
	testJob.arrival_time = 0;
	if(strcmp(argv[2], "move1") == 0||strcmp(argv[2],"move2")==0){//deadline = 300
		int rnds[5];
		if(strcmp(argv[2], "move1") == 0){
			rnds[0] = 0;  rnds[1] = 1;
			rnds[2] = 0;  rnds[3] = 2; rnds[4] = 0;
		}
		else{
			rnds[0] = 0; rnds[1] = 0;
			rnds[2] = 1; rnds[3] = 2; rnds[4] = 0;
		}			
		for(int i=0; i<5; i++)
		{
			taskVertex tk;
			tk.name = i;
			tk.assigned_type = tk.prefer_type = tk.mark = 0;
			tk.cost = tk.start_time = tk.end_time =tk.sub_deadline = tk.restTime = 0;
			tk.EST = tk.LFT = 0;
			tk.readyCountDown = -1; //
			tk.status = not_ready;
			tk.estTime = new double[types];
			//int rnd = (double)rand() / RAND_MAX * 4;
			int rnd = rnds[i];
			tk.type = rnd;
			if(i == 0 || i == 4) //two dummy tasks
			{
				for(int j=0; j<types; j++)
					tk.estTime[j] = 0;
			}
			else{
				for(int j=0; j<types; j++)
					tk.estTime[j] = Times[rnd][j];
			}
			add_vertex(tk, testJob.g);
		}
		add_edge(0,1,testJob.g);
		add_edge(0,2,testJob.g);
		add_edge(1,3,testJob.g);
		add_edge(2,4,testJob.g);
		add_edge(3,4,testJob.g);
	}
	else if(strcmp(argv[2], "split") == 0){
		int rnds[3] = {0, 2, 0};
		for(int i=0; i<3; i++)
		{
			taskVertex tk;
			tk.name = i;
			tk.assigned_type = tk.prefer_type = tk.mark = 0;
			tk.cost = tk.start_time = tk.end_time =tk.sub_deadline = tk.restTime = 0;
			tk.EST = tk.LFT = 0;
			tk.readyCountDown = -1; //
			tk.status = not_ready;
			tk.estTime = new double[types];
			//int rnd = (double)rand() / RAND_MAX * 4;
			int rnd = rnds[i];
			tk.type = rnd;
			if(i == 0 || i == 2) //two dummy tasks
			{
				for(int j=0; j<types; j++)
					tk.estTime[j] = 0;
			}
			else{
				for(int j=0; j<types; j++)
					tk.estTime[j] = Times[rnd][j];
			}
			add_vertex(tk, testJob.g);
		}
		add_edge(0,1,testJob.g);
		add_edge(1,2,testJob.g);
	}
	else if(strcmp(argv[2], "coschedule") == 0){
		int rnds[4] = {0, 1, 1, 0};
		for(int i=0; i<4; i++)
		{
			taskVertex tk;
			tk.name = i;
			tk.assigned_type = tk.prefer_type = tk.mark = 0;
			tk.cost = tk.start_time = tk.end_time =tk.sub_deadline = tk.restTime = 0;
			tk.EST = tk.LFT = 0;
			tk.readyCountDown = -1; //
			tk.status = not_ready;
			tk.estTime = new double[types];
			//int rnd = (double)rand() / RAND_MAX * 4;
			int rnd = rnds[i];
			tk.type = rnd;
			if(i == 0 || i == 3) {//two dummy tasks
				for(int j=0; j<types; j++)
					tk.estTime[j] = 0;
			}
			else{
				for(int j=0; j<types; j++)
					tk.estTime[j] = Times[rnd][j];
			}
			add_vertex(tk, testJob.g);
		}
		add_edge(0,1,testJob.g);
		add_edge(0,2,testJob.g);
		add_edge(2,3,testJob.g);
		add_edge(1,3,testJob.g);
	}
	else if(strcmp(argv[2], "promote") == 0){
		int rnds[4] = {0, 2, 1, 0};
		for(int i=0; i<4; i++)
		{
			taskVertex tk;
			tk.name = i;
			tk.assigned_type = tk.prefer_type = tk.mark = 0;
			tk.cost = tk.start_time = tk.end_time =tk.sub_deadline = tk.restTime = 0;
			tk.EST = tk.LFT = 0;
			tk.readyCountDown = -1; //
			tk.status = not_ready;
			tk.estTime = new double[types];
			//int rnd = (double)rand() / RAND_MAX * 4;
			int rnd = rnds[i];
			tk.type = rnd;
			if(i == 0 || i == 3) {//two dummy tasks
				for(int j=0; j<types; j++)
					tk.estTime[j] = 0;
			}
			else{
				for(int j=0; j<types; j++)
					tk.estTime[j] = Times[rnd][j];
			}
			add_vertex(tk, testJob.g);
		}
		add_edge(0,1,testJob.g);
		add_edge(1,2,testJob.g);
		add_edge(2,3,testJob.g);
	}
	else if(strcmp(argv[2], "demote") == 0){
	}

	std::clock_t starttime = std::clock();

	std::vector<Job*> jobs;
	jobs.push_back(&testJob);
	Job testJob1 = testJob;
	testJob1.arrival_time = 10;
	testJob1.deadline = 300; 
	testJob1.g[1].type = 1;
	for(int i=0; i<3; i++) {
		testJob1.g[i].estTime = new double[types];
		if(i == 1){
			for(int j=0; j<types; j++)
				testJob1.g[i].estTime[j] = Times[i][j];
		}else	{
			for(int j=0; j<types; j++)
				testJob1.g[i].estTime[j] = 0;
		}
	}

	if(strcmp(argv[2],"split") == 0){		
		jobs.push_back(&testJob1);
	}
	GanttConsolidation alg3;
	if(strcmp(argv[2],"promote")==0){
		testJob.g[0].assigned_type=testJob.g[0].end_time=testJob.g[0].start_time=0;
		testJob.g[3].assigned_type=testJob.g[3].end_time=testJob.g[3].start_time =0;
		testJob.g[1].assigned_type=1; testJob.g[1].start_time = 0; testJob.g[1].end_time = testJob.g[1].estTime[1];
		testJob.g[2].assigned_type=0; testJob.g[2].start_time=testJob.g[1].end_time; testJob.g[2].end_time = testJob.g[1].end_time+testJob.g[2].estTime[0];
		testJob.g[3].assigned_type =0; testJob.g[3].start_time=testJob.g[3].end_time =testJob.g[2].end_time;
	}
	else if(strcmp(argv[2],"coschedule")==0){
		testJob.g[0].assigned_type=testJob.g[0].end_time=testJob.g[0].start_time=0;
		testJob.g[3].assigned_type=testJob.g[3].end_time=testJob.g[3].start_time =0;
		testJob.g[1].assigned_type=1; testJob.g[1].start_time = 0; testJob.g[1].end_time = testJob.g[1].estTime[1];
		testJob.g[2].assigned_type=1; testJob.g[2].start_time = 0; testJob.g[2].end_time = testJob.g[2].estTime[1];
		testJob.g[3].assigned_type =0; testJob.g[3].start_time=testJob.g[3].end_time =testJob.g[2].end_time;
	}
	else {
		//Initialization
		for(int i=0; i<jobs.size(); i++){
			if(strcmp(argv[12],"bestfit") == 0) alg3.Initialization(jobs[i],1);
			else if(strcmp(argv[12],"worstfit") == 0) alg3.Initialization(jobs[i],2);
			else if(strcmp(argv[12],"mostefficient") == 0) alg3.Initialization(jobs[i],3);
		}
	}
	
	for(int i=0; i<jobs.size(); i++){
		jobs[i]->deadline += jobs[i]->arrival_time;
		jobs[i]->name = i;
		std::pair<vertex_iter, vertex_iter> vp; 
		vp = vertices(jobs[i]->g);
		for(; vp.first!=vp.second; ++vp.first) {
			jobs[i]->g[*vp.first].sub_deadline = jobs[i]->deadline;
			jobs[i]->g[*vp.first].job_id = i;
			jobs[i]->g[*vp.first].start_time += jobs[i]->arrival_time;
			jobs[i]->g[*vp.first].end_time += jobs[i]->arrival_time;
		}
	}
	int threshold = 10;
	double totalcost = alg3.Planner(jobs,threshold);

	//the end of simulation
	//calculate the deadline violation rate
	int violate = 0;
	for(int i=0; i<jobs.size(); i++) {
		std::pair<vertex_iter, vertex_iter> vp;
		vp = vertices(jobs[i]->g);
		int vend = *(vp.second -1);
		if(jobs[i]->g[vend].end_time > jobs[i]->deadline)
			violate += 1;
	}
	double violate_rate = (double)violate/jobs.size();
	printf("deadline violation rate is: %4f\n",violate_rate);
	printf("total cost is: %4f\n",totalcost);
}