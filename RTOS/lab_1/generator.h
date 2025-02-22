#ifndef GENERATOR_H
#define GENERATOR_H

struct LCGParams
{
    unsigned long a, x, c, m;
};

unsigned long generate(LCGParams* params) 
{
    params->x = (params->a * params->x + params->c) % params->m;
    return params->x;
}

bool check_params(LCGParams* params)
{
	unsigned long a = params->a;
	unsigned long x = params->x;
	unsigned long c = params->c;
	unsigned long m = params->m;
	
	bool begin = (a > 0) && (x > 0) && (c > 0) && (m > 1);
	bool end = (a < m) && (x < m) && (c < m);
	
	return begin && end;
}
#endif