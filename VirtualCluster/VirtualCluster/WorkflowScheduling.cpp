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
double Times[4][types] = {{200,105,58,34},{140,75,43,27},{80,45,28,19},{40,25,18,14}};//{{202,106,58,34},{141,76,43,27},{81,45,28,19},{40,25,18,14}};//real cloud
double budget;

int main(int argc, char** argv)
{
	int depth = atoi(argv[3]);
	int width = atoi(argv[4]);
	deadline = atof(argv[5]);
	double p_rate = atof(argv[15]);
	if(strcmp(argv[6],"dynamic")==0)
		DynamicSchedule = true;
	else DynamicSchedule = false;
	max_t = atof(argv[7]);
	chk_period = atof(argv[8]);
	if(strcmp(argv[9], "sharing")==0)
		sharing = true;
	lamda = atof(argv[10]);
	QoS = atof(argv[11]);
	budget = atof(argv[18]);

	if(p_rate != 0.95){
		for(int i=0; i<4; i++)
			for(int j=1; j<types; j++)
				Times[i][j]=Times[i][0]*p_rate/pow(2.0,j) + Times[i][0]*(1 - p_rate);
	}

	//organize the inputs: workflows, VMs, user defined parameters	
	Job testJob(pipeline,deadline,lamda);
	Job testJob1(pipeline,deadline,lamda);
	testJob.budget = budget;
	testJob1.budget = budget;
			
	if(strcmp(argv[2], "pipeline") == 0)
	{
		testJob.type = pipeline;
		int rnds[] = {0,0,3,3,1,2,1,0,0};//two dummy tasks
		//generate DAG
		for(int i=0; i<depth+2; i++) //two dummy tasks
		{
			taskVertex tk;
			tk.name = i;			
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
		int rnds[] = {0,1,2,3,0,1,2,3,0,1,2,3,0,1,2,3,0,1,2,3,0,1};//[3.7,4.7]=4.2
		for(int i=0; i<20+2; i++){
			taskVertex tk;
			tk.name = i;			
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
	else if(strcmp(argv[2], "Ligo") == 0) {//longest running time is 1080
		testJob.type = Ligo;
		int rnds[] = {0,1,2,3,0,1,2,3,0,1,2,3,0,1,2,3,0,1,2,3,0,1,2,3,0,1,2,3,0,1,2,3,0,1,2,3,0,1,2,3,0,1};//[7.3,9.5]=8.4
		for(int i=0; i<40+2; i++){
			taskVertex tk;
			tk.name = i;			
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
	else if(strcmp(argv[2],"Ligo+Montage")==0){
		testJob.type = Ligo;
		testJob.budget = 9.6;
		int rnds[] = {0,1,2,3,0,1,2,3,0,1,2,3,0,1,2,3,0,1,2,3,0,1,2,3,0,1,2,3,0,1,2,3,0,1,2,3,0,1,2,3,0,1};//42
		for(int i=0; i<40+2; i++){
			taskVertex tk;
			tk.name = i;			
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

		testJob1.type = Montage;
		testJob1.budget = 5.5;
		int rrnds[] = {0,1,2,3,0,1,2,3,0,1,2,3,0,1,2,3,0,1,2,3,0,1};//22
		for(int i=0; i<20+2; i++){
			taskVertex tk;
			tk.name = i;			
			tk.estTime = new double[types];
			//int rnd = (double)rand() / RAND_MAX * types;
			int rnd = rrnds[i];
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
			add_vertex(tk, testJob1.g);
		}
		//add edges to the graph
		int l1[4] = {1,2,3,4};
		int l2[6] = {5,6,7,8,9,10};
		int l3[4] = {13,14,15,16};
		for(int i=0; i<4; i++) //for the dummy entry node
			add_edge(0,l1[i],testJob1.g);
		add_edge(20,21,testJob1.g); //for the exit dummy node
		for(int i=0; i<4; i++) {
			add_edge(l1[i],l3[i],testJob1.g);
			add_edge(l1[i],l2[i],testJob1.g);
			add_edge(l1[i],l2[i]+1,testJob1.g);
		}
		add_edge(4,10,testJob1.g);
		add_edge(2,5,testJob1.g);
		add_edge(2,9,testJob1.g);
		for(int i=0; i<6; i++)
			add_edge(l2[i],11,testJob1.g);
		add_edge(11,12,testJob1.g);
		for(int i=0; i<4; i++) {
			add_edge(12,l3[i],testJob1.g);
			add_edge(l3[i],17,testJob1.g);
		}
		add_edge(17,18,testJob1.g);
		add_edge(18,19,testJob1.g);
		add_edge(19,20,testJob1.g);
	}
	//deadline assignment for each class of workflows
	//initially assign according to the minimum execution time
	std::pair<vertex_iter, vertex_iter> vp, vvp;
	vp = vertices(testJob.g);
	if(strcmp(argv[2],"Ligo+Montage")==0)
		vvp = vertices(testJob1.g);
	
	std::clock_t starttime = std::clock();
	if(strcmp(argv[1], "virtualcluster") == 0 ) {
		for(; vp.first!=vp.second; ++vp.first) {//edge weight for communication cost		
			Vertex v1 = *vp.first;
			testJob.g[v1].prefer_type = types-1;
		}
			
		testJob.deadline_assign();

		//task configuration, find the prefered VM type for tasks
		vp = vertices(testJob.g);
		for(; vp.first != vp.second; ++vp.first)
			testJob.g[*vp.first].instance_config();

		if(strcmp(argv[2],"Ligo+Montage")==0){
			for(; vvp.first != vvp.second; ++vvp.first)	{
				Vertex v1 = *vvp.first;
				testJob1.g[v1].prefer_type = types-1;
			}
			testJob1.deadline_assign();

			vvp = vertices(testJob1.g);
			for(; vvp.first!=vvp.second; ++vvp.first)
				testJob1.g[*vvp.first].instance_config();
		}
	}
	
	if(strcmp(argv[1], "virtualcluster") == 0) {

		VirtualCluster alg1;
		alg1.Simulate_SA(testJob);
	}
	else if(strcmp(argv[1], "autoscaling") == 0){
		bool time;
		if(strcmp(argv[17],"istime")==0)
			time = true;
		else if(strcmp(argv[17],"cost")==0)
			time = false;
		
		if(strcmp(argv[2],"Ligo+Montage")==0){
			AutoScaling alg2;
			alg2.Simulate(testJob,testJob1,time);
		}else{
			AutoScaling alg2;
			alg2.Simulate(testJob,time);
		}
	}
	else if(strcmp(argv[1], "consolidation") == 0){
		GanttConsolidation alg3;			
		int interval = atoi(argv[13]);
		bool rule;
		if(strcmp(argv[14],"rule")==0)
			rule = true;
		else if(strcmp(argv[14],"random")==0)
			rule = false;
		else return 1;
//		int threshold = atoi(argv[15]);
//		int threshold = 1;//its not a sensitive parameter
		bool estimate;
		if(strcmp(argv[16],"estimate")==0)
			estimate = true;
		else if(strcmp(argv[16],"real")==0)
			estimate = false;
		else return 1;
		bool time;
		if(strcmp(argv[17],"istime")==0)
			time = true;
		else if(strcmp(argv[17],"cost")==0)
			time = false;
		if(strcmp(argv[2],"Ligo+Montage")==0){
			//simulation
			if(strcmp(argv[12],"bestfit") == 0) alg3.Simulate(testJob,testJob1,1,interval,rule,estimate,time);
			else if(strcmp(argv[12],"worstfit") == 0) alg3.Simulate(testJob,testJob1,2,interval,rule,estimate,time);
			else if(strcmp(argv[12],"mostefficient") == 0) alg3.Simulate(testJob,testJob1,3,interval,rule,estimate,time);
		}else{
			//simulation
			if(strcmp(argv[12],"bestfit") == 0) alg3.Simulate(testJob,1,interval,rule,estimate,time);
			else if(strcmp(argv[12],"worstfit") == 0) alg3.Simulate(testJob,2,interval,rule,estimate,time);
			else if(strcmp(argv[12],"mostefficient") == 0) alg3.Simulate(testJob,3,interval,rule,estimate,time);		
		}
	}

	std::clock_t endtime = std::clock();
	double timeelapsed = (double)(endtime - starttime) / (double)CLOCKS_PER_SEC;
	printf("Time elapsed for the experiment is: %4f\n", timeelapsed);

	return 0;
}