#include "Model/Stages.h"
#include <cstdio>
#include <vector>
#include <complex>
using namespace bluechannel::model;
struct Result {double gain,thd;};
Result analyse(const std::vector<double>& v,double f,double fs)
{
    const int n=int(fs),start=int(v.size())-n;double h[10]={};
    for(int k=1;k<10;++k) {std::complex<double> sum{};for(int i=0;i<n;++i) sum+=v[size_t(start+i)]*std::polar(1.0,-2*3.141592653589793*k*f*i/fs);h[k]=2*std::abs(sum)/n;}
    double distortion=0;for(int k=2;k<10;++k)distortion+=h[k]*h[k];return {h[1],100*std::sqrt(distortion)/h[1]};
}
int main()
{
    constexpr double fs=96000;std::puts("stage,freq_hz,connector_equivalent_dbu,gain_db,core_enabled_vs_zero_current_rms_v,bit_identical");
    for(double f : {20.,50.,1000.}) for(double level : {4.,16.,22.})
    {
        OutputStage stage;stage.prepare(fs);
        bluechannel::circuit::DkStage linear;linear.load(bluechannel::circuit::parseNetlist(nl::kOutput));linear.setSampleRate(fs);linear.update();
        const double amp=dbuToPeakVolts(level)/2;
        std::vector<double> a(size_t(2*fs));double error=0;bool identical=true;
        for(size_t i=0;i<a.size();++i)
        {
            const double input=amp*std::sin(2*3.141592653589793*f*double(i)/fs);
            a[i]=stage.process(input);const double u[2]={railClip(input),0};double y[2];linear.process(u,y);const double baseline=y[0]-y[1];
            identical=identical && a[i]==baseline;
            if(i>=size_t(fs)) error+=(a[i]-baseline)*(a[i]-baseline);
        }
        const auto r=analyse(a,f,fs);
        std::printf("output,%.0f,%.0f,%.6f,%.10g,%s\n",f,level,20*std::log10(r.gain/amp),std::sqrt(error/fs),identical ? "yes" : "no");
    }
}
