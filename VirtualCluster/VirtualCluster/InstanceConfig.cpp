#include "stdafx.h"
#include "InstanceConfig.h"
#include <boost/graph/dag_shortest_paths.hpp>
#include <limits>

void taskVertex::instance_config(){
	for(int i=0; i<types; i++) //VM types from cheap to expensive
		if(this->estTime[i]  < (this->LFT - this->EST)) //data transfer time?? + OnDemandLag
		{
			prefer_type = i;
			break;
		}
}

void Job::reset()// reset the parameters of tasks in the Job
{

}
bool myfunction(taskVertex* a, taskVertex* b)
{
	return (a->sub_deadline < b->sub_deadline);
}
std::vector<int> Job::find_CP() //find the critical path of the Job, for deadline assignment
{
	std::vector<int> cp;

	property_map<Graph, vertex_distance_t>::type d_map = get(vertex_distance, this->g);
	property_map<Graph, edge_weight_t>::type w_map = get(edge_weight, g);
	typedef graph_traits<Graph>::vertex_descriptor vertex_descriptor;
	vertex_descriptor s = *(vertices(this->g).first);

#if defined(BOOST_MSVC) && BOOST_MSVC <= 1300
	std::vector<default_color_type> color(num_vertices(g));
	std::vector<std::size_t> pred(num_vertices(g));
	default_dijkstra_visitor vis;
	std::less<int> compare;
	closed_plus<int> combine;
	
	//dag_shortest_paths(this->g, s, distance_map(d_map));	
	dag_shortest_paths(g, s, d_map, w_map, &color[0], &pred[0], 
     vis, compare, combine, (std::numeric_limits<int>::max)(), 0);
#else
	std::vector<vertex_descriptor> p(num_vertices(g));
	//all vertices start out as their own parent
	typedef graph_traits<Graph>::vertices_size_type size_type;
	for (size_type i =0; i< num_vertices(g); ++i)
		p[i] = i;
    std::vector<double> d(num_vertices(g));   
	//dag_shortest_paths(g, s, distance_map(d_map));
	dag_shortest_paths(g, s, predecessor_map(&p[0]).distance_map(&d[0]));    
#endif
	
	int k = num_vertices(g);
	
	for(size_type i=(num_vertices(g)-1);i !=0;)
	{
		cp.push_back(i); // assuming one exit and one entrance node
		i = p[i];
	}
	//cp.pop_back();
	cp.push_back(0); // add the source task as the first node in cp
	return cp;//the last node is a virtual node
}

void Job::deadline_assign()//each task is associated with a sub-deadline, a start time and an end time
{
	//deadline assignment, according to minimum execution time
	
	//new deadline assign method
	std::pair<vertex_iter, vertex_iter> vp; 
	vp = vertices(this->g);
	int size = num_vertices(this->g);
	this->g[0].sub_deadline = this->g[0].EST = 0;
	this->g[size-1].sub_deadline = this->g[size-1].LFT = this->deadline;
	this->g[0].mark = this->g[size-1].mark = 1;
	for(int i=1; i<size-1; i++)
		this->g[i].mark = 0;

	for(; vp.first!=vp.second; ++vp.first)
	{
		Vertex v = *vp.first;
		this->g[v].EST = EST(this->g[v],*this);
	}
	vp = vertices(this->g);
	for(;vp.first!=vp.second; --vp.second)
	{
		Vertex v = *vp.second-1;
		this->g[v].LFT = LFT(this->g[v],*this);
	}
	AssignParents(&this->g[size-1],this);
}

