
int isPrime(int n) {
  int i;
  for (i=2; i<n; i++) {
    if (n % i == 0) return 0;
  }
  return 1;
}

int nextPrime(int n) {
  int p = n+1;
  while (1) {
    if (isPrime(p)) {
      return p;
    }
    p++;
  }
}
