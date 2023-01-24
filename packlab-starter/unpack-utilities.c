// Utilities for unpacking files
// PackLab - CS213 - Northwestern University

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "unpack-utilities.h"


// --- public functions ---

void error_and_exit(const char* message) {
  fprintf(stderr, "%s", message);
  exit(1);
}

void* malloc_and_check(size_t size) {
  void* pointer = malloc(size);
  if (pointer == NULL) {
    error_and_exit("ERROR: malloc failed\n");
  }
  return pointer;
}

void parse_header(uint8_t* input_data, size_t input_len, packlab_config_t* config) {

  // TODO
  // Validate the header and set configurations based on it
  // Call error_and_exit() if the header is invalid or input_len is shorter than expected

  // checking input length:
  if (input_len < 4){
    error_and_exit("input header size error - not large enough"); 
  }

  if (input_data[0] != 0x02 || input_data[1] != 0x13){
    error_and_exit("magic bytes are incorrect"); 
  }
  else if (input_data[2] != 0x01) {
    error_and_exit("version byte is incorrect"); 
  }
  else {
    // make a copy since editing the value
    // preserve original data
    uint8_t flag_copy = input_data[3]; 
    config->data_offset = 4; // at least 4

    // init all configs to false
    config->should_decompress = false;
    config->should_decrypt = false;
    config->should_checksum = false; 

    // compress bit
    if (flag_copy >> 7 == 1){
      config->should_decompress = true; 
      flag_copy = flag_copy - 128; 
      config->data_offset += 16;
      for (size_t i = 0; i < 16; i++) {
        // pulling out dictionary and writing into conig
        config->dictionary_data[i] = input_data[i+4];
      }
    }
    
    // encrypt bit
    if (flag_copy >> 6 == 1) {
      config->should_decrypt = true; 
      flag_copy = flag_copy - 64; 
    }
    // checksum bit
    if (flag_copy >> 5 == 1) {
      config->should_checksum = true; 
      config->data_offset += 2;

      size_t i = 4; 
      if (config->should_decompress){
        i += 16; 
      }
      uint8_t d1 = input_data[i]; 
      uint8_t d2 = input_data[i+1]; 
      uint16_t val = (d1 << 8) | d2;
      config->checksum_value = val; 
    }

    // test print statements:
    // printf("compress = %d\n", (int) config->should_decompress);
    // printf("encrypt = %d\n", (int) config->should_decrypt);
    // printf("checksum = %d\n", (int) config->should_checksum);
    // printf("data offset = %d\n", (int) config->data_offset);
    
  }

}

uint16_t calculate_checksum(uint8_t* input_data, size_t input_len) {

  // TODO
  // Calculate a checksum over input_data
  // Return the checksum value

  uint16_t val = 0; 

  for(size_t i = 0; i < input_len; i++){
    // update val:
    val += input_data[i]; 
  }

  // test print statements:
  // printf("checksum val = %d\n", (int) val);

  return val; 

}

uint16_t lfsr_step(uint16_t oldstate) {

  // TODO
  // Calculate the new LFSR state given previous state
  // Return the new LFSR state
   
  // push those vals to the front index
  // ront index will be the x or of the assesing indicies
  uint16_t var0 = oldstate << 15;
  uint16_t var11 = oldstate << 4; 
  uint16_t front = var0 ^ var11; 
  uint16_t var13 = oldstate << 2; 
  front = front ^ var13;
  uint16_t var14 = oldstate << 1; 
  front = front ^ var14;
  // now only keep track of the first dig
  // make the rest all 0s
  front = front >> 15; 
  front = front << 15; 

  uint16_t newstate = oldstate >> 1;
  newstate = (newstate | front); 
  
  return newstate; 
}

void decrypt_data(uint8_t* input_data, size_t input_len,
                  uint8_t* output_data, size_t output_len,
                  uint16_t encryption_key) {

  // TODO
  // Decrypt input_data and write result to output_data
  // Uses lfsr_step() to calculate psuedorandom numbers, initialized with encryption_key
  // Step the LFSR once before encrypting data
  // Apply psuedorandom number with an XOR in big-endian order
  // Beware: input_data may be an odd number of bytes

  bool cont = true; 
  uint16_t s = encryption_key; 
  size_t i = 0; 

  while(cont) {
    // take a step
    s = lfsr_step(s);

    // seperate 16 into 2 8s:
    uint8_t b1 = (s >> 8); 
    uint8_t b2 = s & 0x00FF; 

    if (i < input_len) {
      output_data[i] = (b2 ^ input_data[i]); 
      output_len += 1; 
      i += 1; 

      if (i < input_len) {
        output_data[i] = (b1 ^ input_data[i]); 
        output_len += 1; 
        i += 1; 
      }

    }

    // check if we need to keep going:
    if (i >= input_len) {
      cont = false; 
    }

  }

}


size_t decompress_data(uint8_t* input_data, size_t input_len,
                       uint8_t* output_data, size_t output_len,
                       uint8_t* dictionary_data) {

  // TODO
  // Decompress input_data and write result to output_data
  // Return the length of the decompressed data

  // iterate through bytes of input
  // case 1 - normal byte = copy over
  // case 2 - escape byte = look 1 byte further 
  // careful to not go past end - check len as go
  
  // general:
  // decompress input data and write result into output
  // dictionary_data = compression diction used when compressing
  // input_data = array of bytes (only contains data to decompress)
  // does not contain header bytes
  // return = total len of decompressed data

  bool cont = true; 
  size_t input_data_i = 0; 
  size_t output_data_i = 0; 

  while (cont){
    if (input_data_i < input_len){
      uint8_t b1 = input_data[input_data_i]; 

      // if it is an escape key:
      // make sure there is a bit afterwards to look at
      if (b1 == 0x07){
        if (input_data_i+1 < input_len){
          uint8_t b2 = input_data[input_data_i+1]; 
          
          // is the next bit 0x00?
          if (b2 == 0x00){
            // just copy over and move on
            output_data[output_data_i] = b1; 
            input_data_i += 2; 
            output_data_i += 1;
          }

          // interpret the next bit and fill in dictionary data
          else {
            // first 4 bit = repeate count
            // second 4 bit = index in dictionary being repeated
            uint8_t rep = b2 >> 4; // select top 4 bits
            uint8_t dict_i = b2 & 0x0F; // select bottom 4 bits

            for (size_t j = 0; j < rep; j++){
              output_data[output_data_i+j] = dictionary_data[dict_i];
            }
            output_data_i += rep;
            input_data_i += 2; // go ahead 2 
          }
        }
        // when the escape key is the last thing - just copy over
        else {
          output_data[output_data_i] = b1; 
          input_data_i += 1; 
          output_data_i += 1;
        }
      }

      // when it is not an escape key, copy over and move on
      else{
        output_data[output_data_i] = b1;
        output_data_i += 1; 
        input_data_i += 1; 
      }
    }

    if (input_data_i >= input_len){
      cont = false; 
    }

  }

  output_len = output_data_i; 
  return output_len;
}