int* Job::find_ServerGroup() //perform after deadline assignment and instance configuration
{
	/*
	int* VMs = new int[types];
	int* tempVMs = new int[types];
	for(int i=0; i<types; i++)
		VMs[i] = tempVMs[i] = 0;
	
	//each task belong to a time interver [start_time, end_time]
	std::pair<vertex_iter, vertex_iter> vp; 
	vp = vertices(this->g);
	this->g[*vp.first+1].status = ready; //the first one is dummy task
	std::vector<taskVertex*> ready_task;
	for(int i=0; i < (*vp.second - *vp.first ); i++)
	{
		bool tag = true;
		//get parent vertices
		in_edge_iterator in_i, in_end;
		edge_descriptor e;
		for (boost::tie(in_i, in_end) = in_edges(i, this->g); in_i != in_end; ++in_i) 
		{
			e = *in_i;
			Vertex src = source(e, this->g);					
			if(this->g[src].status != finished)
			{
				tag = false;
				//break;
			}
		}
		if(this->g[i].status == ready || tag && this->g[i].status != scheduled && this->g[i].status != finished){
			ready_task.push_back(&this->g[i]);							
		}
	}
	return VMs;
	*/
	int size_of_building = num_vertices(this->g);
	std::vector<Building>* buildlists = new std::vector<Building>[types];
	for(int i=0; i<size_of_building; i++)
		buildlists[this->g[i].prefer_type].push_back(Building(this->g[i].EST,this->g[i].sub_deadline,1));

	int* VMs = new int[types];
	for(int i=0; i<types; i++)
	{
		std::vector<Point> result;
		result = merge_sort(buildlists[i]);
		int size = result.size();
		if(size>0){
			int max = result[0].height;
			for(int j=1; j<size; j++)
				if(result[j].height>max)
					max = result[j].height;
			VMs[i] = max;
		}
		else{ VMs[i]=0;}
	}
	return VMs;
}
int* Job::find_ServerGroup_SA(){
	
	std::vector<Building>* buildlists = new std::vector<Building>[types];
	for(int i=0; i<types; i++) {
		int size_slots = this->templates[i].size();
		for(int j=0; j<size_slots; j++)
			buildlists[i].push_back(Building(this->templates[i][j].first,this->templates[i][j].second,1.0));
	}

	int* VMs = new int[types];
	for(int i=0; i<types; i++)
	{
		std::vector<Point> result;
		result = merge_sort(buildlists[i]);
		int size = result.size();
		if(size>0){
			int max = result[0].height;
			for(int j=1; j<size; j++)
				if(result[j].height>max)
					max = result[j].height;
			VMs[i] = max;
		}
		else{ VMs[i]=0;}
	}
	return VMs;
}
double MET(taskVertex tk)
{
	return tk.estTime[tk.prefer_type];
}
double MTT(taskVertex* tk1, taskVertex* tk2)
{
	return 0;
}
double EST(taskVertex tk, Job job)
{
	double max = 0;
	double tmp;
	std::vector<Vertex> parents;

	//get parent vertices
	in_edge_iterator in_i, in_end;
	edge_descriptor e;
	for (boost::tie(in_i, in_end) = in_edges(tk.name, job.g); in_i != in_end; ++in_i) 
	{
		e = *in_i;
		Vertex src = source(e, job.g);		
		parents.push_back(src);
		int test;
		if(job.g[src].EST == 0)
			test = 1;
	}
	if(parents.size() > 0)
		max = job.g[parents[0]].EST + job.g[parents[0]].estTime[job.g[parents[0]].prefer_type];
	else max = tk.EST;
	for(int i=1; i<parents.size(); i++)
	{
		tmp = job.g[parents[i]].EST + job.g[parents[i]].estTime[job.g[parents[i]].prefer_type];
		if(tmp > max) max = tmp;
	}
	return max;
}
double LFT(taskVertex tk, Job job)
{
	double min = 0;
	double tmp;
	std::vector<Vertex> children;

	//get children vertices
	out_edge_iterator out_i, out_end;
	edge_descriptor e;
	for (boost::tie(out_i, out_end) = out_edges(tk.name, job.g); out_i != out_end; ++out_i) 
	{
		e = *out_i;
		Vertex tgt = target(e,job.g);
		children.push_back(tgt);
	}
	if(children.size() > 0)
		min = job.g[children[0]].LFT - job.g[children[0]].estTime[job.g[children[0]].prefer_type];
	else min = tk.LFT;
	for(int i=1; i<children.size(); i++)
	{
		tmp =  job.g[children[i]].LFT - job.g[children[i]].estTime[job.g[children[i]].prefer_type];
		if(tmp < min) min = tmp;
	}
	return min;
}
void AssignParents(taskVertex* tk, Job* job)
{	
	while(has_unassigned_parent(tk,job))
	{
		taskVertex* ti = tk;
		std::deque<Vertex> PCP;
		while(has_unassigned_parent(ti,job))
		{
			Vertex v = CriticalParent(ti,job);
			PCP.push_front(v);
			ti = &job->g[v];
		}
		AssignPath(PCP, job);
		int size = PCP.size();
		for(int iter1=0; iter1 <size; iter1++)
		{
			taskVertex tk1 = job->g[PCP[iter1]];
			//update EST for unassigned successors
			//get children vertices
			out_edge_iterator out_i, out_end;
			edge_descriptor e;
			for (boost::tie(out_i, out_end) = out_edges(tk1.name, job->g); out_i != out_end; ++out_i) 
			{
				e = *out_i;
				Vertex tgt = target(e,job->g);
				if(job->g[tgt].mark == 0) {
					if(tk1.EST + tk1.restTime > job->g[tgt].EST)
						job->g[tgt].EST = tk1.EST + tk1.restTime;

					taskVertex* task = &job->g[tgt];
					update_EST(task,job);
				}
			}
			//update LFT for unassigned predecessors
			//get parent vertices
			in_edge_iterator in_i, in_end;
			edge_descriptor e1;
			for (boost::tie(in_i, in_end) = in_edges(tk1.name, job->g); in_i != in_end; ++in_i) 
			{
				e1 = *in_i;
				Vertex src = source(e1, job->g);	
				if(job->g[src].mark == 0){
					if(tk1.LFT - tk1.restTime < job->g[src].LFT)
						job->g[src].LFT = tk1.LFT - tk1.restTime;

				taskVertex* task = &job->g[src];
				update_LFT(task,job);
				}
			}
			AssignParents(&tk1,job);
		}
	}
}
void update_EST(taskVertex* tk, Job* job)//update the EST of tk's unassigned children
{
	out_edge_iterator out_i, out_end;
	edge_descriptor e;
	for (boost::tie(out_i, out_end) = out_edges(tk->name, job->g); out_i != out_end; ++out_i) 
	{
		e = *out_i;
		Vertex tgt = target(e,job->g);
		if(job->g[tgt].mark == 0){
			if(tk->EST+tk->estTime[tk->prefer_type]>job->g[tgt].EST)
				job->g[tgt].EST = tk->EST+tk->estTime[tk->prefer_type];
			update_EST(&job->g[tgt],job);
		}
	}
}
void update_LFT(taskVertex* tk, Job* job)//update the LFT of tk's unassigned parents
{
	in_edge_iterator in_i, in_end;
	edge_descriptor e;
	for (boost::tie(in_i, in_end) = in_edges(tk->name, job->g); in_i != in_end; ++in_i) 
	{
		e = *in_i;
		Vertex src = source(e,job->g);
		if(job->g[src].mark == 0){
			if(tk->LFT - tk->estTime[tk->prefer_type] < job->g[src].LFT)
				job->g[src].LFT = tk->LFT - tk->estTime[tk->prefer_type];
			update_LFT(&job->g[src],job);
		}
	}
}
Vertex CriticalParent(taskVertex* tk, Job* job)
{
	double max = 0;
	Vertex maxP;
	//get parent vertices
	in_edge_iterator in_i, in_end;
	edge_descriptor e;
	for (boost::tie(in_i, in_end) = in_edges(tk->name, job->g); in_i != in_end; ++in_i) 
	{
		e = *in_i;
		Vertex src = source(e, job->g);		
		if(job->g[src].EST + job->g[src].estTime[job->g[src].prefer_type] > max && job->g[src].mark == 0)
		{
			maxP = src;
			max = job->g[src].EST + job->g[src].estTime[job->g[src].prefer_type];
		}
	}
	return maxP;
}
bool has_unassigned_parent(taskVertex* tk, Job* job)
{
	std::vector<Vertex> unassigned_parent;
	//get parent vertices
	in_edge_iterator in_i, in_end;
	edge_descriptor e;
	for (boost::tie(in_i, in_end) = in_edges(tk->name, job->g); in_i != in_end; ++in_i) 
	{
		e = *in_i;
		Vertex src = source(e, job->g);	
		if(job->g[src].mark == 0)
			return true;
	}
	return false;
}
bool has_unassigned_child(taskVertex* tk, Job* job)
{
	std::vector<Vertex> unassigned_child;
	//get children vertices
	out_edge_iterator out_i, out_end;
	edge_descriptor e;
	for (boost::tie(out_i, out_end) = out_edges(tk->name, job->g); out_i != out_end; ++out_i) 
	{
		e = *out_i;
		Vertex tgt = target(e,job->g);
		if(job->g[tgt].mark == 0)
			return true;
	}
	return false;
}
void AssignPath(std::deque<Vertex> PCP, Job* job)
{
	/*
	//the following is the so-called optimized path assigning policy
	double bestCost = std::numeric_limits<double>::max();
	bool best = false; //if found a best schedule
	std::deque<Vertex>::iterator iter = PCP.begin();
	Vertex t = PCP.front();
	int s = types-1;
	while(t!= NULL)
	{
		job->g[t].prefer_type = s; //before move to the next slower service
		s -= 1;
		job->g[t].assigned_type = s; //after move
		if(s<0 || ((job->g[t].EST+job->g[t].estTime[s])>job->g[t].LFT))
		{
			if(iter == PCP.begin())
				t = NULL;
			else{
			iter --;
			t = PCP[*iter];}
			continue;
		}
		if(iter == PCP.end())
		{
			double totalCost = 0;
			for(std::deque<Vertex>::iterator iter1=PCP.begin(); iter1 != PCP.end(); iter1++)
				totalCost += priceOnDemand[job->g[PCP[*iter1]].assigned_type];
			if(totalCost < bestCost)
			{
				best = true;
				for(std::deque<Vertex>::iterator iter1=PCP.begin(); iter1 != PCP.end(); iter1++)
					job->g[PCP[*iter1]].prefer_type = job->g[PCP[*iter1]].assigned_type;
			}
			t = PCP[*iter--];
		}
		t = PCP[*iter++];
	}
	if(best == false)
	{
		for(std::deque<Vertex>::iterator iter1=PCP.begin(); iter1 != PCP.end(); iter1++)
		{
			job->g[PCP[*iter1]].sub_deadline = job->g[PCP[*iter1]].EST + job->g[PCP[*iter1]].estTime[types-1];
			job->g[PCP[*iter1]].mark = 1;
		}
	}
	else
	{
		//set EST and sub-deadline 
		for(std::deque<Vertex>::iterator iter1=PCP.begin(); iter1 != PCP.end(); iter1++)
		{
			if(iter1==PCP.begin())
			{
				job->g[PCP[*iter1]].sub_deadline = job->g[PCP[*iter1]].EST + job->g[PCP[*iter1]].estTime[job->g[PCP[*iter1]].prefer_type];
				job->g[PCP[*iter1]].mark = 1;
			}
			else{
				job->g[PCP[*iter1]].EST = job->g[PCP[*iter1-1]].sub_deadline;
				job->g[PCP[*iter1]].sub_deadline = job->g[PCP[*iter1]].EST + job->g[PCP[*iter1]].estTime[job->g[PCP[*iter1]].prefer_type];
				job->g[PCP[*iter1]].mark = 1;
			}
		}
		//if the rest time is larger than 10% of the total time, proportionally assign to other tasks, otherwise give it the the last task
		double rest_time = job->g[PCP.back()].LFT - job->g[PCP.back()].sub_deadline;
		double longest_time = job->g[PCP.back()].sub_deadline - job->g[PCP.front()].EST;
		if( rest_time > longest_time*0.1) 
		{
			for(std::deque<Vertex>::iterator iter1=PCP.begin(); iter1 != PCP.end(); iter1++)
			{
				if(iter1==PCP.begin())
				{
					job->g[PCP[*iter1]].sub_deadline +=  rest_time*job->g[PCP[*iter1]].estTime[job->g[PCP[*iter1]].prefer_type]/longest_time;
				}
				else{
					job->g[PCP[*iter1]].EST = job->g[PCP[*iter1-1]].sub_deadline;
					job->g[PCP[*iter1]].sub_deadline +=  rest_time*job->g[PCP[*iter1]].estTime[job->g[PCP[*iter1]].prefer_type]/longest_time;
				}
			}
		}
		else{
			job->g[PCP.back()].sub_deadline = job->g[PCP.back()].LFT;
		}

	}*/
	double longest_time = job->g[PCP.back()].LFT - job->g[PCP.front()].EST;
	double total_execution = 0;
	int size = PCP.size();
	for(int iter=0; iter < size; iter++)
	{
		total_execution += job->g[PCP[iter]].estTime[job->g[PCP[iter]].prefer_type];
	}
	for(int iter=0; iter <size; iter++)
		job->g[PCP[iter]].restTime = longest_time*job->g[PCP[iter]].estTime[job->g[PCP[iter]].prefer_type]/total_execution; //the execution time assigned
	for(int iter=0; iter <size; iter++)
	{
		if(iter == 0)
		{
			job->g[PCP[iter]].sub_deadline = job->g[PCP[iter]].EST + job->g[PCP[iter]].restTime;
			job->g[PCP[iter]].mark = 1;
		}
		else{
			int prev_iter = iter-1;
			job->g[PCP[iter]].EST = job->g[PCP[prev_iter]].sub_deadline;
			job->g[PCP[iter]].restTime = longest_time*job->g[PCP[iter]].estTime[job->g[PCP[iter]].prefer_type]/total_execution;
			job->g[PCP[iter]].sub_deadline =  job->g[PCP[iter]].EST + job->g[PCP[iter]].restTime;
			job->g[PCP[size-iter-1]].LFT = job->g[PCP[size-prev_iter-1]].LFT - job->g[PCP[size-prev_iter-1]].restTime;
			job->g[PCP[iter]].mark = 1;
		}
	}
}

