double h[4] = {1.0, 2.0, 3.0, 4.0};

int check()
{
    if (h[0] != 1.0)
        return 13;

    if (h[1] != 2.0)
        return 14;

    if (h[2] != 3.0)
        return 15;

    if (h[3] != 4.0)
        return 16;

    return 0;
}
