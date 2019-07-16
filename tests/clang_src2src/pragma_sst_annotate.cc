#pragma sst placeholder tool(memtrace) outputs(time)
double fxn(double const* __restrict__ a, int N) {
  double b = 0.0;

  for(auto i = 0; i < N; ++i){
    b += a[i];
  }

  return b;
}


char sstmac_appname_str[] = "pragma_test";
int main() { 
  int N = 10;
  double *a = new double[N]; 
  
  const double my_pi = 3.14159265359;
  const double circ_in_yards = 43825760;
  for(auto i = 0; i < N; ++i){
    a[i] = (i * circ_in_yards) / my_pi;
  }

  a[0] = fxn(a,N);

  return 0; 
}
