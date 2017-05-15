#include "shelveeq.h"
#include "filters.h"

// from http://www.musicdsp.org/files/filters003.txt

/*
slider1:0<0,1,1{Stereo,Mono}>Processing
slider2:50<0,100,0.05>Low Shelf (Scale)
slider3:0<-24,24,0.01>Gain (dB)
slider4:50<0,100,0.05>High Shelf (Scale)
slider5:0<-24,24,0.01>Gain (dB)
slider6:0<-24,24,0.05>Output (dB)
*/

///////////////////////////////////////////

void ShelveEq::init()
{
    SPN=0;
    yl=x1l=x2l=y1l=y2l=yr=x1r=x2r=y1r=y2r=0;
}

void LoShelveEq::set(float cf, float peakg)
{
    float freq1 = cf;
    cf *= ISR;

    float sa = tanf(pi*(cf-0.25));
    float asq = sa*sa;
    float A = powf(10,(peakg/20.0));
    
    float F = ((peakg < 6.0) && (peakg > -6.0)) 
            ? sqrt(A)
            : (A > 1.0) 
                ? A/sqrt(2.0)
                : A*sqrt(2.0);
    
    float F2 = F*F;
    float tmp = A*A - F2;

    float gammad = (fabs(tmp) <= SPN)
                 ? 1.0
                 : powf((F2-1.0)/tmp,0.25);

    float gamman = sqrt(A)*gammad;
    float gamma2 = gamman*gamman;
    float gam2p1 = 1.0 + gamma2;
    float siggam2 = 2.0*sqrt(2.0)/2.0*gamman;
    float ta0 = gam2p1 + siggam2;
    float ta1 = -2.0*(1.0 - gamma2);
    float ta2 = gam2p1 - siggam2;
          gamma2 = gammad*gammad;
          gam2p1 = 1.0 + gamma2;
          siggam2 = 2.0*sqrt(2.0)/2.0*gammad;
    float tb0 = gam2p1 + siggam2;
    float tb1 = -2.0*(1.0 - gamma2);
    float tb2 = gam2p1 - siggam2;

    float aa1 = sa*ta1;
    float a0 = ta0 + aa1 + asq*ta2;
    float a1 = 2.0*sa*(ta0+ta2)+(1.0+asq)*ta1;
    float a2 = asq*ta0 + aa1 + ta2;

    float ab1 = sa*tb1;
    float b0 = tb0 + ab1 + asq*tb2;
    float b1 = 2.0*sa*(tb0+tb2)+(1.0+asq)*tb1;
    float b2 = asq*tb0 + ab1 + tb2;

    float recipb0 = 1.0/b0;
    a0 *= recipb0;
    a1 *= recipb0;
    a2 *= recipb0;
    b1 *= recipb0;
    b2 *= recipb0;

    a0 = a0;  
    a1 = a1;
    a2 = a2;
    b1 = -b1;
    b2 = -b2;

}

void HiShelveEq::set(float cf, float peakg )
{
    float freq2 = cf;
    cf *= ISR;
    float boost = -peakg;

    float sa = tan(pi*(cf-0.25));
    float asq = sa*sa;
    float A = powf(10,(boost/20.0));
    float F = ((boost < 6.0) && (boost > -6.0))
            ? sqrt(A)
            : (A > 1.0) 
              ? A/sqrt(2.0) 
              : A*sqrt(2.0);

    float F2 = F*F;
    float tmp = A*A - F2;
    float gammad = (fabs(tmp) <= SPN)
                 ? 1.0
                 : powf((F2-1.0)/tmp,0.25);
    float gamman = sqrt(A)*gammad;
    float gamma2 = gamman*gamman;
    float gam2p1 = 1.0 + gamma2;
    float siggam2 = 2.0*sqrt(2.0)/2.0*gamman;
    float ta0 = gam2p1 + siggam2;
    float ta1 = -2.0*(1.0 - gamma2);
    float ta2 = gam2p1 - siggam2;
          gamma2 = gammad*gammad;
          gam2p1 = 1.0 + gamma2;
          siggam2 = 2.0*sqrt(2.0)/2.0*gammad;
    float tb0 = gam2p1 + siggam2;
    float tb1 = -2.0*(1.0 - gamma2);
    float tb2 = gam2p1 - siggam2;

    float aa1 = sa*ta1;
    float a0 = ta0 + aa1 + asq*ta2;
    float a1 = 2.0*sa*(ta0+ta2)+(1.0+asq)*ta1;
    float a2 = asq*ta0 + aa1 + ta2;

    float ab1 = sa*tb1;
    float b0 = tb0 + ab1 + asq*tb2;
    float b1 = 2.0*sa*(tb0+tb2)+(1.0+asq)*tb1;
    float b2 = asq*tb0 + ab1 + tb2;

    float recipb0 = 1.0/b0;
    a0 *= recipb0;
    a1 *= recipb0;
    a2 *= recipb0;
    b1 *= recipb0;
    b2 *= recipb0;
      
    float gain = powf(10.0f,(boost/20.0));
    a0 = a0/gain;
    a1 = a1/gain; 
    a2 = a2/gain; 
    b1 = -b1;
    b2 = -b2;

}

///////////////////////////////////////////

float LoShelveEq::compute(float inp)
{
    float xl = inp;
     
    yl = a0*xl 
          + a1*x1l 
          + a2*x2l 
          + b1*y1l 
          + b2*y2l;
    x2l = x1l;
    x1l = xl;
    y2l = y1l;
    y1l = yl;

    return yl;
}

///////////////////////////////////////////

float HiShelveEq::compute(float inp)
{
    float xl = inp;
     
    yl = a0*xl 
          + a1*x1l 
          + a2*x2l 
          + b1*y1l 
          + b2*y2l;
    x2l = x1l;
    x1l = xl;
    y2l = y1l;
    y1l = yl;

    return yl;
}