std::vector<Point> merge_sort(std::vector<Building> buildlist)
{
	int size_of_building = buildlist.size();
	std::vector<Building> Left_buildings, Right_buildings;
	std::vector<Point> Left, Right, result;

	if(size_of_building > 2)	{
		for(int i=0; i<size_of_building/2; i++){
			Left_buildings.push_back(buildlist[i]);
		}
		for(int i=size_of_building/2; i<size_of_building; i++)
			Right_buildings.push_back(buildlist[i]);
		Left = merge_sort(Left_buildings);
		Right = merge_sort(Right_buildings);
	}
	else if(size_of_building == 2){
		Point p1 = Point(buildlist[0].left,buildlist[0].height);
		Point p2 = Point(buildlist[0].right,0);
		Left.push_back(p1);
		Left.push_back(p2);
		Point p3 = Point(buildlist[1].left,buildlist[1].height);
		Point p4 = Point(buildlist[1].right,0);
		Right.push_back(p3);
		Right.push_back(p4);
	}
	else if(size_of_building == 1){
		Point p1 = Point(buildlist[0].left,buildlist[0].height);
		Point p2 = Point(buildlist[0].right,0);
		result.push_back(p1);
		result.push_back(p2);
		return result;
	}
	result = merge(Left,Right);
	return result;
}
std::vector<Point> merge(std::vector<Point> build1, std::vector<Point> build2)
{
	int size1 = build1.size();
	int size2 = build2.size();
	int i=0, j=0, r=size1+size2;
	std::vector<Point> result(r);

	for(int k=0; k<r; k++) {
		if(i < size1 && j < size2){
			if(build1[i].location < build2[j].location){
				result[k].location = build1[i].location;
				if(j >0)
					result[k].height = build1[i].height + build2[j-1].height;
				else result[k].height = build1[i].height;
				i ++;
			}
			else if(build1[i].location == build2[j].location){
				result[k].location = build1[i].location;
				result[k].height = build1[i].height;
				i ++; 
			}
			else{	result[k].location = build2[j].location;
				if(i>0)
					result[k].height = build2[j].height + build1[i-1].height;
				else result[k].height = build2[j].height;
				j ++;	}
		}
		else{ 
			if(i>size1-1){
			result[k].location = build2[j].location;
			result[k].height = build2[j].height;
			j ++;
		} else{
			result[k].location = build1[i].location;
			result[k].height = build1[i].height;
			i ++;	}		
		}
	}
	return result;
}
bool mycompare(std::pair<double, double> time1, std::pair<double, double> time2) {	
	if(time1.first < time2.first) 
		return true;
	else 
		return false;
}
/*
double Cal_Cost(std::vector<std::pair<double, double> > timeslots){
	
	for(int i=0; i<types; i++)
	{
		merge_sort_sharing(timeslots);
	}
}

std::vector<std::pair<double, double> > merge_sort_sharing(std::vector<std::pair<double, double>> buildlist){
	
	int size_of_building = buildlist.size();
	std::vector<std::pair<double, double> > Left, Right, result;
	if(size_of_building >= 2)	{
		for(int i=0; i<size_of_building/2; i++){
			Left_buildings.push_back(buildlist[i]);
		}
		for(int i=size_of_building/2; i<size_of_building; i++)
			Right_buildings.push_back(buildlist[i]);
		Left = merge_sort_sharing(Left_buildings);
		Right = merge_sort_sharing(Right_buildings);
	}
	else if(size_of_building == 1){
		return buildlist;
	}
	result = merge_sharing(Left,Right);
	return result;
	
}

std::vector<std::pair<double, double> > merge_sharing(std::vector<std::pair<double, double> > build1, std::vector<std::pair<double, double> > build2){
	int size1 = build1.size();
	int size2 = build2.size();
	int i=0, j=0, r=size1+size2;
	std::vector<Point> result1(r), result2(r), result(r);

	for(int k=0; k<r; k++) {
		if(i < size1 && j < size2){//left in front of right-> build1 in front of build2
			if(build1[i].second < build2[j].first){
				if(build2[])
				result1[k].location = result2[k].location = build1[i].location;
				result1[k].height = build1[i].height;
				result2[k].height = 0;
				i ++;
			}
			else if(build1[i].location == build2[j].location){
				result1[k].location = result2[k].location = build1[i].location;
				result1[k].height = result2[k].height = build1[i].height;
				i ++; 
			}
			else{	result1[k].location = result2[k].location = build2[j].location;
				result2[k].height = build2[j].height;
				result1[k].height = 0;
				j ++;	}
		}
		else{ 
			if(i>size1-1){
			result1[k].location = result2[k].location = build2[j].location;
			result2[k].height = build2[j].height;
			result1[k].height = 0;
			j ++;
		} else{
			result1[k].location = result2[k].location = build1[i].location;
			result1[k].height = build1[i].height;
			result2[k].height = 0;
			i ++;	}		
		}
	}
	for(int k=0; k<r; k++) //update height of the new building
	{
		result[k].height = result1[k].height + result2[k].height;
		result[k].location = result1[k].location;
	}
	return result;

}
*/