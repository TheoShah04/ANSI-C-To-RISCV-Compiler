double f(double x, int n)
{
    double acc=5.5;
    int i=0;
    while(i<n){
        i++;
        acc=acc*x;
    }
    return acc;
}
