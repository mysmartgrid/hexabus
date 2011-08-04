/*
 * readhex.h
 *
 *  Created on: 18.03.2011
 *      Author: Dennis Wilfert
 */

#ifndef READHEX_H_
#define READHEX_H_

/* this loads an intel hex file into the memory[] array */
int load_file(char *filename, int *min, int *max);

/* this is used by load_file to get each line of intex hex */
int parse_hex_line(char *theline, int bytes[], int *addr, int *num, int *code);

extern int	memory[65536*2];		/* the memory is global */

#endif /* READHEX_H_ */
