#include "stdafx.h"
#include "ConsolidateOperators.h"

double opSplit(vector<VM*>* VM_queue, vector<Job*> jobs)
{
	bool dosplit = false;
	double costsplit = 0;

	//first consider split the tasks on the same VM but with very large gap
	//in this case, can split the tasks to different VMs to use their residual time
	return 0;
}