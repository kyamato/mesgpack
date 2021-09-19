#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sysexits.h>
#include <stdlib.h>

#define BUFSIZE		(1048576 * 16)
#define LEN_HEADDER	24
#define	LEN_UNKNOWN	11
#define	LEN_EXT8	8
#define	FIXARRAY	0x90
#define	FIXMAP		0x80
#define	FIXSTR		0xa0
#define	UINT8		0xcc
#define	UINT16		0xcd
#define	FIXEXT8		0xd7
#define	STR8		0xd9
#define	STR16		0xda
#define	MAP16		0xde

#define	HEADER		0xc1
#define	PADDING		16

unsigned char buffer[BUFSIZE];
long high,high0=0,low,low0=0;

unsigned char *putstr(unsigned char *pt, int stlen){
	int cnt;

	printf("\"");
	for(cnt=0;cnt < stlen;cnt++) putchar(*pt++);
	printf("\"");
	return(pt);
}

/*   dump n bytes for Debugging */
void dumpnb(unsigned char *p, int n) {
	int i;
	unsigned char *po = p;

	printf("\nDUMP:");
	for (i=0;i<n;i++)
		printf("%2x ",*po++);
	printf(" END DUMP\n");

}

/*  FIXMAP */
unsigned char *fixmap(unsigned char *pt){
	int cnt,stlen,code,arrycnt;
	unsigned char *ptl = pt;

	code = *ptl & 0xff;
	//printf("+ %x %2x",ptl,code);
	//dumpnb(buffer+LEN_HEADDER,N);

	if((code & 0xe0) == FIXSTR) {  // Key string
		printf("\n\t");  // Not necessary, Just for pretty printing
		stlen = *ptl++&0x1f;
		//printf("FIXSTR %d ",stlen);
		ptl = putstr(ptl,stlen);
		printf(": ");
	}

	// Value portion
	//printf("= %2x",*ptl);
	code = *ptl & 0xff;
	if((code & 0xe0) == FIXSTR) {
		stlen = *ptl++&0x1f;
		//printf("Value %d ",stlen);
		ptl = putstr(ptl,stlen);
		//printf("===");
		//printf(",");
		//printf("- %2x",*ptl);
		return ptl;
	}
	
	if (code == STR8) {
		stlen = *++ptl;
		//printf("STR8 %d ",stlen);
		ptl++;
		ptl = putstr(ptl,stlen);
		//printf("- %2x",*ptl);
		return ptl;
	} else if (code == STR16) {
		stlen = *++ptl << 8;
		++ptl;
		stlen |= *ptl++; 
		//printf("STR16 %d ",stlen);
		ptl = putstr(ptl,stlen);
		return ptl;
	} else if ((code & 0xf0) == FIXARRAY) {
		//printf("+ %x %2x",ptl,code);
		printf("[");
		for(cnt=0,arrycnt=*ptl++&0x0f;cnt < arrycnt;cnt++) {
			//printf("cnt = %d, arrycnt = %d\n",cnt,arrycnt);
			if ((*ptl & 0xe0) == FIXSTR) {
				stlen = *ptl++ & 0x1f;
		       		ptl=putstr(ptl,stlen);
				if (cnt < arrycnt-1) printf(",");
			} else if (*ptl == STR8) {
				ptl++;
				stlen = *ptl++;
		       		ptl=putstr(ptl,stlen);
				if (cnt < arrycnt-1) printf(",");
			} else {
				if ((ptl-buffer) >= BUFSIZE) {
					printf("Empty buffer\n");
					return(0);
					//exit(EXIT_SUCCESS);
				}
				printf("Unknown Structure-3 %x %x %x %x\n",*ptl, *(ptl+1),*(ptl+2),*(ptl+3));
				return(0);
				//exit(EXIT_FAILURE);
			}
		}
		printf("]");
	} else if ((code &0xf0) == FIXMAP) {
		printf("{");
		for(cnt=0,stlen=*ptl++&0x0f;cnt<stlen;cnt++) {
		       	ptl=fixmap(ptl);
			if (cnt < stlen-1) printf(",");
		}
		printf("\n\t}");
	} else if (code == UINT8) {
		printf("%d",*++ptl);
		ptl++;
	} else if (code == UINT16) {
		ptl++;
		printf("%d",(*ptl++<<8)|*ptl++);
	} else {
		if ((ptl-buffer) >= BUFSIZE) {
			printf("Empty buffer\n"); exit(EXIT_SUCCESS);
		}
		printf("OTHERs %2x %2x\n",code,pt[1]);
		return ((unsigned char *)-1);
	}
	//printf("- %2x",*ptl);
	return ptl;
}

unsigned long getcrc32(unsigned char *pt)
{
	return 0;
}

