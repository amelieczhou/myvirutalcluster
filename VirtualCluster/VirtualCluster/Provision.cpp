#include "stdafx.h"
#include "Provision.h"
#include <cmath>
#include <cstdlib>
#include <time.h>
//#include <iostream>

//calculate the probability of response time less than deadline given m
double function(double lamda, double miu, double deadline, int m)
{
	double ru = lamda/miu;
	double y = 0;
	for(int n = 0; n<m; n++)
	{
		y += std::pow(ru, n)/factorial(n);
	}
	double P0 = y + (double)m * std::pow(ru, m) / (factorial(m) * (m-ru));
	P0 = std::pow(P0, -1);

	/*double z = (1-std::exp(-miu * deadline))*y + 
		(double)m * std::pow(ru,m) /(factorial(m)*(1.0-(double)m+ru)) * ((1-std::exp((ru-(double)m)*miu*deadline))/(m-ru) -1 + std::exp(-miu*deadline));
	double result = p0 * z;*/
	double Pd = P0 * std::pow(ru,m) / (factorial(m) * (1 - ru / m));
	double result, tmp;
	tmp = 1 - std::exp(-miu*deadline);
	if(m == (ru + 1))
		result = tmp - Pd*miu*deadline*std::exp(-miu*deadline);
	else {
		result = tmp - Pd*std::exp(-miu*deadline*(m-ru))*(1-std::exp(-miu*deadline*(1-m+ru)))/(1-m+ru);
	}

	//double tmp = (1 - Pd) * miu * std::exp(-miu*deadline);
	//double result = Pd / (1 - m + ru)*(m - ru)*miu*(std::exp(-(m*miu-lamda)*deadline) - std::exp(-miu*deadline));
	//result += tmp;
	return result;
}
double stationary_p(double lamda, double miu, int m, int i)
{
	if(i==0) {
		double r1 = 0;
		for(int j=0; j<m+1; j++) {
			double r2 = 0;
			for(int k=0; k<m-j+1; k++) {
				r2 += pow(lamda/miu, k)*std::exp(-lamda/miu)/factorial(k);
			}
			r1 += stationary_p(lamda, miu, m, j) * r2;
		}
		return r1;		
	}
	else {
		double r1 = 0;
		for(int j=0; j<i+m+1; j++) {
			r1 += stationary_p(lamda, miu, m, j) * pow(lamda/miu, i+m-j)*std::exp(-lamda/miu)/factorial(i+m-j);
		}
		return r1;
	}
}
double wait_distribution(double lamda, double miu, double t, int m)
{
	bool MMC = false; //only one of them can be true, pick one
	bool MDC = true; //
//	if(MMC) {
		double ru = lamda/m/miu;
		double y = 0;
		double a = m*ru;
		for(int k = 0; k<m; k++)
		{
			double b = std::pow(a,k);
			double c = factorial(k);
			y += (b/c);
		}
		double P0 = y + std::pow(m*ru, m) / (factorial(m) * (1-ru));
		P0 = std::pow(P0, -1);
		double Pd = P0 * std::pow(m*ru,m) / (factorial(m) * (1 - ru));
		double result = 1-Pd*std::exp(-(m*miu-lamda)*t);
		return result;
//	}
	//if(MDC) {
	//	double result;
	//	double D = 1/miu;
	//	double k = ceil(t/D);
	//	result = std::exp(-lamda*(k*D-t));
	//	int count = k*m;
	//	double tmp = 0;
	//	for(int i=0; i<count; i++) {
	//		int i_count = k*m-i;
	//		double Q_kcj =0;
	//		for(int j=0; j<i_count; j++) {
	//			Q_kcj += stationary_p(lamda, miu, m, j);
	//		}
	//		tmp += Q_kcj * pow(lamda*(k/miu - t), i) / factorial(i);
	//	}
	//	result *= tmp;
	//	return result;
	//}
}
//return the m value when the probability is no less than p
int provision(double lamda, double miu, double deadline, double p)
{
	//int m = std::floor(lamda/miu) + 1;
	//
	//double F = function(lamda, miu, deadline, m);
	//double F1 = function(lamda, miu, deadline, m+1);
	//while((F1 - F) > 0.0001)
	//{
	//	m += 1;
	//	//if(m == (lamda/miu + 1)) m++;
	//	F = function(lamda, miu, deadline, m);
	//	F1 = function(lamda, miu, deadline, m+1);
	//}
	//double test = 1e1;//=10
	//double test1 = std::exp(1.0);//=2.7182818284590451
	double exeT = -1/miu/std::log(std::exp(1.0))*std::log(1-p);
	double t = deadline - 1.0/miu ;
	//approximate of M/D/c
	t *= 2;
	//t*=1.5;
	//p = p*std::exp(1.0)/(std::exp(1.0)-1.0);
	int m = std::floor(lamda/miu) + 1;
	//test
	//lamda = 8; miu = 1; t= 0.917*1.5; m=10;
	double F = wait_distribution(lamda, miu, t, m); //Pdelay = 1 - W(0) = 1-0.17=0.83
	while(F<p) {
		m += 1;
		F = wait_distribution(lamda, miu, t, m);
	}
	//m = std::floor(lamda/miu) + 1;
	printf("the probability of waiting time less than %4f is %4f\n", t, F);
	return m;
}

double factorial(int n)
{
  return (n == 1 || n == 0) ? 1 : factorial(n - 1) * n;
}

double rnd_exponential(double lamda, double t)
{
	//unsigned seed = t;
	//srand( (unsigned)time( NULL ) + seed);
	//struct timeval tv;
	//gettimeofday(&tv, NULL);
	
	//srand( (unsigned)time( NULL ));
	double rnd = (double)rand() / (RAND_MAX + 1);
	//std::cerr<<"exponential rnd is "<<rnd<<std::endl;
	double result = -1 / lamda * log(rnd);
	return result;
}