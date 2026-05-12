#ifndef SIF_H
#define SIF_H

#include "common.h"

enum {
/*00*/ SBC_NOP,
/*01*/ SBC_SUBR,
/*02*/ SBC_LEAVE,
/*03*/ SBC_LEAVE0,
/*04*/ SBC_JMP,
/*05*/ SBC_JMP16,
/*06*/ SBC_B,
/*07*/ SBC_B8,
/*08*/ SBC_BZ,
/*09*/ SBC_CALL,
/*0A*/ SBC_CALLIR,  //ignore return value version
/*0B*/ SBC_CALLT,   //call tagged
/*0C*/ SBC_CALLTIR,
/*0D*/ SBC_MCALL,
/*0E*/ SBC_MCALLIR, //ignore return value version
/*0F*/ SBC_MCALL8,
/*10*/ SBC_CLOSURE,
/*11*/ SBC_OBJECT,
/*12*/ SBC_LIST,
/*13*/ SBC_IFFXN,
/*14*/ SBC_CNAS,
/*15*/ SBC_ARGLIST,
/*16*/ SBC_ARGLIST8,
/*17*/ SBC_STARG,
/*18*/ SBC_STARG8,
/*19*/ SBC_MOVE,
/*1A*/ SBC_MOVE8,
/*1B*/ SBC_MOVEEMT,  //move empty list
/*1C*/ SBC_MOVENO,
/*1D*/ SBC_MOVETX,   //move text
/*1E*/ SBC_MOVETX8,
/*1F*/ SBC_MOVEMT,   //move methods table
/*20*/ SBC_MOVEMT8,
/*21*/ SBC_MOVEIM,   //move import
/*22*/ SBC_STOR,
/*23*/ SBC_STOR8,
/*24*/ SBC_LOAD,
/*25*/ SBC_LOAD8,
/*26*/ SBC_COPY,
/*27*/ SBC_FXNB0,
/*28*/ SBC_FXNB8,
/*29*/ SBC_FXNB16,
/*2A*/ SBC_FXNB32,
/*2B*/ SBC_FXN0,
/*2C*/ SBC_FXN8,
/*2D*/ SBC_FXN16,
/*2E*/ SBC_FXN32,
/*2F*/ SBC_IMMB64,
/*30*/ SBC_IMM64,
/*31*/ SBC_FXNTAG,
/*32*/ SBC_FXNLISTN,
/*33*/ SBC_ABS,
/*34*/ SBC_FXNLGET,
/*35*/ SBC_FXNLSET,
/*36*/ SBC_FXNLSETIR,
/*37*/ SBC_FXNSIZE,
/*38*/ SBC_NEG,
/*39*/ SBC_FXNADD,
/*3A*/ SBC_FXNSUB,
/*3B*/ SBC_FXNMUL,
/*3C*/ SBC_FXNDIV,
/*3D*/ SBC_FXNREM,
/*3E*/ SBC_IMMEQ,
/*3F*/ SBC_IMMNE,
/*40*/ SBC_FXNLT,
/*41*/ SBC_FXNGT,
/*42*/ SBC_FXNLTE,
/*43*/ SBC_FXNGTE,
/*44*/ SBC_FXNAND,
/*45*/ SBC_FXNIOR,
/*46*/ SBC_FXNXOR,
/*47*/ SBC_FXNSHL,
/*48*/ SBC_FXNSHR,
/*49*/ SBC_NLDU1, //foreign get unsigned
/*4A*/ SBC_NLDU2,
/*4B*/ SBC_NLDU4,
/*4C*/ SBC_NSTU1, //foreign set unsigned
/*4D*/ SBC_NSTU2,
/*4E*/ SBC_NSTU4,
/*4F*/ SBC_NLDS1, //foreign get signed
/*50*/ SBC_NLDS2,
/*51*/ SBC_NLDS4,
/*52*/ SBC_NSTS1, //foreign set signed
/*53*/ SBC_NSTS2,
/*54*/ SBC_NSTS4,
/*55*/ SBC_TINIT, //type init
/*56*/ SBC_TINITI, //type init for immediate text
/*57*/ SBC_SUBTYPE,
/*58*/ SBC_DMET,
/*59*/ SBC_GID,
/*5A*/ SBC_CURMET, // currently executing method
/*5B*/ SBC_MNAME,
/*5C*/ SBC_FATAL,
/*5D*/ SBC_GC0, //turn gc on
/*5E*/ SBC_GC1, //turn gc off

//opcodes for pushing native arguments
/*5F*/ SBC_NFI, //native function function invocation
/*60*/ SBC_NFI_VOID,
/*61*/ SBC_NFI_INT,
/*62*/ SBC_NFI_PTR,
/*63*/ SBC_NFI_TXT,
/*64*/ SBC_NFI_U4,
/*65*/ SBC_NFI_FLT,
/*66*/ SBC_NFI_DBL,
/*67*/ SBC_TCALL,  //tail call
/*68*/ SBC_TMCALL, //method tail call
/*69*/ SBC_LEAVEN, //leave constant value
/*6A*/ SBC_LD4_0,
/*6B*/ SBC_LD4_1,
/*6C*/ SBC_LD4_2,
/*6D*/ SBC_LD4_3,
/*6E*/ SBC_LD4_4,
/*6F*/ SBC_LD4_5,
/*70*/ SBC_LD4_6,
/*71*/ SBC_LD4_7,
/*72*/ SBC_LD4_8,
/*73*/ SBC_LD4_9,
/*74*/ SBC_LD4_A,
/*75*/ SBC_LD4_B,
/*76*/ SBC_LD4_C,
/*77*/ SBC_LD4_D,
/*78*/ SBC_LD4_E,
/*79*/ SBC_LD4_F,
/*7A*/ SBC_ST4_0,
/*7B*/ SBC_ST4_1,
/*7C*/ SBC_ST4_2,
/*7D*/ SBC_ST4_3,
/*7E*/ SBC_ST4_4,
/*7F*/ SBC_ST4_5,
/*80*/ SBC_ST4_6,
/*81*/ SBC_ST4_7,
/*82*/ SBC_ST4_8,
/*83*/ SBC_ST4_9,
/*84*/ SBC_ST4_A,
/*85*/ SBC_ST4_B,
/*86*/ SBC_ST4_C,
/*87*/ SBC_ST4_D,
/*88*/ SBC_ST4_E,
/*89*/ SBC_ST4_F,
/*8A*/ SBC_ARGLIST0,
/*8B*/ SBC_ARGLIST1,
/*8C*/ SBC_ARGLIST2,
/*8D*/ SBC_ARGLIST3,
/*8E*/ SBC_ARGLIST4,
/*8F*/ SBC_ARGLIST5,
/*90*/ SBC_FXT8,
/*91*/ SBC_FXT16,
/*92*/ SBC_FXT24,
/*93*/ SBC_FXT32,
/*94*/ SBC_FXT40,
/*95*/ SBC_FXT48,
/*96*/ SBC_FXT56,
/*97*/ SBC_MOVE4,
/*98*/ SBC_NLD, //native load
/*99*/ SBC_NST, //native store
/*9A*/ SBC_CTX, //context manipulation

//virtual opcodes
/*9B*/ SBC_NOT,
/*9C*/ SBC_GOT,
/*9D*/ SBC_NO,
/*9E*/ SBC_INC,
/*9F*/ SBC_DEC,
/*A0*/ SBC_UNUSEDA0,
/*A1*/ SBC_UNUSEDA1,
/*A2*/ SBC_UNUSEDA2,
/*A3*/ SBC_UNUSEDA3,
/*A4*/ SBC_UNUSEDA4,
/*A5*/ SBC_UNUSEDA5,
/*A6*/ SBC_UNUSEDA6,
/*A7*/ SBC_UNUSEDA7,

/*A8*/ SBC_SAME, //1 if handles equal
/*A9*/ SBC_VARY, //0 if handles equal

/*AA*/ SBC_END,


