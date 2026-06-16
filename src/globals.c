/*
    the DASM macro assembler (aka small systems cross assembler)

    Copyright (c) 1988-2002 by Matthew Dillon.
    Copyright (c) 1995 by Olaf "Rhialto" Seibert.
    Copyright (c) 2003-2008 by Andrew Davie.
    Copyright (c) 2008 by Peter H. Froehlich.

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License along
    with this program; if not, write to the Free Software Foundation, Inc.,
    51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
*/

/*
 *  GLOBALS.C
 */

#include "asm.h"

SYMBOL *SHash[SHASHSIZE];   /*	symbol hash table   */


MNEMONIC    *MHash[MHASHSIZE];   /*	mnemonic hash table */
INCFILE *pIncfile;	    /*	include file stack  */
REPLOOP *Reploop;	    /*	repeat loop stack   */
SEGMENT *Seglist;	    /*	segment list	    */
SEGMENT *Csegment;	    /*	current segment     */
IFSTACK *Ifstack;	    /*	IF/ELSE/ENDIF stack */
char	*Av[256];	    /*	up to 256 arguments */
char	Avbuf[512];
unsigned char	MsbOrder = 1;
int	Mnext;
char	Inclevel;
unsigned int	Mlevel;
unsigned long	Localindex;	   /*  to generate local variables */
unsigned long	Lastlocalindex;

unsigned long	Localdollarindex;
unsigned long	Lastlocaldollarindex;

unsigned long Processor = 0;
bool bTrace = false;
bool Xdebug;
bool bStrictMode = false;

unsigned char	Outputformat;

unsigned long   Redo_why = 0;
int	Redo_eval = 0;	   /*  infinite loop detection only    */
int Redo = 0;

int nMacroDeclarations = 0;
int nMacroClosings = 0;

unsigned long maxFileSize = 640 * 1024;		// avoid recursive growing via set,eqm
	// 640k are enough for everybody ... said Bill G. How many 64k pages have you ?

unsigned long	Redo_if = 0;

char	ListMode = 1;
unsigned long	CheckSum;	    /*	output data checksum		*/
unsigned long	SymbolCount;    /*	symbol order counter        */

int F_format = FORMAT_DEFAULT;

/* -T option [phf] */
sortmode_t F_sortmode = SORTMODE_DEFAULT;
/* -E option [phf] */
errorformat_t F_errorformat = ERRORFORMAT_DEFAULT;

unsigned char	 F_verbose;
const char	*F_outfile = "a.out";
char	*F_listfile;
char	*F_symfile;
FILE	*FI_listfile;
FILE	*FI_temp;
unsigned char	 Fisclear;
unsigned long	 Plab, Pflags;

/*unsigned int	Adrbytes[]  = { 1, 2, 3, 2, 2, 2, 3, 3, 3, 2, 2, 2, 3, 1, 1, 2, 3 };*/
/* fallback addressing mode: if a mnemonic doesn't support the current mode,
   retry with Cvt[addrmode]. 0 = no fallback. */