void main(int argc,char *argv[])
{
	int i,j,k;
	int fd;
	int len;
	unsigned char *pt;
	int stlen;
	int numstr;
	int cntstr;
	int datasize;
	int records = 0;
	int filep = 1;
	int sfmt = 0;
	int zcount=0;
	unsigned char *first = buffer;
	unsigned long	crc32;
	unsigned char *zeropt;

	if ( argc != 2) {
		if ( (argc != 3) || ((argv[1][0] != '-') || (argv[1][1] != 's')) ) {
			fprintf(stderr,"%s [-s] data-file\n", argv[0]);
			exit(EXIT_FAILURE);
		}
		filep = 2;
		sfmt = 1;
	}

	printf("%s: ",argv[filep]);
	fd = open(argv[filep],O_RDONLY);

	if(fd <= 0) {
		fprintf(stderr,"Can't open");
		exit(EXIT_FAILURE);
	}

	if((datasize = read(fd,buffer,BUFSIZE)) < 0) {
		fprintf(stderr,"Can't read");
		close(fd);
		exit(EXIT_FAILURE);
	}
	//printf("READ = %d ",datasize);

	pt = buffer;
	if (*pt != HEADER) {
		printf("Invalid chunk file %.2x\n",*pt);
		close(fd);
		exit(EXIT_FAILURE);
	}
	pt += 2;
	crc32 = getcrc32(pt); pt += 4;
	pt += PADDING;
	len = (*pt++ << 8) | (*pt++);
	printf("[Length %d] ",len);
	first += PADDING + 2 + 4;	// 0xc1:0x00:crc32:PADDING

	for (j=0;j < len;j++) putchar(*pt++); putchar(0x20);

	first += len + 2;	// length:filename
	zeropt = first;

	if ((*pt&0xf0) != FIXARRAY) {
		for(;pt < buffer + datasize;pt++) {
			if(*pt) {
				printf("Broken at %d: %d  %x",pt-buffer, datasize, *pt&0xf0);
				close(fd);
				exit(EXIT_FAILURE);
			}
		}
		printf("NULL File %d",datasize);
		printf("\n");
		close(fd);
		exit(EXIT_FAILURE);
	}

	printf("\n");
	if (sfmt) {
		close(fd);
		exit (EXIT_SUCCESS);
	}

	//while((pt-buffer)<BUFSIZE) {
	while(1) {
		//printf("%2x ",*pt);
		if (zcount==0) printf("\nRecord Length = %d\n",pt-first);
		zcount = 0;
		first = pt;

		if ((pt-buffer) >= datasize) {
			printf("\n\n# of records = %d\n",records); exit(EXIT_SUCCESS);
		}

		if( *pt == 0x92 ) {  // FIXARRAY 2 tupl
			if (*(pt+1) == FIXEXT8) {
				records++;
#if 0
				pt+=3;
				high = (((((pt[0] << 8) | pt[1]) << 8) | pt[2]) << 8) | pt[3];
				pt += 4;	
				high = high<<32 | (((((pt[0] << 8) | pt[1]) << 8) | pt[2]) << 8) | pt[3];
				pt -= 7;
				printf("high=%d(%x) ", high-high0);
				high0 = high;
#endif
				printf("[%d] FIXEXT8:",records);
				for ( k=0,pt+=3 ;k < LEN_EXT8;k++) {
					printf("%.2x",*pt);
					pt++;
				}
				printf("\n{");
			} else {
				printf("Other type = %2x ",*(pt+1));
				dumpnb(pt,LEN_UNKNOWN);
				close(fd);
				exit(EXIT_FAILURE);
			}
		}
		if((*pt & 0xf0) == FIXMAP) {
			numstr = *pt & 0x0f;
			//printf("# of Str = %d ",numstr);
			pt++;
			//printf("%2x",*pt);
			for(cntstr=0;cntstr<numstr;cntstr++) {
				pt = fixmap(pt);
				if(pt == 0) {
					close(fd);
					exit(EXIT_SUCCESS);
				} if(pt < 0) {
					close(fd);
					exit(EXIT_FAILURE);
				}
				if( cntstr<numstr-1 ) printf(",");
			}
			printf("}");
		} else if (*pt == MAP16) {
			numstr = *++pt << 8;
			pt++;
			numstr |= *pt++;
			for (cntstr=0;cntstr<numstr;cntstr++) {
				printf("%.2x",*pt++);
			}
			
		} else {
			if ((pt-buffer) >= datasize) {
				printf("Empty buffer\n"); close(fd); exit(EXIT_SUCCESS);
			}
			//printf("Unknown Structure-2 %d %2x\n",pt-buffer,*pt);
			for(zcount=0;pt<buffer+datasize;pt++,zcount++) {
				if(*pt) {
					printf("%d zero byte block offset=%o(%d)\n",zcount,pt-zeropt-zcount,pt-zeropt-zcount);
					break;
				} 
			}
			continue;
			//dumpnb(pt, 32);
			//close(fd);
			//exit(EXIT_FAILURE);
		}
		//if ( pt == (unsigned char *)-1) break;
		//printf(" %2x ",*pt);
		//printf("\n");
	}
	//printf("\n");
	close(fd);
}
