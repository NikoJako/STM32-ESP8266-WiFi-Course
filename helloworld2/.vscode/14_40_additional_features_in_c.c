/* personal finance calculations, using function calls */ 
#include <stdio.h> 
#include <stdlib.h> 
#include <ctype.h> 
#include <math.h> 

/* function prototypes */ 
double mdl (double i,int m, double n) ; 
double md2(double i,int m, double n); 
double md3(double i,double n);

main(int argc, char *argv[]) /* calculate the future value of a series of monthly deposits */ 
{ 
    enum {A = 1, S = 2, Q = 4, M = 12, D = 360, C} m; 

    /* number of compounding periods per year */ 
    int count; /* loop counter */ 
    double n; /* number of years */ 
    double a; /* amount of each monthly payment */ 
    double i; /* annual interest rate */ 
    double f; /* future value */ 
    char freq; /* frequency of compounding indicator */ 

    /* enter input data */ 
    printf("\nFUTURE VALUE OF A SERIES OF MONTHLY DEPOSITS\n\n"); 
    printf("Amount of Each Monthly Payment: "); 
    scanf("%lf", &a); 
    printf("Number of Years: "); 
    scanf("%lf", &n); 

    /*enter frequency of compounding*/

    do { 
        printf("Frequency of Compounding (A, S, Q, M, D, C): "); 
        
        freq = argv[1]; /* convert to uppercase */ 
        if (freq == 'A' || freq == 'a') 
        { 
            m = A; 
            printf("\nAnnual Compounding\n"); 
        } 
        else if (freq == 'S' || freq == 's') 
        { 
            m = S; 
            printf("\nSemiannual Compounding\n"); 
        }
        else if (freq == 'Q' || freq == 'q') 
        { 
            m = Q; 
            printf("\nQuarterly Compounding\n"); 
        } 
        else if (freq == 'M' || freq == 'm') 
        { 
            m = M; 
            printf("\nMonthly Compounding\n"); 
        } 
        else if (freq == 'D' || freq == 'd') 
        { 
            m = D; 
            printf("\nDaily Compounding\n" ); 
        } 
        else if (freq == 'C' || freq == 'c') 
        { 
            m = C; 
            printf("\nContinuous Compounding\n"); 
        } 
        else 
            printf("\nERROR - Please Repaat\n\n"); 

    } 

        while (freq != 'A' && freq != 'S' && freq != 'Q' && freq != 'M' && freq != 'D' && freq != 'C'); 

    /*carry out the calculations*/
    printf("\nInterest Rate Future Amount\n\n");

    for (count = 1; count <= 20; ++count) 
    { 
        i= 0.01 * count; 
        if (m == C) 
        f = a * md3(i, n); /* continuous compounding */ 
        else if (m == D) 
        f = a * md2(i, m, n); /* daily compounding */ 
        else 
        f = a * mdl(i, m, n); /* annual, semiannual, quarterly or monthly compounding */ 
        printf (" %2d %.2f\n", count, f); 
    } 
    return 0;
}


double mdl(double i,int m, double n) 
{/* monthly deposits, periodic compounding */ 
    double factor, ratio; 
    factor = 1 + i/m; 
    ratio = 12 * (pow(factor, m*n) - 1) / i; 
    return(ratio); 
} 

double md2(double i,int m, double n) 
{
    /* monthly deposits, daily compounding */ 
     
    double factor, ratio; 
    factor = 1 + i/m; 
    ratio = (pow(factor, m*n) - 1) / (pow(factor, m/12) - 1); 
    return(ratio); 
} 

double md3(double i, double n) 
{   /* monthly deposits, continuous compounding */ 
    double ratio; 
    ratio = (exp(i*n) - 1) / (exp(i/12) - 1); 
    return(ratio); 
}