unsigned int Cvt[] = {
    /* AM_IMP        = 0  */  0,          /* implied — no fallback */
    /* AM_IMM8       = 1  */  AM_IMM16,   /* 8-bit immediate ? try 16-bit */
    /* AM_IMM16      = 2  */  AM_IMM32,   /* 16-bit immediate ? try 32-bit */
    /* AM_BYTEADR    = 3  */  AM_WORDADR, /* zero-page ? try absolute */
    /* AM_BYTEADRX   = 4  */  AM_WORDADRX,/* zero-page,X ? try absolute,X */
    /* AM_BYTEADRY   = 5  */  AM_WORDADRY,/* zero-page,Y ? try absolute,Y */
    /* AM_WORDADR    = 6  */  AM_REL,     /* absolute ? try relative (for branch targets) */
    /* AM_WORDADRX   = 7  */  0,          /* absolute,X — no fallback */
    /* AM_WORDADRY   = 8  */  0,          /* absolute,Y — no fallback */
    /* AM_REL        = 9  */  0,          /* relative — no fallback */
    /* AM_BYTEREL    = 10 */  0,          /* 8-bit relative — no fallback */
    /* AM_INDBYTEX   = 11 */  0,          /* (indirect,X) — no fallback */
    /* AM_INDBYTEY   = 12 */  0,          /* (indirect),Y — no fallback */
    /* AM_INDWORD    = 13 */  0,          /* indirect absolute — no fallback */
    /* AM_INDWORDX   = 14 */  AM_IMM16,   /* indirect absolute,X ? try 16-bit imm */
    /* AM_INDBYTE    = 15 */  0,          /* indirect 8-bit — no fallback */
    /* AM_0X         = 16 */  AM_BYTEADRX,/* index X 0-bit ? try zero-page,X */
    /* AM_0Y         = 17 */  AM_BYTEADRY,/* index Y 0-bit ? try zero-page,Y */
    /* AM_BITMOD     = 18 */  0,          /* bit modification — no fallback */
    /* AM_BITBRAMOD  = 19 */  0,          /* bit-test+branch — no fallback */
    /* AM_BYTEADR_SP = 20 */  0,          /* SP+8-bit — no fallback */
    /* AM_WORDADR_SP = 21 */  0,          /* SP+16-bit — no fallback */
    /* AM_IMM32      = 22 */  0,          /* 32-bit immediate — no fallback */
    /* AM_REGX       = 23 */  0,          /* register X — no fallback */
    /* AM_REGUX      = 24 */  0,          /* register upper-X — no fallback */
    /* AM_REGY       = 25 */  0,          /* register Y — no fallback */
    /* AM_REGUY      = 26 */  0,          /* register upper-Y — no fallback */
    /* AM_REGXB      = 27 */  0,          /* register X byte — no fallback */
    /* AM_REGUXB     = 28 */  0,          /* register upper-X byte — no fallback */
    /* AM_REGYB      = 29 */  0,          /* register Y byte — no fallback */
    /* AM_REGUYB     = 30 */  0,          /* register upper-Y byte — no fallback */
    /* AM_REGXW      = 31 */  0,          /* register X word — no fallback */
    /* AM_REGUXW     = 32 */  0,          /* register upper-X word — no fallback */
    /* AM_REGYW      = 33 */  0,          /* register Y word — no fallback */
    /* AM_REGUYW     = 34 */  0,          /* register upper-Y word — no fallback */
    /* AM_REGXL      = 35 */  0,          /* register X long — no fallback */
    /* AM_REGUXL     = 36 */  0,          /* register upper-X long — no fallback */
    /* AM_REGYL      = 37 */  0,          /* register Y long — no fallback */
    /* AM_REGUYL          = 38 */  0,          /* register upper-Y long — no fallback */
    /* AM_W_FROM_WORDADR  = 39 */  0,          /* get word from word adr — no fallback */
    /* AM_L_FROM_WORDADR  = 40 */  0,          /* get long from word adr — no fallback */
    /* AM_IMP_L           = 41 */  0,          /* implied.l — no fallback */
    /* AM_IMP_W           = 42 */  0,          /* implied.w — no fallback */
};

/* number of operand bytes per addressing mode (not counting the opcode itself) */
unsigned int Opsize[] = {
    /* AM_IMP        = 0  */  0,  /* implied — no operand */
    /* AM_IMM8       = 1  */  1,  /* immediate 8-bit  — 1 operand byte  */
    /* AM_IMM16      = 2  */  2,  /* immediate 16-bit — 2 operand bytes */
    /* AM_BYTEADR    = 3  */  1,  /* zero-page / 8-bit address */
    /* AM_BYTEADRX   = 4  */  1,  /* zero-page,X */
    /* AM_BYTEADRY   = 5  */  1,  /* zero-page,Y */
    /* AM_WORDADR    = 6  */  2,  /* absolute 16-bit address */
    /* AM_WORDADRX   = 7  */  2,  /* absolute,X */
    /* AM_WORDADRY   = 8  */  2,  /* absolute,Y */
    /* AM_REL        = 9  */  2,  /* relative branch (handled separately) */
    /* AM_BYTEREL    = 10 */  1,  /* 8-bit relative */
    /* AM_INDBYTEX   = 11 */  1,  /* (indirect,X) */
    /* AM_INDBYTEY   = 12 */  1,  /* (indirect),Y */
    /* AM_INDWORD    = 13 */  2,  /* indirect absolute */
    /* AM_INDWORDX   = 14 */  2,  /* indirect absolute,X */
    /* AM_INDBYTE    = 15 */  1,  /* indirect 8-bit */
    /* AM_0X         = 16 */  0,  /* index X, 0-bit offset */
    /* AM_0Y         = 17 */  0,  /* index Y, 0-bit offset */
    /* AM_BITMOD     = 18 */  1,  /* bit modification */
    /* AM_BITBRAMOD  = 19 */  1,  /* bit-test + relative branch */
    /* AM_BYTEADR_SP = 20 */  1,  /* SP + 8-bit offset */
    /* AM_WORDADR_SP = 21 */  2,  /* SP + 16-bit offset */
    /* AM_IMM32      = 22 */  4,  /* immediate 32-bit — 4 operand bytes */
    /* AM_REGX       = 23 */  0,  /* register X as direct address */
    /* AM_REGUX      = 24 */  0,  /* register upper-X as direct address */
    /* AM_REGY       = 25 */  0,  /* register Y as direct address */
    /* AM_REGUY      = 26 */  0,  /* register upper-Y as direct address */
    /* AM_REGXB      = 27 */  0,  /* register X byte — 1 operand byte */
    /* AM_REGUXB     = 28 */  0,  /* register upper-X byte — 1 operand byte */
    /* AM_REGYB      = 29 */  0,  /* register Y byte — 1 operand byte */
    /* AM_REGUYB     = 30 */  0,  /* register upper-Y byte — 1 operand byte */
    /* AM_REGXW      = 31 */  0,  /* register X word — 2 operand bytes */
    /* AM_REGUXW     = 32 */  0,  /* register upper-X word — 2 operand bytes */
    /* AM_REGYW      = 33 */  0,  /* register Y word — 2 operand bytes */
    /* AM_REGUYW     = 34 */  0,  /* register upper-Y word — 2 operand bytes */
    /* AM_REGXL      = 35 */  0,  /* register X long — 4 operand bytes */
    /* AM_REGUXL     = 36 */  0,  /* register upper-X long — 4 operand bytes */
    /* AM_REGYL      = 37 */  0,  /* register Y long — 4 operand bytes */
    /* AM_REGUYL          = 38 */  0,  /* register upper-Y long — 4 operand bytes */
    /* AM_W_FROM_WORDADR  = 39 */  2,  /* get word from word adr — 2 operand bytes */
    /* AM_L_FROM_WORDADR  = 40 */  2,  /* get long from word adr — 2 operand bytes */
    /* AM_IMP_L           = 41 */  0,  /* implied.l — no operand */
    /* AM_IMP_W           = 42 */  0,  /* implied.w — no operand */
};

