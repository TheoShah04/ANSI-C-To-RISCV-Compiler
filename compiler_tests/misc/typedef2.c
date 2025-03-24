typedef int int_s;

typedef int_s *pint_s;

int_s g(int_s y)
{
    pint_s p;
    int_s x;
    x=y;
    p=&x;
    return 1+*p;
}