  SBC_BS,
  SBC_T,      //table
  SBC_LDFXN,  //virtual opcode, which compiles to SBC_IMM
  SBC_LDFLT,  //virtual opcode, which compiles to SBC_IMM
  SBC_NFA,    //native function argument
  SBC_NFR,    //native function result
  SBC_SRC,    //file source
  SBC_DEPS,   //file deps
  SBC_EXPORT, //file exports
  SBC_INCL,   //associated .h header
  SBC_SETJMP,
  SBC_LONGJMP,
  SBC_IGNORE
};


#define SBC_DEFAULT_SRC 0x8000

typedef struct instr_t {
  int opcode;    //interger opcode
  char **args;   //arguments to this instruction
  char **labels; //labels referencing this instruction
  int row;       //row in the sif file, this instruction originates from
  int ofs;       //offset in the SBC file
} instr_t;

typedef struct { char *key; int value; } *labels_t;

typedef struct sif_t { //Parsed Symta Instructions File
//FIXME: some code to free the sif file after use.
  instr_t *instrs;
  labels_t labels;
} sif_t;

typedef struct {
  method_node_t *node; //this can be replaced with an int index to method page
} __attribute__((packed)) mcache_t;

#define SBC_MCACHE

typedef struct sbc_t {
  char *filename;
  uint8_t *file; //FIXME: file body can be attached to the end of sbc_t
  uint8_t *code;
  int code_sz;
  uint8_t *data;
  int data_sz;
  uint8_t *tbls;
  int tbls_sz;
  uint8_t *fntbl;
  int fntbl_sz;
  uint8_t *tot;
  uint32_t setup;
  uint32_t entry;
  uint64_t *hooks; //array of offsets into hooks_base
  uint8_t *nrs; //native functions relocations
  int nrs_sz;
  tot_entry_t rtot[7]; //relocated tot
  dyn *tx;
  dyn *ty;
  int *mt;
  dyn *libs;
  dyn *imlibs;
  dyn *im;
  int ready; //is this symta bytecode module ready for execution?
  void **nfi_trmps; //nfi trampoulines
  void *nfi_ctx; //nfi context
  int src_file_string; //offset of the name of the src filed used to build sbc
  int deps_string;
  int export_string;
} sbc_t;