MNEMONIC Ops[] = {
    { NULL, v_list    , "list",           0,      0, {0,} },
    { NULL, v_include , "include",        0,      0, {0,} },
    { NULL, v_seg     , "seg",            0,      0, {0,} },
    { NULL, v_hex     , "hex",            0,      0, {0,} },
    { NULL, v_err     , "err",            0,      0, {0,} },
    { NULL, v_dc      , "dc",             0,      0, {0,} },
    { NULL, v_dc      , "byte",           0,      0, {0,} },
    { NULL, v_dc      , "word",           0,      0, {0,} },
    { NULL, v_dc      , "long",           0,      0, {0,} },
    { NULL, v_ds      , "ds",             0,      0, {0,} },
    { NULL, v_dc      , "dv",             0,      0, {0,} },
    { NULL, v_end     , "end",            0,      0, {0,} },
    { NULL, v_trace   , "trace",          0,      0, {0,} },
    { NULL, v_org     , "org",            0,      0, {0,} },
    { NULL, v_rorg    , "rorg",           0,      0, {0,} },
    { NULL, v_rend    , "rend",           0,      0, {0,} },
    { NULL, v_align   , "align",          0,      0, {0,} },
    { NULL, v_subroutine, "subroutine",   0,      0, {0,} },
    { NULL, v_equ     , "equ",            0,      0, {0,} },
    { NULL, v_equ     , "=",              0,      0, {0,} },
    { NULL, v_eqm     , "eqm",            0,      0, {0,} },
    { NULL, v_set     , "set",            0,      0, {0,} },
    { NULL, v_setstr  , "setstr",         0,      0, {0,} },
    { NULL, v_setsym  , "setsym",         0,      0, {0,} },
    { NULL, v_macro   , "mac",            MF_IF|MF_BEGM,  0, {0,} },
    { NULL, v_macro   , "macro",          MF_IF|MF_BEGM,  0, {0,} },
    { NULL, v_endm    , "endm",           MF_ENDM,0, {0,} },
    { NULL, v_mexit   , "mexit",          0,      0, {0,} },
    { NULL, v_ifconst , "ifconst",        MF_IF,  0, {0,} },
    { NULL, v_ifnconst, "ifnconst",       MF_IF,  0, {0,} },
    { NULL, v_ifnconst, "ifndef",         MF_IF,  0, {0,} },
    { NULL, v_if      , "if",             MF_IF,  0, {0,} },
    { NULL, v_else    , "else",           MF_IF,  0, {0,} },
    { NULL, v_endif   , "endif",          MF_IF,  0, {0,} },
    { NULL, v_endif   , "eif",            MF_IF,  0, {0,} },
    { NULL, v_repeat  , "repeat",         MF_IF,  0, {0,} },
    { NULL, v_repend  , "repend",         MF_IF,  0, {0,} },
    { NULL, v_echo    , "echo",           0,      0, {0,} },
    { NULL, v_processor,"processor",      0,      0, {0,} },
    { NULL, v_incbin  , "incbin",         0,      0, {0,} },
    { NULL, v_incdir  , "incdir",         0,      0, {0,} },
    MNEMONIC_NULL
};

