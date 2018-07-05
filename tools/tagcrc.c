#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
extern unsigned long crc32 (unsigned long, const unsigned char *, unsigned int);

void usage (void)
{
	fprintf(stderr, "tagcrc [-o offset] [-s start-range] [-e end-range] files ...\n");
	fprintf(stderr, "places a 32-bit CRC in a file. original position must have 0s\n");
	exit(EXIT_FAILURE);
}

int main (int argc, char **argv)
{
	size_t ofs=0x24, start=0, end=(size_t)-1; /* parameters */
	int i;

	while (--argc > 0 && **++argv == '-') {	
		switch (*++*argv) {
			case 'o': /* offset for CRC32 */
				if ((--argc <= 0)) {
					        usage ();
							goto NXTARG;
				}
				ofs = strtoul(*++argv, 0, 0);
				break;
			case 's': /* start range */
				if ((--argc <= 0)) {
					        usage ();
							goto NXTARG;
				}
				start = strtoul(*++argv, 0, 0);
				break;
			case 'e': /* end range (-1 means EOF) */
				if ((--argc <= 0)) {
					        usage ();
							goto NXTARG;
				}
				end = strtol(*++argv, 0, 0);
				break;
			default:
				usage();
		}
NXTARG: ;
	}
	if (argc < 1)
		usage();


	for(i=0;i<argc;i++) {
		unsigned long c;
		unsigned char buf[1024];
		FILE *f;
		int res=0;
		size_t total;

		f=fopen(argv[i], "r+b");
		if(!f) {
			perror(argv[i]);
			return EXIT_FAILURE;
		}
		c=~0ul;
		total=0;
		if(start) {
			if(fseek(f, start, SEEK_SET)==-1) {
				perror(argv[i]);
				return EXIT_FAILURE;
			}
			total+=start;
			printf("%s:seeking %d\n", argv[i], ofs);
		}
		while((end==~0U || total<end) && (res=fread(buf, 1, sizeof buf, f))!=0) {
			if(total+res>end) {
				/* truncate to fit in range */
				res=end-total;
			}
			c=~crc32(~c, buf, res);
			total+=res;
		}
		if(res==0 && ferror(f)) {
			perror(argv[i]);
			return EXIT_FAILURE;
		}
		c=~c;
		printf("%s:CRC=0x%08lX (%d bytes CRC'd)\n", argv[i], c, total);

		if(ofs==(size_t)-1) {
			ofs=total;
		}

		if(fseek(f, ofs, SEEK_SET)==-1) {
			perror(argv[i]);
			return EXIT_FAILURE;
		}
		buf[0]=c&255;
		buf[1]=(c>>8)&255;
		buf[2]=(c>>16)&255;
		buf[3]=(c>>24)&255;
		if(fwrite(buf, 4, 1, f)==0 && ferror(f)) {
			perror(argv[i]);
			return EXIT_FAILURE;
		}
		printf("%s:wrote CRC32 to offset %zd\n", argv[i], ofs);

		fclose(f);
	}
	return 0;
}
