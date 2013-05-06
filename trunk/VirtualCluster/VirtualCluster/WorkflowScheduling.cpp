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

	if(strcmp(argv[2], "pipeline") == 0)
	{
		testJob.type = pipeline;
		int rnds[] = {0,0,3,3,1,2,1,0,0};//two dummy tasks
		//generate DAG
		for(int i=0; i<depth+2; i++) //two dummy tasks
		{
			taskVertex tk;
			tk.name = i;
			tk.assigned_type = tk.prefer_type = tk.mark = 0;
			tk.cost = tk.start_time = tk.end_time =tk.sub_deadline = tk.restTime = 0;
			tk.EST = tk.LFT = 0;
			tk.readyCountDown = -1; //
			tk.status = not_ready;
			tk.estTime = new double[types];
			//int rnd = (double)rand() / RAND_MAX * types; //[range_min, range_max)	
			int rnd = rnds[i];
			printf("rntypes:%d\n",rnd);
			tk.type = (int)rnd;
			if(i == 0 || i == depth+1) //two dummy tasks
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
		
		//add edges to the pipeline graph
		std::pair<vertex_iter, vertex_iter> vp; 
		vp = vertices(testJob.g);
		for(; vp.first != (vp.second-1); )
		{
			Vertex v1 = *vp.first;
			vp.first++;
			Vertex v2 = *vp.first;

			edge_descriptor e; 
			bool inserted;
			boost::tie(e, inserted) = add_edge(v1, v2, testJob.g);
		}				
	}
	//for hybrid application
	else if(strcmp(argv[2], "hybrid") == 0) //fixed shape as indicated in the paper
	{
		testJob.type = hybrid;
		int rnds[] = {0, 0,3,3,1,2,1,0,1,2,2,3,3,0,1,2, 0};//two dummy tasks
		//generate DAG
		for(int i=0; i<15+2; i++)
		{
			taskVertex tk;
			tk.name = i;
			tk.assigned_type = tk.prefer_type = tk.mark = 0;
			tk.cost = tk.start_time = tk.end_time =tk.sub_deadline = tk.restTime = 0;
			tk.EST = tk.LFT = 0;
			tk.readyCountDown = -1; //
			tk.status = not_ready;
			tk.estTime = new double[types];
			//int rnd = (double)rand() / RAND_MAX * types; //[range_min, range_max)	
			int rnd = rnds[i];
			printf("rntypes:%d\n",rnd);
			tk.type = (int)rnd;
			if(i == 0 || i == 16) //two dummy tasks
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
		
		//add edges to the hybrid graph
		std::pair<vertex_iter, vertex_iter> vp; 
		vp = vertices(testJob.g);

		int p[4] = {6,8,11,13};//{5, 7, 10, 12};
		for(int i=0; i<4; i++)
			add_edge(p[i], p[i]+1, testJob.g);
		int start[5] = {2,4,5,6,8};//{1, 3, 4, 5, 7};
		for(int i=0; i<5; i++)
			add_edge(1, start[i], testJob.g);//(0, start[i], testJob.g);
		/*add_edge(6, 9, testJob.g);
		add_edge(8, 9, testJob.g);
		add_edge(9, 10, testJob.g);
		add_edge(9, 12, testJob.g);
		add_edge(1, 2, testJob.g);
		add_edge(2, 14, testJob.g);
		add_edge(3, 11, testJob.g);
		add_edge(4, 10, testJob.g);
		add_edge(11, 14, testJob.g);
		add_edge(13, 14, testJob.g);*/
		add_edge(7, 10, testJob.g);
		add_edge(9, 10, testJob.g);
		add_edge(10, 12, testJob.g);//10, 11
		add_edge(10, 13, testJob.g);
		add_edge(2, 3, testJob.g);
		add_edge(3, 15, testJob.g);
		add_edge(4, 12, testJob.g);
		add_edge(5, 11, testJob.g);
		add_edge(12, 15, testJob.g);
		add_edge(14, 15, testJob.g);
		add_edge(0, 1, testJob.g);
		add_edge(15, 16, testJob.g);
	}
	else if(strcmp(argv[2],"test") == 0) {
		int rnds[8] = {1, 2, 3, 0, 0, 1, 2,3};
		for(int i=0; i<8; i++)
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
			if(i == 0 || i == 7) //two dummy tasks
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
	
		//add edges to the graph
		add_edge(1,2,testJob.g);
		add_edge(1,3,testJob.g);
		add_edge(2,4,testJob.g);
		add_edge(3,4,testJob.g);
		add_edge(4,5,testJob.g);
		add_edge(3,6,testJob.g);
		add_edge(4,6,testJob.g);
		add_edge(0,1,testJob.g); //a dummy entry node
		add_edge(5,7,testJob.g); //a dummy exit node
		add_edge(6,7,testJob.g);
	}
	else if(strcmp(argv[2],"Montage") == 0) {
		testJob.type = Montage;
		int rnds[] = {0,1,2,3,0,1,2,3,0,1,2,3,0,1,2,3,0,1,2,3,0,1};//22
		for(int i=0; i<20+2; i++){
			taskVertex tk;
			tk.name = i;
			tk.assigned_type = tk.prefer_type = tk.mark = 0;
			tk.cost = tk.start_time = tk.end_time =tk.sub_deadline = tk.restTime = 0;
			tk.EST = tk.LFT = 0;
			tk.readyCountDown = -1; //
			tk.status = not_ready;
			tk.estTime = new double[types];
			//int rnd = (double)rand() / RAND_MAX * types;
			int rnd = rnds[i];
			tk.type = rnd;
			if(i == 0 || i == 21) //two dummy tasks
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
		//add edges to the graph
		int l1[4] = {1,2,3,4};
		int l2[6] = {5,6,7,8,9,10};
		int l3[4] = {13,14,15,16};
		for(int i=0; i<4; i++) //for the dummy entry node
			add_edge(0,l1[i],testJob.g);
		add_edge(20,21,testJob.g); //for the exit dummy node
		for(int i=0; i<4; i++) {
			add_edge(l1[i],l3[i],testJob.g);
			add_edge(l1[i],l2[i],testJob.g);
			add_edge(l1[i],l2[i]+1,testJob.g);
		}
		add_edge(4,10,testJob.g);
		add_edge(2,5,testJob.g);
		add_edge(2,9,testJob.g);
		for(int i=0; i<6; i++)
			add_edge(l2[i],11,testJob.g);
		add_edge(11,12,testJob.g);
		for(int i=0; i<4; i++) {
			add_edge(12,l3[i],testJob.g);
			add_edge(l3[i],17,testJob.g);
		}
		add_edge(17,18,testJob.g);
		add_edge(18,19,testJob.g);
		add_edge(19,20,testJob.g);
	}
	else if(strcmp(argv[2], "Ligo") == 0) {
		testJob.type = Ligo;
		int rnds[] = {0,1,2,3,0,1,2,3,0,1,2,3,0,1,2,3,0,1,2,3,0,1,2,3,0,1,2,3,0,1,2,3,0,1,2,3,0,1,2,3,0,1};//42
		for(int i=0; i<40+2; i++){
			taskVertex tk;
			tk.name = i;
			tk.assigned_type = tk.prefer_type = tk.mark = 0;
			tk.cost = tk.start_time = tk.end_time =tk.sub_deadline = tk.restTime = 0;
			tk.EST = tk.LFT = 0;
			tk.readyCountDown = -1; //
			tk.status = not_ready;
			tk.estTime = new double[types];
			//int rnd = (double)rand() / RAND_MAX * types;
			int rnd = rnds[i];
			tk.type = rnd;
			if(i == 0 || i == 41) //two dummy tasks
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
		for(int i = 1; i<10; i++) //for the dummy entry node
			add_edge(0,i,testJob.g);
		add_edge(39,41,testJob.g); //for the dummy exit node
		add_edge(40,41,testJob.g);

		for(int i =1; i<10; i++) {
			add_edge(i, i+9, testJob.g); 
			add_edge(i+20, i+29, testJob.g);
		}
		for(int i=10; i<15; i++) {
			add_edge(i,19,testJob.g);
			add_edge(i+20,39,testJob.g);
		}
		for(int i=15; i<19; i++) {
			add_edge(i,20,testJob.g);
			add_edge(i+20, 40, testJob.g);
		}
		for(int i=21; i<26; i++)
			add_edge(19, i, testJob.g);
		for(int i=26; i<30; i++)
			add_edge(20,i,testJob.g);

	}
	else if(strcmp(argv[2], "Cybershake") == 0) {
		testJob.type = Cybershake;
		int rnds[] = {0,1,2,3,0,1,2,3,0,1,2,3,0,1,2,3,0,1,2,3,0,1};//22
		for(int i=0; i<20+2; i++){
			taskVertex tk;
			tk.name = i;
			tk.assigned_type = tk.prefer_type = tk.mark = 0;
			tk.cost = tk.start_time = tk.end_time =tk.sub_deadline = tk.restTime = 0;
			tk.EST = tk.LFT = 0;
			tk.readyCountDown = -1; //
			tk.status = not_ready;
			tk.estTime = new double[types];
			//int rnd = (double)rand() / RAND_MAX * types;
			int rnd = rnds[i];
			tk.type = rnd;
			if(i == 0 || i == 21) //two dummy tasks
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
		add_edge(0,1,testJob.g); //for dummy entry node
		add_edge(0,2,testJob.g);
		add_edge(20,21,testJob.g);//for the dummy exit node
		add_edge(11,21,testJob.g);

		for(int i=3; i<7; i++) {
			add_edge(1,i,testJob.g);
			add_edge(i,11,testJob.g);
			add_edge(i,i+9,testJob.g);
		}
		for(int i=7; i<11; i++) {
			add_edge(2,i,testJob.g);
			add_edge(i,11,testJob.g);
			add_edge(i,i+9,testJob.g);
		}
		for(int i=12; i<20; i++)
			add_edge(i,20,testJob.g);
	}
	else if(strcmp(argv[2],"single") == 0) {
		testJob.type = single;
		int rnds[3] = {1, 0, 3};
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
	
	//deadline assignment for each class of workflows
	//initially assign according to the minimum execution time
	std::pair<vertex_iter, vertex_iter> vp; 
	vp = vertices(testJob.g);
	std::clock_t starttime = std::clock();
	if(strcmp(argv[1], "virtualcluster") == 0 || strcmp(argv[1], "autoscaling") == 0) {
		for(; vp.first!=vp.second; ++vp.first) //edge weight for communication cost
		{
			Vertex v1 = *vp.first;
			testJob.g[v1].prefer_type = types-1;
		}
			
		testJob.deadline_assign();

		//task configuration, find the prefered VM type for tasks
		vp = vertices(testJob.g);
		for(; vp.first != vp.second; ++vp.first)
			testJob.g[*vp.first].instance_config();
	}
	
	if(strcmp(argv[1], "virtualcluster") == 0) {

		VirtualCluster alg1;
		alg1.Simulate_SA(testJob);
	}
	else if(strcmp(argv[1], "autoscaling") == 0){
		//remove the dummy tasks which are added for deadline assign
		vertex_iter vi, vi_end, next;
		boost::tie(vi, vi_end) = vertices(testJob.g);
		clear_vertex(*vi,testJob.g);
		remove_vertex(*vi,testJob.g);
		boost::tie(vi, vi_end) = vertices(testJob.g);
		clear_vertex(*(--vi_end),testJob.g);
		remove_vertex(*(vi_end),testJob.g);

		AutoScaling alg2;
		alg2.Simulate(testJob);
	}
	else if(strcmp(argv[1], "consolidation") == 0){
		GanttConsolidation alg3;
		//Initialization
		if(strcmp(argv[12],"bestfit") == 0) alg3.Initialization(&testJob,1);
		else if(strcmp(argv[12],"worstfit") == 0) alg3.Initialization(&testJob,2);
		else if(strcmp(argv[12],"mostefficient") == 0) alg3.Initialization(&testJob,3);
		
		vp = vertices(testJob.g);
		for(; vp.first!=vp.second; ++vp.first) 
			testJob.g[*vp.first].sub_deadline = testJob.deadline;
		
		alg3.Simulate(testJob);
	}

	std::clock_t endtime = std::clock();
	double timeelapsed = (double)(endtime - starttime) / (double)CLOCKS_PER_SEC;
	printf("Time elapsed for the experiment is: %4f\n", timeelapsed);

	return 0;
}