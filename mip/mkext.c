
/*
 * Generate defines for the needed hardops.
 */
#include "pass2.h"

int chkop[MAXOP];

void mktables(void);

char *cname = "external.c";
char *hname = "external.h";
FILE *fc, *fh;

/*
 * masks for matching dope with shapes
 */
int mamask[] = {
        SIMPFLG,                /* OPSIMP */
        SIMPFLG|ASGFLG,         /* ASG OPSIMP */
        COMMFLG,        /* OPCOMM */
        COMMFLG|ASGFLG, /* ASG OPCOMM */
        MULFLG,         /* OPMUL */
        MULFLG|ASGFLG,  /* ASG OPMUL */
        DIVFLG,         /* OPDIV */
        DIVFLG|ASGFLG,  /* ASG OPDIV */
        UTYPE,          /* OPUNARY */
        TYFLG,          /* ASG OPUNARY is senseless */
        LTYPE,          /* OPLEAF */
        TYFLG,          /* ASG OPLEAF is senseless */
        0,              /* OPANY */
        ASGOPFLG|ASGFLG,        /* ASG OPANY */
        LOGFLG,         /* OPLOG */
        TYFLG,          /* ASG OPLOG is senseless */
        FLOFLG,         /* OPFLOAT */
        FLOFLG|ASGFLG,  /* ASG OPFLOAT */
        SHFFLG,         /* OPSHFT */
        SHFFLG|ASGFLG,  /* ASG OPSHIFT */
        SPFLG,          /* OPLTYPE */
        TYFLG,          /* ASG OPLTYPE is senseless */
        };


struct checks {
	int op, type;
	char *name;
} checks[] = {
	{ MUL, TLONGLONG, "SMULLL", },
	{ DIV, TLONGLONG, "SDIVLL", },
	{ MOD, TLONGLONG, "SMODLL", },
	{ PLUS, TLONGLONG, "SPLUSLL", },
	{ MINUS, TLONGLONG, "SMINUSLL", },
	{ MUL, TULONGLONG, "UMULLL", },
	{ DIV, TULONGLONG, "UDIVLL", },
	{ MOD, TULONGLONG, "UMODLL", },
	{ PLUS, TULONGLONG, "UPLUSLL", },
	{ MINUS, TULONGLONG, "UMINUSLL", },
	{ 0, 0, 0, },
};

int
main(int argc, char *argv[])
{
	struct optab *op;
	struct checks *ch;
	int i, fregs;
	char *bitary;
	int bitsz;

	mkdope();

	for (op = table; op->op != FREE; op++) {
		if (op->op >= OPSIMP)
			continue;
		if ((op->ltype & TLONGLONG) &&
		    (op->rtype & TLONGLONG))
			chkop[op->op] |= TLONGLONG;
		if ((op->ltype & TULONGLONG) &&
		    (op->rtype & TULONGLONG))
			chkop[op->op] |= TULONGLONG;
	}
	if ((fc = fopen(cname, "w")) == NULL) {
		perror("open cfile");
		return(1);
	}
	if ((fh = fopen(hname, "w")) == NULL) {
		perror("open hfile");
		return(1);
	}
	for (ch = checks; ch->op != 0; ch++) {
		if ((chkop[ch->op] & ch->type) == 0)
			fprintf(fh, "#define NEED_%s\n", ch->name);
	}
	mktables();

	/* calculate number of temporary regs */
	for (fregs = 0, i = (TAREGS|TBREGS); i; i >>= 1)
		if (i & 1)
			fregs++;

	fprintf(fh, "#define FREGS %d\n", fregs);

	/* create efficient bitset sizes */
	if (sizeof(long) == 8) { /* 64-bit arch */
		bitary = "long";
		bitsz = 64;
	} else {
		bitary = "int";
		bitsz = sizeof(int) == 4 ? 32 : 16;
	}
	fprintf(fh, "#define NUMBITS %d\n", bitsz);
	fprintf(fh, "#define BITSET(arr, bit) "
	     "(arr[bit/NUMBITS] |= (1 << (bit & (NUMBITS-1))))\n");
	fprintf(fh, "#define BITCLEAR(arr, bit) "
	     "(arr[bit/NUMBITS] &= ~(1 << (bit & (NUMBITS-1))))\n");
	fprintf(fh, "#define TESTBIT(arr, bit) "
	     "(arr[bit/NUMBITS] & (1 << (bit & (NUMBITS-1))))\n");
	fprintf(fh, "typedef %s bittype;\n", bitary);

	fclose(fc);
	fclose(fh);
	return 0;
}

#define	P(x)	fprintf x

void
mktables()
{
	struct optab *op;
	int mxalen = 0, curalen;
	int i;

//	P((fc, "#include \"pass2.h\"\n\n"));
	for (i = 0; i <= MAXOP; i++) {
		curalen = 0;
		P((fc, "static int op%d[] = { ", i));
		if (dope[i] != 0)
		for (op = table; op->op != FREE; op++) {
			if (op->op < OPSIMP) {
				if (op->op == i) {
					P((fc, "%d, ", op - table));
					curalen++;
				}
			} else {
				int opmtemp;
				if ((opmtemp=mamask[op->op - OPSIMP])&SPFLG) {
					if (i==NAME || i==ICON || i==TEMP ||
					    i==OREG || i == REG) {
						P((fc, "%d, ", op - table));
						curalen++;
					}
				} else if ((dope[i]&(opmtemp|ASGFLG))==opmtemp){
					P((fc, "%d, ", op - table));
					curalen++;
				}
			}
		}
		if (curalen > mxalen)
			mxalen = curalen;
		P((fc, "-1 };\n"));
	}
	P((fc, "\n"));

	P((fc, "int *qtable[] = { \n"));
	for (i = 0; i <= MAXOP; i++) {
		P((fc, "	op%d,\n", i));
	}
	P((fc, "};\n"));
	P((fh, "#define MAXOPLEN %d\n", mxalen+1));
}
