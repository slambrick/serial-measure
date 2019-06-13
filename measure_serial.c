/*
 * Copyright (c) 2019, Sam Lambrick.
 * All rights reserved.
 * This file is subject to the GNU/GPL-3.0-or-later.
 */
#include <ctype.h>
#include <stdio.h>    /* Standard input/output definitions */
#include <stdlib.h>   /* Standard library */
#include <string.h>   /* String function definitions */
#include <unistd.h>   /* Parseing inputs */
#include <sys/time.h> /* Timing */
#include <termios.h>  /* POSIX terminal control definitions */
#include <fcntl.h>    /* File control definitions */

/* Load the arduino serial library by Tod E. Kurt, http://todbot.com/blog/ */
#include "arduino-serial-lib.h"

void get_ints(int n, char* char_list, int* results);
int16_t recombine_bytes(char lower, char higher);
void time_taken(struct timeval *start, struct timeval *end, int samples);
void print_help(void);

int main(int argc, char **argv) {
    const int buf_max = 32;
    char* serialport = NULL;
    int fd;
    int i;
    char buf[buf_max];
    int n;
    int status;
    struct timeval tv1, tv2;
    char bb[1];
    
    /* Results variables */
    char* char_list;
    int* results;
    double* Vs;
    
    /* Input variables */
    char* b = NULL;
    char* d = NULL;
    int o_flag = 0;
    int r_flag = 0;
    int help_flag = 0;
    int baudrate, delay;
    int opt;
    
    opterr = 0;
    
    /* Parse options */
    while ((opt = getopt(argc, argv, "p:b:d:n:or")) != -1) {
        switch (opt) {
            case 'p': 
                serialport = optarg;
                break;
            case 'b':
                b = optarg;
                break;
            case 'd':
                d = optarg;
                break;
            case 'n':
                n = strtol(optarg, NULL, 10);
                break;
            case 'o':
                o_flag = 1;
                break;
            case 'r':
                r_flag = 1;
                break;
            case 'h':
                help_flag = 1;
                break;
        }
    }
    
    if (help_flag) {
        print_help();
        return(0);
    }
    
    /* TODO: input checking */
    
    /* Convert options to the type we want */
    baudrate = strtol(b, NULL, 10);
    delay = strtol(d, NULL, 10);
    
    /* Allocate memory to results */
    results = (int*)malloc(n*sizeof(int));
    
    /* Open the port */
    fd = serialport_init(serialport, baudrate);
    
    /* Delay while the Arduino reboots  */
    printf("sleep %d millisecs\n",delay);
    usleep(delay * 1000 );
    
    printf("n = %d\n\n", n);
    
    
    if (r_flag) {
        /* Read binary data sent from the Arduino */
        /* Need to allocate an array of chars to read into */
        char_list = (char*)malloc(4*n*sizeof(char));
        
        /* Start timing */
        gettimeofday(&tv1, NULL);
        
        /* Clear the input buffer */
        tcflush(fd, TCIFLUSH);
    
        /* Read one character at a time, retrying if none are found */
        i = 0;
        do {
            status = read(fd, bb, 1);
            if (status == 0) {
                usleep(1);
                continue;
            }
            char_list[i] = bb[0];
            i++;
        } while (i < n*4);
        
        /* Stop timing */
        gettimeofday(&tv2, NULL);
        
        /* Print the integer to the terminal */
        if (o_flag) {
            for (i = 0; i < n*4; i++)
                printf("%c\n", char_list[i]);
        }
        
        /* Parse the data to extract the numbers from the characters  */
        get_ints(n*4, char_list, results);
        free(char_list);
    } else {
        /* Read string information sent from the Arduino */
        
        /* Start timing */
        gettimeofday(&tv1, NULL);
        
        /* Clear the input buffer */
        tcflush(fd, TCIFLUSH);
        
        /* Read printed lines */
        for (i = 0; i < n; i++) {
            serialport_read_until(fd, buf, '\n', buf_max, 1000);
            results[i] = strtol(buf, NULL, 10);
        }
        
        /* Stop timing */
        gettimeofday(&tv2, NULL);
    }
    
    time_taken(&tv1, &tv2, n);
    
    if (o_flag) {
        for (i = 0; i < n; i++)
            printf("%i\n", results[i]);
    }
    
    /* Close the serial port */
    serialport_close(fd);
    
    /* Convert the values read from the arduino to voltage differences */
    Vs = (double*)malloc(n*sizeof(double));
    for (i = 0; i < n; i++)
        Vs[i] = 118*((double)results[i])/(1000*1000);
    
    /* No longer need the results stored as integers */
    free(results);
    
    /* Write the results to a file */
    
    /* Free remaining dynamically allocated memory */
    free(Vs);
}

void get_ints(int n, char* char_list, int* results) {
    int i, cnt;
    
    cnt = 0;
    for (i = 0; i < n - 3; i++) {
        int16_t res;
        char higher, lower;
        
        if (char_list[i] == '<') {
            if (char_list[i+3] == '>') {
                lower = char_list[i+1];
                higher = char_list[i+2];
                res = recombine_bytes(lower, higher);
                results[cnt] = (int)res;
                cnt++;
            }
        }
    }
    
}

int16_t recombine_bytes(char lower, char higher) {
    int16_t res;
    
    res = ((int16_t)higher << 8) | (0x00ff & lower);
    return(res);
}

void time_taken(struct timeval *start, struct timeval *end, int samples) {
    double t;
    
    t = (double) (end->tv_usec - start->tv_usec) / 1000000 +
         (double) (end->tv_sec - start->tv_sec);
    
    printf("Time taken: %f\n", t);
    printf("SPS: %f\n", samples/t);
}

void print_help() {
    /* TODO */
}