typedef struct { int key; char *value; } *rlabels_t;

typedef struct dasm_t { //disassembler state
  sbc_t *sbc;
  uint8_t *pin;
  instr_t ins;
  rlabels_t o2l;
  int row;
  int arglist_bits;
  int arglist_i;
  int arglist_nargs;
} dasm_t;

void sif_loader_init();
void instr_free(instr_t *ins);
void sif_print(instr_t *ins);
sif_t *sif_parse(char *file, int file_size);
void sif_free(sif_t *sif);
uint8_t *sif2sbc(sif_t *sif);
sif_t *sbc2sif(char *filename);
dyn sbc_exec_file(char *path);
dyn sbc_exec(sbc_t *sbc);
sbc_t *sbc_new(uint8_t *pin, int64_t size, char *path);
sbc_t *sbc_load(char *path);
void sbc_free(sbc_t *sbc);
void sbc_dasm(sbc_t *sbc, uint8_t *pin, int n);
void sbc_dasm_sub(dasm_t *dasm);
int sbc_load_metadata(char *filename, char **src, char **deps, char **export);
dyn sbc_exec_fn(uint8_t *bytecode);
void sbc_dasm_fn(uint8_t *pin, int n);

void sbc_init_types(sbc_t *sbc, void *tables);

typedef struct { char *key; int value; } asmsbc_t;

typedef struct { int key; char *value; } sbcasm_t;

extern asmsbc_t *asmsbc;
extern sbcasm_t *sbcasm;

//#define SBC_TRANSITION


#endif