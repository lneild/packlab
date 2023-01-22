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
      config->should_decompress = true; 
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
    }

    // test print statements:
    printf("compress = %d\n", (int) config->should_decompress);
    printf("encrypt = %d\n", (int) config->should_decrypt);
    printf("checksum = %d\n", (int) config->should_checksum);
    printf("data offset = %d\n", (int) config->data_offset);
    
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
  printf("checksum val = %d\n", (int) val);

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

}

size_t decompress_data(uint8_t* input_data, size_t input_len,
                       uint8_t* output_data, size_t output_len,
                       uint8_t* dictionary_data) {

  // TODO
  // Decompress input_data and write result to output_data
  // Return the length of the decompressed data

  return 0;
}

