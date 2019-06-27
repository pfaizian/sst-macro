double fxn(double const* __restrict__ a, int N, double &c) {
#pragma sst annotate
  double b = 0.0;

#pragma sst annotate
  for(auto i = 0; i < N; ++i){
    int d = i - 1;
    b += a[i];
    asm volatile("mov r0, r0");
    if(i % 2 == 0){
      c += a[i] + d;
    }
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

  double c = 0.0;
  a[0] = fxn(a,N,c);

  return 0; 
}
