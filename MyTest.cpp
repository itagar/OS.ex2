#include <iostream>
#include "uthreads.h"

int x, y, z, w;

void f(void)
{
    int tid = uthread_get_tid();
    volatile int k = 0;
    std::cout << "Start f(" << uthread_get_tid() << ")" << std::endl;
    std::cout << "In f(" << uthread_get_quantums(tid) << ")" << std::endl;

    while (k < 800000000)
    {
        k++;
        if (k % 200000000 == 0)
        {
            std::cout << "In f(" << uthread_get_quantums(tid) << ")" << std::endl;
        }
    }

//    std::cout << "terminate g()" << std::endl;
//    uthread_terminate(2);

    k = 0;
    while (k < 800000000)
    {
        k++;
        if (k % 200000000 == 0)
        {
            std::cout << "In f(" << uthread_get_quantums(tid) << ")" << std::endl;
        }
    }

    std::cout << "resume h()" << std::endl;
    uthread_resume(3);

    while (true)
    {
        k++;
        if (k % 200000000 == 0)
        {
            std::cout << "In f(" << uthread_get_quantums(tid) << ")" << std::endl;
        }
    }
}


void r(void)
{
    int tid = uthread_get_tid();
    volatile int j = 0;
    std::cout << "Start r(" << uthread_get_tid() << ")" << std::endl;
    std::cout << "In r(" << uthread_get_quantums(tid) << ")" << std::endl;


    while (j < 1000000000)
    {
        j++;
        if (j % 200000000 == 0)
        {
            std::cout << "In r(" << uthread_get_quantums(tid) << ")" << std::endl;
        }
    }

    while (true)
    {
        j++;
        if (j % 200000000 == 0)
        {
            std::cout << "In r(" << uthread_get_quantums(tid) << ")" << std::endl;
        }
    }

}

void g(void)
{
    int tid = uthread_get_tid();
    volatile int j = 0;
    std::cout << "Start g(" << uthread_get_tid() << ")" << std::endl;
    std::cout << "In g(" << uthread_get_quantums(tid) << ")" << std::endl;

    while (j < 900000000)
    {
        j++;
        if (j % 200000000 == 0)
        {
            std::cout << "In g(" << uthread_get_quantums(tid) << ")" << std::endl;
        }
    }

    std::cout << "terminate g()" << std::endl;
    uthread_terminate(tid);

    while (true)
    {
        j++;
        if (j % 200000000 == 0)
        {
            std::cout << "In g(" << uthread_get_quantums(tid) << ")" << std::endl;
        }
    }

}

void h(void)
{
    int tid = uthread_get_tid();
    volatile int l = 0;
    std::cout << "Start h(" << uthread_get_tid() << ")" << std::endl;
    std::cout << "In h(" << uthread_get_quantums(tid) << ")" << std::endl;

    while (l < 50000000)
    {
        l++;
        if (l % 200000000 == 0)
        {
            std::cout << "In h(" << uthread_get_quantums(tid) << ")" << std::endl;
        }
    }

    std::cout << "Sync h() to g()" << std::endl;
    uthread_sync(2);

    std::cout << "Terminate h()" << std::endl;
    uthread_terminate(tid);

    while (true)
    {
        l++;
        if (l % 200000000 == 0)
        {
            std::cout << "In h(" << uthread_get_quantums(tid) << ")" << std::endl;
        }
    }
}

int main(int argc, char* argv[])
{
    int tid = uthread_get_tid();
    volatile int i = 0;
    uthread_init(1500000);
    std::cout << "Total Quantums at Init: " << uthread_get_total_quantums() << std::endl;
    std::cout << "Start main(" << uthread_get_tid() << ")" << std::endl;
    std::cout << "In main(" << uthread_get_quantums(tid) << ")" << std::endl;
    int x = uthread_spawn(f);
    int y = uthread_spawn(g);
    int z = uthread_spawn(h);

    while (i < 900000000)
    {
        i++;
        if (i % 200000000 == 0)
        {
            std::cout << "In main(" << uthread_get_quantums(tid) << ")" << std::endl;
        }
    }

//    std::cout << "block h()" << std::endl;
//    uthread_block(z);

    i = 0;

    while (i < 2000000000)
    {
        i++;
        if (i % 200000000 == 0)
        {
            std::cout << "In main(" << uthread_get_quantums(tid) << ")" << std::endl;
        }
    }

    std::cout << "terminate main()" << std::endl;
    uthread_terminate(tid);

    while (true)
    {
        i++;
        if (i % 200000000 == 0)
        {
            std::cout << "In main(" << uthread_get_quantums(tid) << ")" << std::endl;
        }
    }

}