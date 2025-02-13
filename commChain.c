/*Goal is to simulate a wireless communication chain from source 
to sink. 
STEPS:
1. Generate bit stream
2. Channel coding MQam 
3. Modulation
4. Channel (Tx+Noise)
5. Syncrohnization, Demod, EQ, Detection
6. Channel Decoding 
FOR THE FUTURE: Add all of this into a while loop
then plot the symbols -> end result is a live view of 
the symbols changing with the channel conditions
*/
#include <stdio.h>
#include <liquid/liquid.h>
#include <stdlib.h>
#include <time.h>

typedef unsigned int uint;
typedef unsigned char uchar;

void rand_bin_array(uchar* array, uint len){
    //Creates an array of random bits of length len 
    srand(time(NULL)); //Seed random number gen.
    for(int i = 0; i < len; i++){
        array[i] = rand() % 2;
    } 
}

void print_bits(uchar* array, uint len) {
    for (unsigned int i = 0; i < len; i++) {
        for (int j = 7; j >= 0; j--) {  // Iterate through bits in the byte (8 bits in a byte)
            printf("%d", (array[i] >> j) & 1);  // Right shift and mask to get the bit at position j
        }
    }
    printf("\n");
}

void uint_to_4bits(uint num, uchar bits[4]){
    if(num > 15){
        printf("Input exceeds largest number!");
        return;
    }
    for (int i = 3; i >= 0; i--) {
        bits[i] = (num & (1 << i)) ? 1 : 0; // Set each bit (0 or 1)
    }
}


void add_bits_to_array(uchar *bits, uchar *dest, int dest_index){
    for (int i = 0; i < 4; i++) {
        dest[dest_index + i] = bits[i]; // Copy each bit to destination
    }
}

int main(){
    uint mesg_len = 16; //Number of bits
    uchar tx_mesg[mesg_len]; //Original tx messege
    modulation_scheme MS = LIQUID_MODEM_QAM16;
    //Messege gen
    rand_bin_array(tx_mesg, mesg_len); //Array of random 0's and 1's 
    printf("Original messege: ");
    for(uint i = 0; i < mesg_len; i++){
        printf("%d", tx_mesg[i]);
    }
    printf("\n");

    //FEC encoding
    fec fec_obj = fec_create(LIQUID_FEC_HAMMING74, NULL); //Create Hamming(7,4) object

    //Calculate the length of the encoded messege 
    //uint k = fec_get_enc_msg_length(LIQUID_FEC_HAMMING74, mesg_len);
    uint k = 64; 
    printf("FEC Length (bits): %d\n", k);
    uchar enc_mesg[k]; //Encoded messege 
    fec_encode(fec_obj, mesg_len, tx_mesg, enc_mesg); //Fec 
    printf("Encoded: ");
    for(int i = 0; i < k; i++){
        printf("%d", enc_mesg[i]);
    }
    printf("\n");
    /*printf("Encoded messege: ");
    print_bits(enc_mesg, k);
    int len_fec = sizeof(enc_mesg);
    printf("Length of messege: %d\n", len_fec);*/
    modem modulator = modem_create(MS); //16-Qam modulator
    modem_print(modulator);
    uint bps = modemcf_get_bps(modulator); //Bits per symbol
    
    int num_symbols = k/bps; //Length of encoded mes/bps
    liquid_float_complex syms[num_symbols]; //Complex symbol array (len = num of syms after mod)

    printf("Number of symbols: %d\n", num_symbols);
    printf("BPS: %d\n",bps);

    /*uint sym_in = 15;
    liquid_float_complex y;
    modem_modulate(modulator,sym_in, &y);
    printf("Modulated complex symbol: %f + %fi\n", crealf(y), cimagf(y));*/

    for(uint i = 0; i < num_symbols; i++){
        uint symbol_bits = 0; 
        for(uint j = 0; j < bps; j++){
            //This line calculates what integer value the 4 bits represent
            //This int corresponds to a point in the QAM constellastion
            //printf("%c", enc_mesg[j]);
            uchar bit = (tx_mesg[i * bps + j]&1);
            symbol_bits |= bit << (bps-j-1);
            //printf("Sym Bits: %d\n", symbol_bits);
        }
        //printf("Symbol_in %d: %d\n", i, symbol_bits);
        modem_modulate(modulator, symbol_bits, &syms[i]); 
        //printf("Modulated complex symbol: %f + %fi\n", crealf(syms[i]), cimagf(syms[i]));
    }
    printf("Modulation Complete\n");

    //Create channel model
    channel_cccf channel = channel_cccf_create(); 
    float noise_floor =  -95.0f; //dB 
    float SNR = 60.0f; //dB
    channel_cccf_add_awgn(channel, noise_floor, SNR); //Build channel model
    channel_cccf_print(channel);
    liquid_float_complex channel_out[num_symbols];
    channel_cccf_execute_block(channel, syms, num_symbols, channel_out);

    //printf("Modulated complex symbol: %f + %fi\n", crealf(channel_out[0]), cimagf(channel_out[0]));
    /*Simple channel model, no need for Eq -> straight to demod*/
    modem demod = modem_create(MS);
    modem_print(demod);
    uchar rx_mesg[k];
    int dest_index = 0;
    for(int i = 0; i < num_symbols; i++){
        liquid_float_complex sym = channel_out[i];
        //liquid_float_complex sym = syms[i];
        uint sym_demod;
        modem_demodulate(modulator, sym, &sym_demod);
        //printf("Demod sym: %d\n", sym_demod);
        //printf("Size of symbol: %d\n", sizeof(sym_demod));
        //printf("Received symbol: %f + %fi\n", crealf(channel_out[i]), cimagf(channel_out[i]));
        //printf("Received symbol: %f + %fi\n", crealf(syms[i]), cimagf(syms[i]));
        //uchar bits[4] = {0};
        uchar bits[4];
        uint_to_4bits(sym_demod, bits);
        //printf("Mapped bits: %d%d%d%d\n", bits[0], bits[1], bits[2], bits[3]);
        if(dest_index + 4 <= k){
            //add_bits_to_array(bits, rx_mesg, dest_index);
            for (int i = 0; i < 4; i++) {
                rx_mesg[dest_index + i] = bits[i]; // Copy each bit to destination
            }
            dest_index += 4;
            //printf("Added to array\n");
        }
        else{
            printf("Warning: Destination out of space\n");
            break;
        }
    }

    printf("Destination index: %d\n", dest_index);

    //FEC decoding
    fec decoder = fec_create(LIQUID_FEC_HAMMING74, NULL);
    uchar rx_mesg_decoded[mesg_len];
    fec_decode(fec_obj, mesg_len, rx_mesg, rx_mesg_decoded);
    printf("Undecoded: ");
    for(int i = 0; i < k; i++){
        printf("%d", rx_mesg[i]);
    }
    printf("\n");

    printf("Length of rx mesg: %d\n", sizeof(rx_mesg_decoded));
    printf("RX Messege: ");
    for(uint i = 0; i < mesg_len; i++){
        printf("%d", rx_mesg[i]);
    }
    printf("\n");

    /*Remember to destroy objects*/
    fec_destroy(fec_obj);
    fec_destroy(decoder);
    modem_destroy(modulator);
    modem_destroy(demod);
    channel_cccf_destroy(channel); 

    return 0;
} 