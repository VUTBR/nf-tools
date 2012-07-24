#define IdentLen			128

typedef struct file_header_s {
	uint16_t	magic;				// magic to recognize nfdump file type and endian type
	uint16_t	version;			// version of binary file layout, incl. magic
	uint32_t	flags;
	uint32_t	NumBlocks;			// number of data blocks in file
	char		ident[IdentLen];	// string identifier for this file
} file_header_t;



void PrintFileHeader (file_header_t fHeader);
void ReadFileHeader (FILE *input);
int parseArgs(int argc, char *argv[]);
int head(int argc, char *argv[]);