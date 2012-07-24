#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#include "subor.h"

/* Print nfdump file header */
void PrintFileHeader (file_header_t fHeader) {

	printf("=========== File header =========== \n");
    printf("Magic:\t\t%X \n", fHeader.magic);
    printf("Version:\t%X \n", fHeader.version);
    printf("Flags:\t\t%X \n", fHeader.flags);
    printf("NumBlocks:\t%d \n", fHeader.NumBlocks);
    printf("StringIdent:\t%s \n", fHeader.ident);
    printf("====================================== \n");
}

/* Read nfdump file header */
void ReadFileHeader (FILE *input) {
	
	file_header_t fHeader;

    fread(&fHeader, sizeof(file_header_t), 1, input);

	PrintFileHeader(fHeader);
}

/* Parse arguments */
int parseArgs(int argc, char *argv[]) {

	// check arguments
	if ((argc != 2) || (strcmp(argv[0], "-r") != 0)) {
		fprintf(stderr, "Argument error\n");
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}

/* MAIN FUNCTION */
int head(int argc, char *argv[]) {
	
	if (parseArgs(argc, argv) == EXIT_FAILURE)
		return EXIT_FAILURE;

	// open file
	FILE *input = fopen(argv[1], "r");

	// error: file cannot be open
	if (input == NULL) {
		fprintf(stderr, "File error\n");
		return EXIT_FAILURE;
	}

	ReadFileHeader(input);

	return EXIT_SUCCESS;
}