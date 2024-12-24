#include "rtl-sdr.h"
#include <stdio.h>
#include <stdlib.h>
#include <vector>
#include "liquid/liquid.h"
#include <complex.h>
#include <string.h>


#define SAMPLE_RATE 2048000  // 2.048 MS/s
#define BLOCK_LEN 4096 // Length of block to be observed

using namespace std;
typedef unsigned int uint;
typedef unsigned char uchar;
/*TODO:
- Add frequency hopping
- Fix PSD calculation: Currently gives a lower number when channel is in use, abs val is larger
*/

void AVG_PSD(liquid_float_complex *IQ_COMPLEX){
    //Calculate average PSD of 1024 frequenct bins
    uint n = BLOCK_LEN;
    liquid_float_complex *IQ_FREQ = new liquid_float_complex[BLOCK_LEN]; //Array for fft outputs
    fftplan q = fft_create_plan(n, IQ_COMPLEX, IQ_FREQ, LIQUID_FFT_FORWARD, 0);
    fft_execute(q);
    float block_psd = 0.0; //init psd of block 
    for(int i = 0; i < n; i++){
        float real = IQ_FREQ[i].real;
        float imag = IQ_FREQ[i].imag;
        float power = (pow(abs(real+imag),2))/(n*SAMPLE_RATE);
        float psd = 10*log10(power); 
        block_psd += psd;
    }
    float avg_psd = block_psd/BLOCK_LEN;
    printf("AVG PSD: %0.2f dB \n", avg_psd);
    fft_destroy_plan(q);
    delete[] IQ_FREQ;
}

void callback(uchar *buf, uint32_t len, void *ctx){
    //Callback funciton to process I/Q samples 
    if(len < BLOCK_LEN){
        printf("Received fewer samples than nessicary\n");
        return;
    }
    liquid_float_complex *IQ_COMPLEX = new liquid_float_complex[BLOCK_LEN];
    for(int i = 0; i < BLOCK_LEN; i++){
        uint8_t I_byte = buf[2*i];
        uint8_t Q_byte = buf[2*i+1];
        IQ_COMPLEX[i].real = (float)I_byte;
        IQ_COMPLEX[i].imag = (float)Q_byte;
    }
    AVG_PSD(IQ_COMPLEX);
    delete[] IQ_COMPLEX;
}

int main(){
    int freq = 433000000; //433 MHz, uesd to test PSD calculation
    rtlsdr_dev_t *dev = NULL;
    int device_index = 0;
    int r; //Return value for functions
    int device_count = rtlsdr_get_device_count();
    if(device_count == 0){
        fprintf(stderr,"No device found.\n");
        return -1;
    }
    printf("Found %d device(s):\n", device_count);
    for (int i = 0; i < device_count; i++) {
        printf("  %d: %s\n", i, rtlsdr_get_device_name(i));
    }
    r = rtlsdr_open(&dev, device_index);
    if(r < 0){
        fprintf(stderr, "Failed to open device\n");
        return -1;
    }
    r = rtlsdr_set_sample_rate(dev, SAMPLE_RATE);
    if(r < 0){
        fprintf(stderr,"Failed to set sample rate.\n");
        return -1;
    }
    r = rtlsdr_set_center_freq(dev, freq);
    if(r < 0){
        fprintf(stderr, "Failed to set freq.\n");
        return -1;
    }
    printf("Tuned to frequncy: %d Hz\n", freq);

    r = rtlsdr_reset_buffer(dev);
    if(r < 0){
        fprintf(stderr, "Failed to reset buffer\n");
        return -1;
    }
    r = rtlsdr_read_async(dev, callback, NULL, 0, BLOCK_LEN);
    if (r < 0) {
        fprintf(stderr, "Error in async read.\n");
        return -1;
    }
    rtlsdr_close(dev);
    return 0;
}
