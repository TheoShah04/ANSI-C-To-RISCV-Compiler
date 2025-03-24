int binomial_coefficient(int n, int k) {
    if (k == 0 || k == n) {
        return 1;
    }

    return binomial_coefficient(n-1, k-1) + binomial_coefficient(n-1, k);
}
