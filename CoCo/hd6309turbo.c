/*
Copyright 2018 by Walter Zambotti
This file is part of VCC (Virtual Color Computer).

    VCC (Virtual Color Computer) is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    VCC (Virtual Color Computer) is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with VCC (Virtual Color Computer).  If not, see <http://www.gnu.org/licenses/>.
*/

#include <stdio.h>
#include <stdint.h>
#include "hd6309.h"
#include "hd6309defs.h"

#if defined(_WIN64)
#define MSABI 
#else
#define MSABI __attribute__((ms_abi))
#endif

#define UINT8 uint8_t
#define UINT16 uint16_t
#define UINT32 uint32_t
#define UINT64 uint64_t

//Global variables for CPU Emulation-----------------------
#define NTEST8(r) r>0x7F;
#define NTEST16(r) r>0x7FFF;
#define NTEST32(r) r>0x7FFFFFFF;
#define OVERFLOW8(c,a,b,r) c ^ (((a^b^r)>>7) &1);
#define OVERFLOW16(c,a,b,r) c ^ (((a^b^r)>>15)&1);
#define ZTEST(r) !r;

#define DPADDRESS(r) (dp.Reg |MemRead8_s(r))
#define IMMADDRESS(r) MemRead16_s(r)
#define INDADDRESS(r) CalculateEA(MemRead8_s(r))

#define M65		0
#define M64		1
#define M32		2
#define M21		3
#define M54		4
#define M97		5
#define M85		6
#define M51		7
#define M31		8
#define M1110	9
#define M76		10
#define M75		11
#define M43		12
#define M87		13
#define M86		14
#define M98		15
#define M2726	16
#define M3635	17
#define M3029	18
#define M2827	19
#define M3726	20
#define M3130	21

typedef union
{
	unsigned short Reg;
	struct
	{
		unsigned char lsb,msb;
	} B;
} cpuregister;

typedef union
{
	unsigned int Reg;
	struct
	{
		unsigned short msw,lsw;
	} Word;
	struct
	{
		unsigned char mswlsb,mswmsb,lswlsb,lswmsb;	//Might be backwards
	} Byte;
} wideregister;

#define D_REG	q_s.Word.lsw
#define W_REG	q_s.Word.msw
#define PC_REG	pc_s.Reg
#define X_REG	x_s.Reg
#define Y_REG	y_s.Reg
#define U_REG	u_s.Reg
#define S_REG	s_s.Reg
#define A_REG	q_s.Byte.lswmsb
#define B_REG	q_s.Byte.lswlsb
#define E_REG	q_s.Byte.mswmsb
#define F_REG	q_s.Byte.mswlsb	
#define Q_REG	q_s.Reg
#define V_REG	v_s.Reg
#define O_REG	z_s.Reg
static char RegName[16][10]={"D","X","Y","U","S","PC","W","V","A","B","CC","DP","ZERO","ZERO","E","F"};

extern void Neg_D_A(void);
extern void Oim_D_A(void);
extern void Aim_D_A(void);
extern void Com_D_A(void);
extern void Lsr_D_A(void);
extern void Eim_D_A(void);
extern void Ror_D_A(void);
extern void Asr_D_A(void);
extern void Asl_D_A(void);
extern void Rol_D_A(void);
extern void Dec_D_A(void);
extern void Tim_D_A(void);
extern void Inc_D_A(void);
extern void Tst_D_A(void);
extern void Jmp_D_A(void);
extern void Clr_D_A(void);
extern void Nop_I_A(void);
extern void Sync_I_A(void);
extern void Sexw_I_A(void);
extern void Lbra_R_A(void);
extern void Lbsr_R_A(void);
extern void Daa_I_A(void);
extern void Orcc_M_A(void);
extern void Andcc_M_A(void);
extern void Sex_I_A(void);
extern void Exg_M_A(void);
extern void Tfr_M_A(void);
extern void Bra_R_A(void);
extern void Brn_R_A(void);
extern void Bhi_R_A(void);
extern void Bls_R_A(void);
extern void Bhs_R_A(void);
extern void Blo_R_A(void);
extern void Bne_R_A(void);
extern void Beq_R_A(void);
extern void Bvc_R_A(void);
extern void Bvs_R_A(void);
extern void Bpl_R_A(void);
extern void Bmi_R_A(void);
extern void Bge_R_A(void);
extern void Blt_R_A(void);
extern void Bgt_R_A(void);
extern void Ble_R_A(void);
extern void Leax_X_A(void);
extern void Leay_X_A(void);
extern void Leas_X_A(void);
extern void Leau_X_A(void);
extern void Pshs_M_A(void);
extern void Puls_M_A(void);
extern void Pshu_M_A(void);
extern void Pulu_M_A(void);
extern void Rts_I_A(void);
extern void Abx_I_A(void);
extern void Rti_I_A(void);
extern void Cwai_I_A(void);
extern void Mul_I_A(void);
extern void Swi1_I_A(void);
extern void Nega_I_A(void);
extern void Coma_I_A(void);
extern void Lsra_I_A(void);
extern void Rora_I_A(void);
extern void Asra_I_A(void);
extern void Asla_I_A(void);
extern void Rola_I_A(void);
extern void Deca_I_A(void);
extern void Inca_I_A(void);
extern void Tsta_I_A(void);
extern void Clra_I_A(void);
extern void Negb_I_A(void);
extern void Comb_I_A(void);
extern void Lsrb_I_A(void);
extern void Rorb_I_A(void);
extern void Asrb_I_A(void);
extern void Aslb_I_A(void);
extern void Rolb_I_A(void);
extern void Decb_I_A(void);
extern void Incb_I_A(void);
extern void Tstb_I_A(void);
extern void Clrb_I_A(void);
extern void Neg_X_A(void);
extern void Oim_X_A(void);
extern void Aim_X_A(void);
extern void Com_X_A(void);
extern void Lsr_X_A(void);
extern void Eim_X_A(void);
extern void Ror_X_A(void);
extern void Asr_X_A(void);
extern void Asl_X_A(void);
extern void Rol_X_A(void);
extern void Dec_X_A(void);
extern void Tim_X_A(void);
extern void Inc_X_A(void);
extern void Tst_X_A(void);
extern void Jmp_X_A(void);
extern void Clr_X_A(void);
extern void Neg_E_A(void);
extern void Oim_E_A(void);
extern void Aim_E_A(void);
extern void Com_E_A(void);
extern void Lsr_E_A(void);
extern void Eim_E_A(void);
extern void Ror_E_A(void);
extern void Asr_E_A(void);
extern void Asl_E_A(void);
extern void Rol_E_A(void);
extern void Dec_E_A(void);
extern void Tim_E_A(void);
extern void Inc_E_A(void);
extern void Tst_E_A(void);
extern void Jmp_E_A(void);
extern void Clr_E_A(void);
extern void Suba_M_A(void);
extern void Cmpa_M_A(void);
extern void Sbca_M_A(void);
extern void Suba_M_A(void);
extern void Subd_M_A(void);
extern void Anda_M_A(void);
extern void Bita_M_A(void);
extern void Lda_M_A(void);
extern void Eora_M_A(void);
extern void Adca_M_A(void);
extern void Ora_M_A(void);
extern void Adda_M_A(void);
extern void Cmpx_M_A(void);
extern void Bsr_R_A(void);
extern void Ldx_M_A(void);
extern void Suba_D_A(void);
extern void Cmpa_D_A(void);
extern void Sbca_D_A(void);
extern void Subd_D_A(void);
extern void Anda_D_A(void);
extern void Bita_D_A(void);
extern void Lda_D_A(void);
extern void Sta_D_A(void);
extern void Eora_D_A(void);
extern void Adca_D_A(void);
extern void Ora_D_A(void);
extern void Adda_D_A(void);
extern void Cmpx_D_A(void);
extern void Jsr_D_A(void);
extern void Ldx_D_A(void);
extern void Stx_D_A(void);
extern void Suba_X_A(void);
extern void Cmpa_X_A(void);
extern void Sbca_X_A(void);
extern void Subd_X_A(void);
extern void Anda_X_A(void);
extern void Bita_X_A(void);
extern void Lda_X_A(void);
extern void Sta_X_A(void);
extern void Eora_X_A(void);
extern void Adca_X_A(void);
extern void Ora_X_A(void);
extern void Adda_X_A(void);
extern void Cmpx_X_A(void);
extern void Jsr_X_A(void);
extern void Ldx_X_A(void);
extern void Stx_X_A(void);
extern void Suba_E_A(void);
extern void Cmpa_E_A(void);
extern void Sbca_E_A(void);
extern void Subd_E_A(void);
extern void Anda_E_A(void);
extern void Bita_E_A(void);
extern void Lda_E_A(void);
extern void Sta_E_A(void);
extern void Eora_E_A(void);
extern void Adca_E_A(void);
extern void Ora_E_A(void);
extern void Adda_E_A(void);
extern void Cmpx_E_A(void);
extern void Jsr_E_A(void);
extern void Ldx_E_A(void);
extern void Stx_E_A(void);
extern void Subb_M_A(void);
extern void Cmpb_M_A(void);
extern void Sbcb_M_A(void);
extern void Addd_M_A(void);
extern void Andb_M_A(void);
extern void Bitb_M_A(void);
extern void Ldb_M_A(void);
extern void Eorb_M_A(void);
extern void Adcb_M_A(void);
extern void Orb_M_A(void);
extern void Addb_M_A(void);
extern void Ldd_M_A(void);
extern void Ldq_M_A(void);
extern void Ldu_M_A(void);
extern void Subb_D_A(void);
extern void Cmpb_D_A(void);
extern void Sbcb_D_A(void);
extern void Addd_D_A(void);
extern void Andb_D_A(void);
extern void Bitb_D_A(void);
extern void Ldb_D_A(void);
extern void Stb_D_A(void);
extern void Eorb_D_A(void);
extern void Adcb_D_A(void);
extern void Orb_D_A(void);
extern void Addb_D_A(void);
extern void Ldd_D_A(void);
extern void Std_D_A(void);
extern void Ldu_D_A(void);
extern void Stu_D_A(void);
extern void Subb_X_A(void);
extern void Cmpb_X_A(void);
extern void Sbcb_X_A(void);
extern void Addd_X_A(void);
extern void Andb_X_A(void);
extern void Bitb_X_A(void);
extern void Ldb_X_A(void);
extern void Stb_X_A(void);
extern void Eorb_X_A(void);
extern void Adcb_X_A(void);
extern void Orb_X_A(void);
extern void Addb_X_A(void);
extern void Ldd_X_A(void);
extern void Std_X_A(void);
extern void Ldu_X_A(void);
extern void Stu_X_A(void);
extern void Subb_E_A(void);
extern void Cmpb_E_A(void);
extern void Sbcb_E_A(void);
extern void Addd_E_A(void);
extern void Andb_E_A(void);
extern void Bitb_E_A(void);
extern void Ldb_E_A(void);
extern void Stb_E_A(void);
extern void Eorb_E_A(void);
extern void Adcb_E_A(void);
extern void Orb_E_A(void);
extern void Addb_E_A(void);
extern void Ldd_E_A(void);
extern void Std_E_A(void);
extern void Ldu_E_A(void);
extern void Stu_E_A(void);
extern void LBrn_R_A(void);
extern void LBhi_R_A(void);
extern void LBls_R_A(void);
extern void LBhs_R_A(void);
extern void LBcs_R_A(void);
extern void LBne_R_A(void);
extern void LBeq_R_A(void);
extern void LBvc_R_A(void);
extern void LBvs_R_A(void);
extern void LBpl_R_A(void);
extern void LBmi_R_A(void);
extern void LBge_R_A(void);
extern void LBlt_R_A(void);
extern void LBgt_R_A(void);
extern void LBle_R_A(void);
extern void Addr_A(void);
extern void Adcr_A(void);
extern void Subr_A(void);
extern void Sbcr_A(void);
extern void Andr_A(void);
extern void Orr_A(void);
extern void Eorr_A(void);
extern void Cmpr_A(void);
extern void Pshsw_A(void);
extern void Pulsw_A(void);
extern void Pshuw_A(void);
extern void Puluw_A(void);
extern void Swi2_I_A(void);
extern void Negd_I_A(void);
extern void Comd_I_A(void);
extern void Lsrd_I_A(void);
extern void Rord_I_A(void);
extern void Asrd_I_A(void);
extern void Asld_I_A(void);
extern void Rold_I_A(void);
extern void Decd_I_A(void);
extern void Incd_I_A(void);
extern void Tstd_I_A(void);
extern void Clrd_I_A(void);
extern void Comw_I_A(void);
extern void Lsrw_I_A(void);
extern void Rorw_I_A(void);
extern void Rolw_I_A(void);
extern void Decw_I_A(void);
extern void Incw_I_A(void);
extern void Tstw_I_A(void);
extern void Clrw_I_A(void);
extern void Subw_M_A(void);
extern void Cmpw_M_A(void);
extern void Sbcd_M_A(void);
extern void Cmpd_M_A(void);
extern void Andd_M_A(void);
extern void Bitd_M_A(void);
extern void Ldw_M_A(void);
extern void Eord_M_A(void);
extern void Adcd_M_A(void);
extern void Ord_M_A(void);
extern void Addw_M_A(void);
extern void Cmpy_M_A(void);
extern void Ldy_M_A(void);
extern void Subw_D_A(void);
extern void Cmpw_D_A(void);
extern void Sbcd_D_A(void);
extern void Cmpd_D_A(void);
extern void Andd_D_A(void);
extern void Bitd_D_A(void);
extern void Ldw_D_A(void);
extern void Stw_D_A(void);
extern void Eord_D_A(void);
extern void Adcd_D_A(void);
extern void Ord_D_A(void);
extern void Addw_D_A(void);
extern void Cmpy_D_A(void);
extern void Ldy_D_A(void);
extern void Sty_D_A(void);
extern void Subw_X_A(void);
extern void Cmpw_X_A(void);
extern void Sbcd_X_A(void);
extern void Cmpd_X_A(void);
extern void Andd_X_A(void);
extern void Bitd_X_A(void);
extern void Ldw_X_A(void);
extern void Stw_X_A(void);
extern void Eord_X_A(void);
extern void Adcd_X_A(void);
extern void Ord_X_A(void);
extern void Addw_X_A(void);
extern void Cmpy_X_A(void);
extern void Ldy_X_A(void);
extern void Sty_X_A(void);
extern void Subw_E_A(void);
extern void Cmpw_E_A(void);
extern void Sbcd_E_A(void);
extern void Cmpd_E_A(void);
extern void Andd_E_A(void);
extern void Bitd_E_A(void);
extern void Ldw_E_A(void);
extern void Stw_E_A(void);
extern void Eord_E_A(void);
extern void Adcd_E_A(void);
extern void Ord_E_A(void);
extern void Addw_E_A(void);
extern void Cmpy_E_A(void);
extern void Ldy_E_A(void);
extern void Sty_E_A(void);
extern void Lds_M_A(void);
extern void Ldq_D_A(void);
extern void Stq_D_A(void);
extern void Lds_D_A(void);
extern void Sts_D_A(void);
extern void Ldq_X_A(void);
extern void Stq_X_A(void);
extern void Lds_X_A(void);
extern void Sts_X_A(void);
extern void Ldq_E_A(void);
extern void Stq_E_A(void);
extern void Lds_E_A(void);
extern void Sts_E_A(void);
extern void Band_A(void);
extern void Biand_A(void);
extern void Bor_A(void);
extern void Bior_A(void);
extern void Beor_A(void);
extern void Bieor_A(void);
extern void Ldbt_A(void);
extern void Stbt_A(void);
extern void Tfm1_A(void);
extern void Tfm2_A(void);
extern void Tfm3_A(void);
extern void Tfm4_A(void);
extern void Bitmd_M_A(void);
extern void Ldmd_M_A(void);
extern void Swi3_I_A(void);
extern void Come_I_A(void);
extern void Dece_I_A(void);
extern void Ince_I_A(void);
extern void Tste_I_A(void);
extern void Clre_I_A(void);
extern void Comf_I_A(void);
extern void Decf_I_A(void);
extern void Incf_I_A(void);
extern void Tstf_I_A(void);
extern void Clrf_I_A(void);
extern void Sube_M_A(void);
extern void Cmpe_M_A(void);
extern void Cmpu_M_A(void);
extern void Lde_M_A(void);
extern void Adde_M_A(void);
extern void Cmps_M_A(void);
extern void Divd_M_A(void);
extern void Divq_M_A(void);
extern void Muld_M_A(void);
extern void Sube_D_A(void);
extern void Cmpe_D_A(void);
extern void Cmpu_D_A(void);
extern void Lde_D_A(void);
extern void Ste_D_A(void);
extern void Adde_D_A(void);
extern void Cmps_D_A(void);
extern void Divd_D_A(void);
extern void Divq_D_A(void);
extern void Muld_D_A(void);
extern void Sube_X_A(void);
extern void Cmpe_X_A(void);
extern void Cmpu_X_A(void);
extern void Lde_X_A(void);
extern void Ste_X_A(void);
extern void Adde_X_A(void);
extern void Cmps_X_A(void);
extern void Divd_X_A(void);
extern void Divq_X_A(void);
extern void Muld_X_A(void);
extern void Sube_E_A(void);
extern void Cmpe_E_A(void);
extern void Cmpu_E_A(void);
extern void Lde_E_A(void);
extern void Ste_E_A(void);
extern void Adde_E_A(void);
extern void Cmps_E_A(void);
extern void Divd_E_A(void);
extern void Divq_E_A(void);
extern void Muld_E_A(void);
extern void Subf_M_A(void);
extern void Cmpf_M_A(void);
extern void Ldf_M_A(void);
extern void Addf_M_A(void);
extern void Subf_D_A(void);
extern void Cmpf_D_A(void);
extern void Ldf_D_A(void);
extern void Stf_D_A(void);
extern void Addf_D_A(void);
extern void Subf_X_A(void);
extern void Cmpf_X_A(void);
extern void Ldf_X_A(void);
extern void Stf_X_A(void);
extern void Addf_X_A(void);
extern void Subf_E_A(void);
extern void Cmpf_E_A(void);
extern void Ldf_E_A(void);
extern void Stf_E_A(void);
extern void Addf_E_A(void);

#define CFx68 0x01
#define VFx68 0x02
#define ZFx68 0x04
#define NFx68 0x08
#define IFx68 0x10
#define HFx68 0x20
#define FFx68 0x40
#define EFx68 0x80
#define CFx86 0x01
#define HFx86 0x10
#define ZFx86 0x40
#define NFx86 0x80
#define VFx86 0x800

UINT16 x86Flags;
UINT8 SyncWaiting_s = 0;
unsigned short *xfreg16_s[8];
unsigned char *ureg8_s[8];

wideregister q_s;
cpuregister pc_s, x_s, y_s, u_s, s_s, dp_s, v_s, z_s;

#define MD_NATIVE6309_BIT 0x01
#define MD_FIRQMODE_BIT 0x02
#define MD_ILLEGALINST_BIT 0x40
#define MD_DIVBYZERO_BIT 0x80

#define MD_NATIVE6309 (mdbits_s & MD_NATIVE6309_BIT)
#define MD_FIRQMODE (mdbits_s & MD_FIRQMODE_BIT)
#define MD_ILLEGALINST (mdbits_s & MD_ILLEGALINST_BIT)
#define MD_DIVBYZERO (mdbits_s & MD_DIVBYZERO_BIT)

static unsigned char InsCycles[2][25];
unsigned char cc_s[8];
//static unsigned int md_s[8];
//static unsigned char *ureg8[8]; 
//static unsigned char ccbits,mdbits;
unsigned char ccbits_s, mdbits_s;
//static unsigned short *xfreg16[8];
int CycleCounter=0;
//static unsigned int SyncWaiting=0;
unsigned short temp16;
static signed short stemp16;
static signed char stemp8;
static unsigned int  temp32;
static unsigned char temp8; 
static unsigned char PendingInterupts=0;
static unsigned char IRQWaiter=0;
static unsigned char Source=0,Dest=0;
static unsigned char postbyte=0;
static short unsigned postword=0;
static signed char *spostbyte=(signed char *)&postbyte;
static signed short *spostword=(signed short *)&postword;
char InInterupt_s=0;
int gCycleFor;
//END Global variables for CPU Emulation-------------------

//Fuction Prototypes---------------------------------------
void IgnoreInsHandler(void);
unsigned short CalculateEA_s(unsigned char);
extern void MSABI InvalidInsHandler_s(void);
void MSABI DivbyZero_s(void);
void MSABI ErrorVector_s(void);
void setcc_s(UINT8);
UINT8 getcc_s(void);
void setmd_s(UINT8);
UINT8 getmd_s(void);
static void cpu_firq_s(void);
static void cpu_irq_s(void);
static void cpu_nmi_s(void);
unsigned char GetSorceReg_s(unsigned char);
void Page_2_s(void);
void Page_3_s(void);
void MSABI MemWrite8_s(unsigned char, unsigned short);
void MSABI MemWrite16_s(unsigned short, unsigned short);
void MSABI MemWrite32_s(unsigned int, unsigned short);
// unsigned char MemRead8_s(unsigned short);
// unsigned short MemRead16_s(unsigned short);
// unsigned int MemRead32_s(unsigned short);
extern UINT8 /*MemRead8(UINT16),*/ MSABI MemRead8_s(UINT16);
extern UINT16 /*MemRead16(UINT16),*/ MSABI MemRead16_s(UINT16);
extern UINT32 /*MemRead32(UINT16),*/ MSABI MemRead32_s(UINT16);

//unsigned char GetDestReg(unsigned char);
//END Fuction Prototypes-----------------------------------

void HD6309Reset_s(void)
{
	char index;
	for(index=0;index<=6;index++)		//Set all register to 0 except V
		*xfreg16_s[index] = 0;
	for(index=0;index<=7;index++)
		*ureg8_s[index]=0;
	x86Flags = 0;
	//for(index=0;index<=7;index++)
	//	md_s[index]=0;
	mdbits_s =getmd_s();
	mdbits_s = 0;
	dp_s.Reg=0;
	cc_s[I]=1;
	cc_s[F]=1;
	SyncWaiting_s=0;
	PC_REG=MemRead16_s(VRESET);	//PC gets its reset vector // this needs to be uncommented
	SetMapType(0);	//shouldn't be here
	return;
}

void HD6309Init_s(void)
{	//Call this first or RESET will core!
	// reg pointers for TFR and EXG and LEA ops
	xfreg16_s[0] = &D_REG;
	xfreg16_s[1] = &X_REG;
	xfreg16_s[2] = &Y_REG;
	xfreg16_s[3] = &U_REG;
	xfreg16_s[4] = &S_REG;
	xfreg16_s[5] = &PC_REG;
	xfreg16_s[6] = &W_REG;
	xfreg16_s[7] = &V_REG;

	ureg8_s[0]=(unsigned char*)&A_REG;
	ureg8_s[1]=(unsigned char*)&B_REG;
	ureg8_s[2]=(unsigned char*)&ccbits_s;
	ureg8_s[3]=(unsigned char*)&dp_s.B.msb;
	ureg8_s[4]=(unsigned char*)&O_REG;
	ureg8_s[5]=(unsigned char*)&O_REG;
	ureg8_s[6]=(unsigned char*)&E_REG;
	ureg8_s[7]=(unsigned char*)&F_REG;
	cc_s[I]=1;
	cc_s[F]=1;

	return;
}


void Reset_s(void) // 3E
{	//Undocumented
	HD6309Reset_s();
}

short instcyclemu0[256] = 
{
	6, // Neg_D 00
	6, // Oim_D 01
	6, // Aim_D 02
	6, // Com_D 03
	6, // Lsr_D 04
	6, // Eim_D 05
	6, // Ror_D 06
	6, // Asr_D 07
	6, // Asl_D 08
	6, // Rol_D 09
	6, // Dec_D 0A
	6, // Tim_D 0B
	6, // Inc_D 0C
	6, // Tst_D 0D
	3, // Jmp_D 0E
	6, // Clr_D 0F
	0, // Page2 10
	0, // Page3 11
	2, // Nop 12
	0, // Sync 13
	4, // Sexw 14
	1, // Invalid 15
	5, // Lbra 16
	9, // Lbsr 17
	2, // Invalid 18
	2, // Daa 19
	3, // Orcc 1A
	2, // Invalid 1B
	3, // Andcc 1C
	2, // Sex 1D
	8, // Exg 1E
	6, // Tfr 1F
	3, // Bra 20
	3, // Brn 21
	3, // Bhi 22
	3, // Bls 23
	3, // Bhs 24
	3, // Blo 25
	3, // Bne 26
	3, // Beq 27
	3, // Bvc 28
	3, // Bvs 29
	3, // Bpl 2A
	3, // Bmi 2B
	3, // Bge 2C
	3, // Blt 2D
	3, // Bgt 2E
	3, // Ble 2F
	4, // Leax 30
	4, // Leay 31
	4, // Leas 32
	4, // Leau 33
	5, // Pshs 34
	5, // Puls 35
	5, // Pshu 36
	5, // Puls 37
	2, // Invalid 38
	5, // Rts 39
	3, // Abx 3A
	6, // Rti 3B
	22, // Cwai 3C
	11, // Mul_I 3D
	2, // Invalid 3E
	19, // Swi1 3F
	2, // Nega_I 40
	2, // Invalid 41
	2, // Invalid 42
	2, // Coma_I 43
	2, // Lsra_I 44
	2, // Invalid 45
	2, // Rora_I 46
	2, // Asra_I 47
	2, // Asla_I 48
	2, // Rola_I 49
	2, // Deca_I 4A
	2, // Invalid 4B
	2, // Inca_I 4C
	2, // Tsta_I 4D
	2, // Invalid 4E
	2, // Clra_I 4F
	2, // Negb_I 50
	2, // Invalid 51
	2, // Invalid 52
	2, // Comb_I 53
	2, // Lsrb_I 54
	2, // Invalid 55
	2, // Rorb_I 56
	2, // Asrb_I 57
	2, // Aslb_I 58
	2, // Rolb_I 59
	2, // Decb_I 5A
	2, // Invalid 5B
	2, // Incb_I 5C
	2, // Tstb_I 5D
	2, // Invalid 5E
	2, // Clrb_I 5F
	6, // Neg_X 60
	7, // Oim_X 61
	7, // Aim_X 62
	6, // Com_X 63
	6, // Lsr_X 64
	7, // Eim_X 65
	6, // Ror_X 66
	6, // Asr_X 67
	6, // Asl_X 68
	6, // Rol_X 69
	6, // Dec_X 6A
	7, // Tim_X 6B
	6, // Inc_X 6C
	6, // Tst_X 6D
	3, // Jmp_X 6E
	6, // Clr_X 6F
	7, // Neg_E 70
	7, // Oim_E 71
	7, // Aim_E 72
	7, // Com_E 73
	7, // Lsr_E 74
	7, // Eim_E 75
	7, // Ror_E 76
	7, // Asr_E 77 
	7, // Asl_E 78
	7, // Rol_E 79
	7, // Dec_E 7A
	7, // Tim_E 7B
	7, // Inc_E 7C
	7, // Tst_E 7D
	4, // Jmp_E 7E
	7, // Clr_E 7F
	2, // Suba_M 80
	2, // Cmpa_M 81
	2, // Sbca_M 82
	4, // Subd_M 83
	2, // Anda_M 84
	2, // Bita_M 85
	2, // Lda_M 86
	2, // Invalid 87
	2, // Eora_M 88
	2, // Adca_M 89
	2, // Ora_M 8A
	2, // Adda_M 8B
	4, // Cmpx_M 8C
	7, // Bsr 8D
	3, // Ldx_M 8E
	2, // Invalid 8F
	4, // Suba_D 90
	4, // Cmpa_D 91
	4, // Sbca_D 92
	6, // Subd_D 93
	4, // Anda_D 94
	4, // Bita_D 95
	4, // Lda_D 96
	4, // Sta_D 97
	4, // Eora_D 98
	4, // Adca_D 99
	4, // Ora_D 9A
	4, // Adda_D 9B
	6, // Cmpx_D 9C
	7, // Jsr_D 9D
	5, // Ldx_D 9E
	5, // Stx_D 9F
	4, // Suba_X A0
	4, // Cmpa_X A1
	4, // Sbca_X A2
	6, // Subd_X A3
	4, // Anda_X A4
	4, // Bita_X A5
	4, // Lda_X A6
	4, // Sta_X A7
	4, // Eora_X A8
	4, // Adca_X A9
	4, // Ora_X AA
	4, // Adda_X AB
	6, // Cmpx_X AC
	7, // Jsr_X AD
	5, // Ldx_X AE
	5, // Stx_X AF
	5, // Suba_E B0
	5, // Cmpa_E B1
	5, // Sbca_E B2
	6, // Subd_E B3
	5, // Anda_E B4
	5, // Bita_E B5
	5, // Lda_E B6
	5, // Sta_E B7
	5, // Eora_E B8
	5, // Adca_E B9
	5, // Ora_E BA
	5, // Adda_E BB
	7, // Cmpx_E BC
	8, // Jsr_E BD
	6, // Ldx_E BE
	6, // Stx_E BF
	2, // Subb_M C0
	2, // Cmpb_M C1
	2, // Sbcb_M C2
	4, // Addd_M C3
	2, // Andb_M C4
	2, // Bitb_M C5
	2, // Ldb_M C6
	2, // Invalid C7
	2, // Eorb_M C8
	2, // Adcb_M C9
	2, // Orb_M CA
	2, // Addb_M CB
	3, // Ldd_M CC
	5, // Ldq_M CD
	3, // Ldu_M CE
	2, // Invalid CF
	4, // Subb_D D0
	4, // Cmpb_D D1
	4, // Sbcb_D D2
	6, // Addd_D D3
	4, // Andb_D D4
	4, // Bitb_D D5
	4, // Ldb_D D6
	4, // Stb_D D7
	4, // Eorb_D D8
	4, // Adcb_D D9
	4, // Orb_D DA
	4, // Addb_D DB
	5, // Ldd_D DC
	5, // Std_D DD
	5, // Ldu_D DE
	5, // Stu_D DF
	4, // Subb_X E0
	4, // Cmpb_X E1
	4, // Sbcb_X E2
	6, // Addd_X E3
	4, // Andb_X E4
	4, // Bitb_X E5
	4, // Ldb_X E6
	4, // Stb_X E7
	4, // Eorb_X E8
	4, // Adcb_X E9
	4, // Orb_X EA
	4, // Addb_X EB
	5, // Ldd_X EC
	5, // Std_X ED
	5, // Ldu_X EE
	5, // Stu_X EF
	5, // Subb_E F0
	5, // Cmpb_E F1
	5, // Sbcb_E F2
	7, // Addd_E F3
	5, // Andb_E F4
	5, // Bitb_E F5
	5, // Ldb_E F6
	5, // Stb_E F7
	5, // Eorb_E F8
	5, // Adcb_E F9
	5, // Orb_E FA
	5, // Addb_E FB
	6, // Ldd_E FC
	6, // Std_E FD
	6, // Ldu_E FE
	6 // Stu_E FF
};

short instcyclemu1[256] =
{
	2, // Invalid 00
	2, // Invalid 01
	2, // Invalid 02
	2, // Invalid 03
	2, // Invalid 04
	2, // Invalid 05
	2, // Invalid 06
	2, // Invalid 07
	2, // Invalid 08
	2, // Invalid 09
	2, // Invalid 0A
	2, // Invalid 0B
	2, // Invalid 0C
	2, // Invalid 0D
	2, // Invalid 0E
	2, // Invalid 0F
	2, // Invalid 10
	2, // Invalid 11
	2, // Invalid 12
	2, // Invalid 13
	2, // Invalid 14
	2, // Invalid 15
	2, // Invalid 16
	2, // Invalid 17
	2, // Invalid 18
	2, // Invalid 19
	2, // Invalid 1A
	2, // Invalid 1B
	2, // Invalid 1C
	2, // Invalid 1D
	2, // Invalid 1E
	2, // Invalid 1F
	2, // Invalid 20
	5, // Lbrn 21
	5, // Lbhi 22
	5, // Lbls 23
	5, // Lbhs 24
	5, // Lbcs 25
	5, // Lbne 26
	5, // Lbeq 27
	5, // Lbvc 28
	5, // Lbvs 29
	5, // Lbpl 2A
	5, // Lbmi 2B
	5, // Lbge 2C
	5, // Lblt 2D
	5, // Lbgt 2E
	5, // Lble 2F
	4, // Addr 30
	4, // Adcr 31
	4, // Subr 32
	4, // Sbcr 33
	4, // Andr 34
	4, // Orr 35
	4, // Eorr 36
	4, // Cmpr 37
	6, // pshsw 38
	6, // pulsw 39
	6, // pshuw 3A
	6, // puluw 3B
	2, // Invalid 3C
	2, // Invalid 3D
	2, // Invalid 3E
	20, // Swi2 3F
	3, // Negd 40
	2, // Invalid 41
	2, // Invalid 42
	3, // Comd 43
	3, // Lsrd 44
	2, // Invalid 45
	3, // Rord 46
	3, // Asrd 47
	3, // Asld 48
	3, // Rold 49
	3, // Decd 4A
	2, // Invalid 4B
	3, // Incd 4C
	3, // Tstd 4D
	2, // Invalid 4E
	3, // Clrd 4F

};

short instcyclnat0[256] =
{
	5, // Neg_D 00
	6, // Oim_D 01
	6, // Aim_D 02
	5, // Com_D 03
	5, // Lsr_D 04
	6, // Eim_D 05
	5, // Ror_D 06
	5, // Asr_D 07
	5, // Asl_D 08
	5, // Rol_D 09
	5, // Dec_D 0A
	6, // Tim_D 0B
	5, // Inc_D 0C
	4, // Tst_D 0D
	2, // Jmp_D 0E
	5, // Clr_D 0F
	0, // Page2 10
	0, // Page3 11
	1, // Nop 12
	0, // Sync 13
	4, // Sexw 14
	1, // Invalid 15
	4, // Lbra 16
	7, // Lbsr 17
	1, // Invalid 18
	1, // Daa 19
	3, // Orcc 1A
	1, // Invalid 1B
	3, // Andcc 1C
	1, // Sex 1D
	5, // Exg 1E
	4, // Tfr 1F
	3, // Bra 20
	3, // Brn 21
	3, // Bhi 22
	3, // Bls 23
	3, // Bhs 24
	3, // Blo 25
	3, // Bne 26
	3, // Beq 27
	3, // Bvc 28
	3, // Bvs 29
	3, // Bpl 2A
	3, // Bmi 2B
	3, // Bge 2C
	3, // Blt 2D
	3, // Bgt 2E
	3, // Ble 2F
	4, // Leax 30
	4, // Leay 31
	4, // Leas 32
	4, // Leau 33
	4, // Pshs 34
	4, // Puls 35
	4, // Pshu 36
	4, // Puls 37
	1, // Invalid 38
	4, // Rts 39
	1, // Abx 3A
	6, // Rti 3B
	20, // Cwai 3C
	10, // Mul_I 3D
	1, // Invalid 3E
	20, // Swi1 3F
	1, // Nega_I 40
	1, // Invalid 41
	1, // Invalid 42
	1, // Coma_I 43
	1, // Lsra_I 44
	1, // Invalid 45
	1, // Rora_I 46
	1, // Asra_I 47
	1, // Asla_I 48
	1, // Rola_I 49
	1, // Deca_I 4A
	1, // Invalid 4B
	1, // Inca_I 4C
	1, // Tsta_I 4D
	1, // Invalid 4E
	1, // Clra_I 4F
	1, // Negb_I 50
	1, // Invalid 51
	1, // Invalid 52
	1, // Comb_I 53
	1, // Lsrb_I 54
	1, // Invalid 55
	1, // Rorb_I 56
	1, // Asrb_I 57
	1, // Aslb_I 58
	1, // Rolb_I 59
	1, // Decb_I 5A
	1, // Invalid 5B
	1, // Incb_I 5C
	1, // Tstb_I 5D
	1, // Invalid 5E
	1, // Clrb_I 5F
	6, // Neg_X 60
	7, // Oim_X 61
	7, // Aim_X 62
	6, // Com_X 63
	6, // Lsr_X 64
	7, // Eim_X 65
	6, // Ror_X 66
	6, // Asr_X 67
	6, // Asl_X 68
	6, // Rol_X 69
	6, // Dec_X 6A
	7, // Tim_X 6B
	6, // Inc_X 6C
	5, // Tst_X 6D
	3, // Jmp_X 6E
	6, // Clr_X 6F
	6, // Neg_X 70
	7, // Oim_E 71
	7, // Aim_E 72
	6, // Com_E 73
	6, // Lsr_E 74
	7, // Eim_E 75
	6, // Ror_E 76
	6, // Asr_E 77
	6, // Asl_E 78
	6, // Rol_E 79
	6, // Dec_E 7A
	7, // Tim_E 7B
	6, // Inc_E 7C
	5, // Tst_E 7D
	3, // Jmp_E 7E
	6, // Clr_E 7F
	2, // Suba_M 80
	2, // Cmpa_M 81
	2, // Sbca_M 82
	3, // Subd_M 83
	2, // Anda_M 84
	2, // Bita_M 85
	2, // Lda_M 86
	1, // Invalid 87
	2, // Eora_M 88
	2, // Adca_M 89
	2, // Ora_M 8A
	2, // Adda_M 8B
	3, // Cmpx_M 8C
	6, // Bsr 8D
	3, // Ldx_M 8E
	1, // Invalid 8F
	3, // Suba_D 90
	3, // Cmpa_D 91
	3, // Sbca_D 92
	4, // Subd_D 93
	3, // Anda_D 94
	3, // Bita_D 95
	3, // Lda_D 96
	3, // Sta_D 97
	3, // Eora_D 98
	3, // Adca_D 99
	3, // Ora_D 9A
	3, // Adda_D 9B
	4, // Cmpx_D 9C
	6, // Jsr_D 9D
	4, // Ldx_D 9E
	4, // Stx_D 9F
	4, // Suba_X A0
	4, // Cmpa_X A1
	4, // Sbca_X A2
	5, // Subd_X A3
	4, // Anda_X A4
	4, // Bita_X A5
	4, // Lda_X A6
	4, // Sta_X A7
	4, // Eora_X A8
	4, // Adca_X A9
	4, // Ora_X AA
	4, // Adda_X AB
	5, // Cmpx_X AC
	6, // Jsr_X AD
	5, // Ldx_X AE
	5, // Stx_X AF
	4, // Suba_E B0
	4, // Cmpa_E B1
	4, // Sbca_E B2
	5, // Subd_E B3
	4, // Anda_E B4
	4, // Bita_E B5
	4, // Lda_E B6
	4, // Sta_E B7
	4, // Eora_E B8
	4, // Adca_E B9
	4, // Ora_E BA
	4, // Adda_E BB
	5, // Cmpx_E BC
	7, // Jsr_E BD
	5, // Ldx_E BE
	5, // Stx_E BF
	2, // Subb_M C0
	2, // Cmpb_M C1
	2, // Sbcb_M C2
	3, // Addd_M C3
	2, // Andb_M C4
	2, // Bitb_M C5
	2, // Ldb_M C6
	1, // Invalid C7
	2, // Eorb_M C8
	2, // Adcb_M C9
	2, // Orb_M CA
	2, // Addb_M CB
	3, // Ldd_M CC
	5, // Ldq_M CD
	3, // Ldu_M CE
	1, // Invalid CF
	3, // Subb_D D0
	3, // Cmpb_D D1
	3, // Sbcb_D D2
	4, // Addd_D D3
	3, // Andb_D D4
	3, // Bitb_D D5
	3, // Ldb_D D6
	3, // Stb_D D7
	3, // Eorb_D D8
	3, // Adcb_D D9
	3, // Orb_D DA
	3, // Addb_D DB
	4, // Ldd_D DC
	4, // Std_D DD
	4, // Ldu_D DE
	4, // Stu_D DF
	4, // Subb_X E0
	4, // Cmpb_X E1
	4, // Sbcb_X E2
	5, // Addd_X E3
	4, // Andb_X E4
	4, // Bitb_X E5
	4, // Ldb_X E6
	4, // Stb_X E7
	4, // Eorb_X E8
	4, // Adcb_X E9
	4, // Orb_X EA
	4, // Addb_X EB
	5, // Ldd_X EC
	5, // Std_X ED
	5, // Ldu_X EE
	5, // Stu_X EF
	4, // Subb_E F0
	4, // Cmpb_E F1
	4, // Sbcb_E F2
	5, // Addd_E F3
	4, // Andb_E F4
	4, // Bitb_E F5
	4, // Ldb_E F6
	4, // Stb_E F7
	4, // Eorb_E F8
	4, // Adcb_E F9
	4, // Orb_E FA
	4, // Addb_E FB
	5, // Ldd_E FC
	5, // Std_E FD
	5, // Ldu_E FE
	5, // Stu_E FF
};

short instcyclnat1[256] =
{
	1, // Invalid 00
	1, // Invalid 01
	1, // Invalid 02
	1, // Invalid 03
	1, // Invalid 04
	1, // Invalid 05
	1, // Invalid 06
	1, // Invalid 07
	1, // Invalid 08
	1, // Invalid 09
	1, // Invalid 0A
	1, // Invalid 0B
	1, // Invalid 0C
	1, // Invalid 0D
	1, // Invalid 0E
	1, // Invalid 0F
	1, // Invalid 10
	1, // Invalid 11
	1, // Invalid 12
	1, // Invalid 13
	1, // Invalid 14
	1, // Invalid 15
	1, // Invalid 16
	1, // Invalid 17
	1, // Invalid 18
	1, // Invalid 19
	1, // Invalid 1A
	1, // Invalid 1B
	1, // Invalid 1C
	1, // Invalid 1D
	1, // Invalid 1E
	1, // Invalid 1F
	5, // Lbrn 21
	5, // Lbhi 22
	5, // Lbls 23
	5, // Lbhs 24
	5, // Lbcs 25
	5, // Lbne 26
	5, // Lbeq 27
	5, // Lbvc 28
	5, // Lbvs 29
	5, // Lbpl 2A
	5, // Lbmi 2B
	5, // Lbge 2C
	5, // Lblt 2D
	5, // Lbgt 2E
	5, // Lble 2F
	4, // Addr 30
	4, // Adcr 31
	4, // Subr 32
	4, // Sbcr 33
	4, // Andr 34
	4, // Orr 35
	4, // Eorr 36
	4, // Cmpr 37
	6, // pshsw 38
	6, // pulsw 39
	6, // pshuw 3A
	6, // puluw 3B
	2, // Invalid 3C
	2, // Invalid 3D
	2, // Invalid 3E
	22, // Swi2 3F
	2, // Negd 40
	1, // Invalid 41
	1, // Invalid 42
	2, // Comd 43
	2, // Lsrd 44
	1, // Invalid 45
	2, // Rord 46
	2, // Asrd 47
	2, // Asld 48
	2, // Rold 49
	2, // Decd 4A
	1, // Invalid 4B
	2, // Incd 4C
	2, // Tstd 4D
	1, // Invalid 4E
	2, // Clrd 4F
};

void(*JmpVec1_s[256])(void) = {
	Neg_D_A,		// 00
	Oim_D_A,		// 01
	Aim_D_A,		// 02
	Com_D_A,		// 03
	Lsr_D_A,		// 04
	Eim_D_A,		// 05
	Ror_D_A,		// 06
	Asr_D_A,		// 07
	Asl_D_A,		// 08
	Rol_D_A,		// 09
	Dec_D_A,		// 0A
	Tim_D_A,		// 0B
	Inc_D_A,		// 0C
	Tst_D_A,		// 0D
	Jmp_D_A,		// 0E
	Clr_D_A,		// 0F
	Page_2_s,		// 10
	Page_3_s,		// 11
	Nop_I_A,		// 12
	Sync_I_A,		// 13
	Sexw_I_A,		// 14
	InvalidInsHandler_s,	// 15
	Lbra_R_A,		// 16
	Lbsr_R_A,		// 17
	InvalidInsHandler_s,	// 18
	Daa_I_A,		// 19
	Orcc_M_A,		// 1A
	InvalidInsHandler_s,	// 1B
	Andcc_M_A,	// 1C
	Sex_I_A,		// 1D
	Exg_M_A,		// 1E
	Tfr_M_A,		// 1F
	Bra_R_A,		// 20
	Brn_R_A,		// 21
	Bhi_R_A,		// 22
	Bls_R_A,		// 23
	Bhs_R_A,		// 24
	Blo_R_A,		// 25
	Bne_R_A,		// 26
	Beq_R_A,		// 27
	Bvc_R_A,		// 28
	Bvs_R_A,		// 29
	Bpl_R_A,		// 2A
	Bmi_R_A,		// 2B
	Bge_R_A,		// 2C
	Blt_R_A,		// 2D
	Bgt_R_A,		// 2E
	Ble_R_A,		// 2F
	Leax_X_A,		// 30
	Leay_X_A,		// 31
	Leas_X_A,		// 32
	Leau_X_A,		// 33
	Pshs_M_A,		// 34
	Puls_M_A,		// 35
	Pshu_M_A,		// 36
	Pulu_M_A,		// 37
	InvalidInsHandler_s,	// 38
	Rts_I_A,		// 39
	Abx_I_A,		// 3A
	Rti_I_A,		// 3B
	Cwai_I_A,		// 3C
	Mul_I_A,		// 3D
	Reset_s,		// 3E
	Swi1_I_A,		// 3F
	Nega_I_A,		// 40
	InvalidInsHandler_s,  // 41
	InvalidInsHandler_s,	// 42
	Coma_I_A,		// 43
	Lsra_I_A,		// 44
	InvalidInsHandler_s,	// 45
	Rora_I_A,		// 46
	Asra_I_A,		// 47
	Asla_I_A,		// 48
	Rola_I_A,		// 49
	Deca_I_A,		// 4A
	InvalidInsHandler_s,	// 4B
	Inca_I_A,		// 4C
	Tsta_I_A,		// 4D
	InvalidInsHandler_s,	// 4E
	Clra_I_A,		// 4F
	Negb_I_A,		// 50
	InvalidInsHandler_s,	// 51
	InvalidInsHandler_s,	// 52
	Comb_I_A,		// 53
	Lsrb_I_A,		// 54
	InvalidInsHandler_s,	// 55
	Rorb_I_A,		// 56
	Asrb_I_A,		// 57
	Aslb_I_A,		// 58
	Rolb_I_A,		// 59
	Decb_I_A,		// 5A
	InvalidInsHandler_s,	// 5B
	Incb_I_A,		// 5C
	Tstb_I_A,		// 5D
	InvalidInsHandler_s,	// 5E
	Clrb_I_A,		// 5F
	Neg_X_A,		// 60
	Oim_X_A,		// 61
	Aim_X_A,		// 62
	Com_X_A,		// 63
	Lsr_X_A,		// 64
	Eim_X_A,		// 65
	Ror_X_A,		// 66
	Asr_X_A,		// 67
	Asl_X_A,		// 68
	Rol_X_A,		// 69
	Dec_X_A,		// 6A
	Tim_X_A,		// 6B
	Inc_X_A,		// 6C
	Tst_X_A,		// 6D
	Jmp_X_A,		// 6E
	Clr_X_A,		// 6F
	Neg_E_A,		// 70
	Oim_E_A,		// 71
	Aim_E_A,		// 72
	Com_E_A,		// 73
	Lsr_E_A,		// 74
	Eim_E_A,		// 75
	Ror_E_A,		// 76
	Asr_E_A,		// 77
	Asl_E_A,		// 78
	Rol_E_A,		// 79
	Dec_E_A,		// 7A
	Tim_E_A,		// 7B
	Inc_E_A,		// 7C
	Tst_E_A,		// 7D
	Jmp_E_A,		// 7E
	Clr_E_A,		// 7F
	Suba_M_A,		// 80
	Cmpa_M_A,		// 81
	Sbca_M_A,		// 82
	Subd_M_A,		// 83
	Anda_M_A,		// 84
	Bita_M_A,		// 85
	Lda_M_A,		// 86
	InvalidInsHandler_s,	// 87
	Eora_M_A,		// 88
	Adca_M_A,		// 89
	Ora_M_A,		// 8A
	Adda_M_A,		// 8B
	Cmpx_M_A,		// 8C
	Bsr_R_A,		// 8D
	Ldx_M_A,		// 8E
	InvalidInsHandler_s,	// 8F
	Suba_D_A,		// 90
	Cmpa_D_A,		// 91
	Sbca_D_A,		// 92
	Subd_D_A,		// 93
	Anda_D_A,		// 94
	Bita_D_A,		// 95
	Lda_D_A,		// 96
	Sta_D_A,		// 97
	Eora_D_A,		// 98
	Adca_D_A,		// 99
	Ora_D_A,		// 9A
	Adda_D_A,		// 9B
	Cmpx_D_A,		// 9C
	Jsr_D_A,		// 9D
	Ldx_D_A,		// 9E
	Stx_D_A,		// 9F
	Suba_X_A,		// A0
	Cmpa_X_A,		// A1
	Sbca_X_A,		// A2
	Subd_X_A,		// A3
	Anda_X_A,		// A4
	Bita_X_A,		// A5
	Lda_X_A,		// A6
	Sta_X_A,		// A7
	Eora_X_A,		// A8
	Adca_X_A,		// A9
	Ora_X_A,		// AA
	Adda_X_A,		// AB
	Cmpx_X_A,		// AC
	Jsr_X_A,		// AD
	Ldx_X_A,		// AE
	Stx_X_A,		// AF
	Suba_E_A,		// B0
	Cmpa_E_A,		// B1
	Sbca_E_A,		// B2
	Subd_E_A,		// B3
	Anda_E_A,		// B4
	Bita_E_A,		// B5
	Lda_E_A,		// B6
	Sta_E_A,		// B7
	Eora_E_A,		// B8
	Adca_E_A,		// B9
	Ora_E_A,		// BA
	Adda_E_A,		// BB
	Cmpx_E_A,		// BC
	Jsr_E_A,		// BD
	Ldx_E_A,		// BE
	Stx_E_A,		// BF
	Subb_M_A,		// C0
	Cmpb_M_A,		// C1
	Sbcb_M_A,		// C2
	Addd_M_A,		// C3
	Andb_M_A,		// C4
	Bitb_M_A,		// C5
	Ldb_M_A,		// C6
	InvalidInsHandler_s,	// C7
	Eorb_M_A,		// C8
	Adcb_M_A,		// C9
	Orb_M_A,		// CA
	Addb_M_A,		// CB
	Ldd_M_A,		// CC
	Ldq_M_A,		// CD
	Ldu_M_A,		// CE
	InvalidInsHandler_s,	// CF
	Subb_D_A,		// D0
	Cmpb_D_A,		// D1
	Sbcb_D_A,		// D2
	Addd_D_A,		// D3
	Andb_D_A,		// D4
	Bitb_D_A,		// D5
	Ldb_D_A,		// D6
	Stb_D_A,		// D7
	Eorb_D_A,		// D8
	Adcb_D_A,		// D9
	Orb_D_A,		// DA
	Addb_D_A,		// DB
	Ldd_D_A,		// DC
	Std_D_A,		// DD
	Ldu_D_A,		// DE
	Stu_D_A,		// DF
	Subb_X_A,		// E0
	Cmpb_X_A,		// E1
	Sbcb_X_A,		// E2
	Addd_X_A,		// E3
	Andb_X_A,		// E4
	Bitb_X_A,		// E5
	Ldb_X_A,		// E6
	Stb_X_A,		// E7
	Eorb_X_A,		// E8
	Adcb_X_A,		// E9
	Orb_X_A,		// EA
	Addb_X_A,		// EB
	Ldd_X_A,		// EC
	Std_X_A,		// ED
	Ldu_X_A,		// EE
	Stu_X_A,		// EF
	Subb_E_A,		// F0
	Cmpb_E_A,		// F1
	Sbcb_E_A,		// F2
	Addd_E_A,		// F3
	Andb_E_A,		// F4
	Bitb_E_A,		// F5
	Ldb_E_A,		// F6
	Stb_E_A,		// F7
	Eorb_E_A,		// F8
	Adcb_E_A,		// F9
	Orb_E_A,		// FA
	Addb_E_A,		// FB
	Ldd_E_A,		// FC
	Std_E_A,		// FD
	Ldu_E_A,		// FE
	Stu_E_A,		// FF
};

void(*JmpVec2_s[256])(void) = {
	InvalidInsHandler_s,		// 00
	InvalidInsHandler_s,		// 01
	InvalidInsHandler_s,		// 02
	InvalidInsHandler_s,		// 03
	InvalidInsHandler_s,		// 04
	InvalidInsHandler_s,		// 05
	InvalidInsHandler_s,		// 06
	InvalidInsHandler_s,		// 07
	InvalidInsHandler_s,		// 08
	InvalidInsHandler_s,		// 09
	InvalidInsHandler_s,		// 0A
	InvalidInsHandler_s,		// 0B
	InvalidInsHandler_s,		// 0C
	InvalidInsHandler_s,		// 0D
	InvalidInsHandler_s,		// 0E
	InvalidInsHandler_s,		// 0F
	InvalidInsHandler_s,		// 10
	InvalidInsHandler_s,		// 11
	InvalidInsHandler_s,		// 12
	InvalidInsHandler_s,		// 13
	InvalidInsHandler_s,		// 14
	InvalidInsHandler_s,		// 15
	InvalidInsHandler_s,		// 16
	InvalidInsHandler_s,		// 17
	InvalidInsHandler_s,		// 18
	InvalidInsHandler_s,		// 19
	InvalidInsHandler_s,		// 1A
	InvalidInsHandler_s,		// 1B
	InvalidInsHandler_s,		// 1C
	InvalidInsHandler_s,		// 1D
	InvalidInsHandler_s,		// 1E
	InvalidInsHandler_s,		// 1F
	InvalidInsHandler_s,		// 20
	LBrn_R_A,		// 21
	LBhi_R_A,		// 22
	LBls_R_A,		// 23
	LBhs_R_A,		// 24
	LBcs_R_A,		// 25
	LBne_R_A,		// 26
	LBeq_R_A,		// 27
	LBvc_R_A,		// 28
	LBvs_R_A,		// 29
	LBpl_R_A,		// 2A
	LBmi_R_A,		// 2B
	LBge_R_A,		// 2C
	LBlt_R_A,		// 2D
	LBgt_R_A,		// 2E
	LBle_R_A,		// 2F
	Addr_A,		// 30
	Adcr_A,		// 31
	Subr_A,		// 32
	Sbcr_A,		// 33
	Andr_A,		// 34
	Orr_A,		// 35
	Eorr_A,		// 36
	Cmpr_A,		// 37
	Pshsw_A,		// 38
	Pulsw_A,		// 39
	Pshuw_A,		// 3A
	Puluw_A,		// 3B
	InvalidInsHandler_s,		// 3C
	InvalidInsHandler_s,		// 3D
	InvalidInsHandler_s,		// 3E
	Swi2_I_A,		// 3F
	Negd_I_A,		// 40
	InvalidInsHandler_s,		// 41
	InvalidInsHandler_s,		// 42
	Comd_I_A,		// 43
	Lsrd_I_A,		// 44
	InvalidInsHandler_s,		// 45
	Rord_I_A,		// 46
	Asrd_I_A,		// 47
	Asld_I_A,		// 48
	Rold_I_A,		// 49
	Decd_I_A,		// 4A
	InvalidInsHandler_s,		// 4B
	Incd_I_A,		// 4C
	Tstd_I_A,		// 4D
	InvalidInsHandler_s,		// 4E
	Clrd_I_A,		// 4F
	InvalidInsHandler_s,		// 50
	InvalidInsHandler_s,		// 51
	InvalidInsHandler_s,		// 52
	Comw_I_A,		// 53
	Lsrw_I_A,		// 54
	InvalidInsHandler_s,		// 55
	Rorw_I_A,		// 56
	InvalidInsHandler_s,		// 57
	InvalidInsHandler_s,		// 58
	Rolw_I_A,		// 59
	Decw_I_A,		// 5A
	InvalidInsHandler_s,		// 5B
	Incw_I_A,		// 5C
	Tstw_I_A,		// 5D
	InvalidInsHandler_s,		// 5E
	Clrw_I_A,		// 5F
	InvalidInsHandler_s,		// 60
	InvalidInsHandler_s,		// 61
	InvalidInsHandler_s,		// 62
	InvalidInsHandler_s,		// 63
	InvalidInsHandler_s,		// 64
	InvalidInsHandler_s,		// 65
	InvalidInsHandler_s,		// 66
	InvalidInsHandler_s,		// 67
	InvalidInsHandler_s,		// 68
	InvalidInsHandler_s,		// 69
	InvalidInsHandler_s,		// 6A
	InvalidInsHandler_s,		// 6B
	InvalidInsHandler_s,		// 6C
	InvalidInsHandler_s,		// 6D
	InvalidInsHandler_s,		// 6E
	InvalidInsHandler_s,		// 6F
	InvalidInsHandler_s,		// 70
	InvalidInsHandler_s,		// 71
	InvalidInsHandler_s,		// 72
	InvalidInsHandler_s,		// 73
	InvalidInsHandler_s,		// 74
	InvalidInsHandler_s,		// 75
	InvalidInsHandler_s,		// 76
	InvalidInsHandler_s,		// 77
	InvalidInsHandler_s,		// 78
	InvalidInsHandler_s,		// 79
	InvalidInsHandler_s,		// 7A
	InvalidInsHandler_s,		// 7B
	InvalidInsHandler_s,		// 7C
	InvalidInsHandler_s,		// 7D
	InvalidInsHandler_s,		// 7E
	InvalidInsHandler_s,		// 7F
	Subw_M_A,		// 80
	Cmpw_M_A,		// 81
	Sbcd_M_A,		// 82
	Cmpd_M_A,		// 83
	Andd_M_A,		// 84
	Bitd_M_A,		// 85
	Ldw_M_A,		// 86
	InvalidInsHandler_s,		// 87
	Eord_M_A,		// 88
	Adcd_M_A,		// 89
	Ord_M_A,		// 8A
	Addw_M_A,		// 8B
	Cmpy_M_A,		// 8C
	InvalidInsHandler_s,		// 8D
	Ldy_M_A,		// 8E
	InvalidInsHandler_s,		// 8F
	Subw_D_A,		// 90
	Cmpw_D_A,		// 91
	Sbcd_D_A,		// 92
	Cmpd_D_A,		// 93
	Andd_D_A,		// 94
	Bitd_D_A,		// 95
	Ldw_D_A,		// 96
	Stw_D_A,		// 97
	Eord_D_A,		// 98
	Adcd_D_A,		// 99
	Ord_D_A,		// 9A
	Addw_D_A,		// 9B
	Cmpy_D_A,		// 9C
	InvalidInsHandler_s,		// 9D
	Ldy_D_A,		// 9E
	Sty_D_A,		// 9F
	Subw_X_A,		// A0
	Cmpw_X_A,		// A1
	Sbcd_X_A,		// A2
	Cmpd_X_A,		// A3
	Andd_X_A,		// A4
	Bitd_X_A,		// A5
	Ldw_X_A,		// A6
	Stw_X_A,		// A7
	Eord_X_A,		// A8
	Adcd_X_A,		// A9
	Ord_X_A,		// AA
	Addw_X_A,		// AB
	Cmpy_X_A,		// AC
	InvalidInsHandler_s,		// AD
	Ldy_X_A,		// AE
	Sty_X_A,		// AF
	Subw_E_A,		// B0
	Cmpw_E_A,		// B1
	Sbcd_E_A,		// B2
	Cmpd_E_A,		// B3
	Andd_E_A,		// B4
	Bitd_E_A,		// B5
	Ldw_E_A,		// B6
	Stw_E_A,		// B7
	Eord_E_A,		// B8
	Adcd_E_A,		// B9
	Ord_E_A,		// BA
	Addw_E_A,		// BB
	Cmpy_E_A,		// BC
	InvalidInsHandler_s,		// BD
	Ldy_E_A,		// BE
	Sty_E_A,		// BF
	InvalidInsHandler_s,		// C0
	InvalidInsHandler_s,		// C1
	InvalidInsHandler_s,		// C2
	InvalidInsHandler_s,		// C3
	InvalidInsHandler_s,		// C4
	InvalidInsHandler_s,		// C5
	InvalidInsHandler_s,		// C6
	InvalidInsHandler_s,		// C7
	InvalidInsHandler_s,		// C8
	InvalidInsHandler_s,		// C9
	InvalidInsHandler_s,		// CA
	InvalidInsHandler_s,		// CB
	InvalidInsHandler_s,		// CC
	InvalidInsHandler_s,		// CD
	Lds_M_A,		// CE - was Lds_I
	InvalidInsHandler_s,		// CF
	InvalidInsHandler_s,		// D0
	InvalidInsHandler_s,		// D1
	InvalidInsHandler_s,		// D2
	InvalidInsHandler_s,		// D3
	InvalidInsHandler_s,		// D4
	InvalidInsHandler_s,		// D5
	InvalidInsHandler_s,		// D6
	InvalidInsHandler_s,		// D7
	InvalidInsHandler_s,		// D8
	InvalidInsHandler_s,		// D9
	InvalidInsHandler_s,		// DA
	InvalidInsHandler_s,		// DB
	Ldq_D_A,		// DC
	Stq_D_A,		// DD
	Lds_D_A,		// DE
	Sts_D_A,		// DF
	InvalidInsHandler_s,		// E0
	InvalidInsHandler_s,		// E1
	InvalidInsHandler_s,		// E2
	InvalidInsHandler_s,		// E3
	InvalidInsHandler_s,		// E4
	InvalidInsHandler_s,		// E5
	InvalidInsHandler_s,		// E6
	InvalidInsHandler_s,		// E7
	InvalidInsHandler_s,		// E8
	InvalidInsHandler_s,		// E9
	InvalidInsHandler_s,		// EA
	InvalidInsHandler_s,		// EB
	Ldq_X_A,		// EC
	Stq_X_A,		// ED
	Lds_X_A,		// EE
	Sts_X_A,		// EF
	InvalidInsHandler_s,		// F0
	InvalidInsHandler_s,		// F1
	InvalidInsHandler_s,		// F2
	InvalidInsHandler_s,		// F3
	InvalidInsHandler_s,		// F4
	InvalidInsHandler_s,		// F5
	InvalidInsHandler_s,		// F6
	InvalidInsHandler_s,		// F7
	InvalidInsHandler_s,		// F8
	InvalidInsHandler_s,		// F9
	InvalidInsHandler_s,		// FA
	InvalidInsHandler_s,		// FB
	Ldq_E_A,		// FC
	Stq_E_A,		// FD
	Lds_E_A,		// FE
	Sts_E_A,		// FF
};

void(*JmpVec3_s[256])(void) = {
	InvalidInsHandler_s,		// 00
	InvalidInsHandler_s,		// 01
	InvalidInsHandler_s,		// 02
	InvalidInsHandler_s,		// 03
	InvalidInsHandler_s,		// 04
	InvalidInsHandler_s,		// 05
	InvalidInsHandler_s,		// 06
	InvalidInsHandler_s,		// 07
	InvalidInsHandler_s,		// 08
	InvalidInsHandler_s,		// 09
	InvalidInsHandler_s,		// 0A
	InvalidInsHandler_s,		// 0B
	InvalidInsHandler_s,		// 0C
	InvalidInsHandler_s,		// 0D
	InvalidInsHandler_s,		// 0E
	InvalidInsHandler_s,		// 0F
	InvalidInsHandler_s,		// 10
	InvalidInsHandler_s,		// 11
	InvalidInsHandler_s,		// 12
	InvalidInsHandler_s,		// 13
	InvalidInsHandler_s,		// 14
	InvalidInsHandler_s,		// 15
	InvalidInsHandler_s,		// 16
	InvalidInsHandler_s,		// 17
	InvalidInsHandler_s,		// 18
	InvalidInsHandler_s,		// 19
	InvalidInsHandler_s,		// 1A
	InvalidInsHandler_s,		// 1B
	InvalidInsHandler_s,		// 1C
	InvalidInsHandler_s,		// 1D
	InvalidInsHandler_s,		// 1E
	InvalidInsHandler_s,		// 1F
	InvalidInsHandler_s,		// 20
	InvalidInsHandler_s,		// 21
	InvalidInsHandler_s,		// 22
	InvalidInsHandler_s,		// 23
	InvalidInsHandler_s,		// 24
	InvalidInsHandler_s,		// 25
	InvalidInsHandler_s,		// 26
	InvalidInsHandler_s,		// 27
	InvalidInsHandler_s,		// 28
	InvalidInsHandler_s,		// 29
	InvalidInsHandler_s,		// 2A
	InvalidInsHandler_s,		// 2B
	InvalidInsHandler_s,		// 2C
	InvalidInsHandler_s,		// 2D
	InvalidInsHandler_s,		// 2E
	InvalidInsHandler_s,		// 2F
	Band_A,		// 30
	Biand_A,		// 31
	Bor_A,		// 32
	Bior_A,		// 33
	Beor_A,		// 34
	Bieor_A,		// 35
	Ldbt_A,		// 36
	Stbt_A,		// 37
	Tfm1_A,		// 38
	Tfm2_A,		// 39
	Tfm3_A,		// 3A
	Tfm4_A,		// 3B
	Bitmd_M_A,	// 3C
	Ldmd_M_A,		// 3D
	InvalidInsHandler_s,		// 3E
	Swi3_I_A,		// 3F
	InvalidInsHandler_s,		// 40
	InvalidInsHandler_s,		// 41
	InvalidInsHandler_s,		// 42
	Come_I_A,		// 43
	InvalidInsHandler_s,		// 44
	InvalidInsHandler_s,		// 45
	InvalidInsHandler_s,		// 46
	InvalidInsHandler_s,		// 47
	InvalidInsHandler_s,		// 48
	InvalidInsHandler_s,		// 49
	Dece_I_A,		// 4A
	InvalidInsHandler_s,		// 4B
	Ince_I_A,		// 4C
	Tste_I_A,		// 4D
	InvalidInsHandler_s,		// 4E
	Clre_I_A,		// 4F
	InvalidInsHandler_s,		// 50
	InvalidInsHandler_s,		// 51
	InvalidInsHandler_s,		// 52
	Comf_I_A,		// 53
	InvalidInsHandler_s,		// 54
	InvalidInsHandler_s,		// 55
	InvalidInsHandler_s,		// 56
	InvalidInsHandler_s,		// 57
	InvalidInsHandler_s,		// 58
	InvalidInsHandler_s,		// 59
	Decf_I_A,		// 5A
	InvalidInsHandler_s,		// 5B
	Incf_I_A,		// 5C
	Tstf_I_A,		// 5D
	InvalidInsHandler_s,		// 5E
	Clrf_I_A,		// 5F
	InvalidInsHandler_s,		// 60
	InvalidInsHandler_s,		// 61
	InvalidInsHandler_s,		// 62
	InvalidInsHandler_s,		// 63
	InvalidInsHandler_s,		// 64
	InvalidInsHandler_s,		// 65
	InvalidInsHandler_s,		// 66
	InvalidInsHandler_s,		// 67
	InvalidInsHandler_s,		// 68
	InvalidInsHandler_s,		// 69
	InvalidInsHandler_s,		// 6A
	InvalidInsHandler_s,		// 6B
	InvalidInsHandler_s,		// 6C
	InvalidInsHandler_s,		// 6D
	InvalidInsHandler_s,		// 6E
	InvalidInsHandler_s,		// 6F
	InvalidInsHandler_s,		// 70
	InvalidInsHandler_s,		// 71
	InvalidInsHandler_s,		// 72
	InvalidInsHandler_s,		// 73
	InvalidInsHandler_s,		// 74
	InvalidInsHandler_s,		// 75
	InvalidInsHandler_s,		// 76
	InvalidInsHandler_s,		// 77
	InvalidInsHandler_s,		// 78
	InvalidInsHandler_s,		// 79
	InvalidInsHandler_s,		// 7A
	InvalidInsHandler_s,		// 7B
	InvalidInsHandler_s,		// 7C
	InvalidInsHandler_s,		// 7D
	InvalidInsHandler_s,		// 7E
	InvalidInsHandler_s,		// 7F
	Sube_M_A,		// 80
	Cmpe_M_A,		// 81
	InvalidInsHandler_s,		// 82
	Cmpu_M_A,		// 83
	InvalidInsHandler_s,		// 84
	InvalidInsHandler_s,		// 85
	Lde_M_A,		// 86
	InvalidInsHandler_s,		// 87
	InvalidInsHandler_s,		// 88
	InvalidInsHandler_s,		// 89
	InvalidInsHandler_s,		// 8A
	Adde_M_A,		// 8B
	Cmps_M_A,		// 8C
	Divd_M_A,		// 8D
	Divq_M_A,		// 8E
	Muld_M_A,		// 8F
	Sube_D_A,		// 90
	Cmpe_D_A,		// 91
	InvalidInsHandler_s,		// 92
	Cmpu_D_A,		// 93
	InvalidInsHandler_s,		// 94
	InvalidInsHandler_s,		// 95
	Lde_D_A,		// 96
	Ste_D_A,		// 97
	InvalidInsHandler_s,		// 98
	InvalidInsHandler_s,		// 99
	InvalidInsHandler_s,		// 9A
	Adde_D_A,		// 9B
	Cmps_D_A,		// 9C
	Divd_D_A,		// 9D
	Divq_D_A,		// 9E
	Muld_D_A,		// 9F
	Sube_X_A,		// A0
	Cmpe_X_A,		// A1
	InvalidInsHandler_s,		// A2
	Cmpu_X_A,		// A3
	InvalidInsHandler_s,		// A4
	InvalidInsHandler_s,		// A5
	Lde_X_A,		// A6
	Ste_X_A,		// A7
	InvalidInsHandler_s,		// A8
	InvalidInsHandler_s,		// A9
	InvalidInsHandler_s,		// AA
	Adde_X_A,		// AB
	Cmps_X_A,		// AC
	Divd_X_A,		// AD
	Divq_X_A,		// AE
	Muld_X_A,		// AF
	Sube_E_A,		// B0
	Cmpe_E_A,		// B1
	InvalidInsHandler_s,		// B2
	Cmpu_E_A,		// B3
	InvalidInsHandler_s,		// B4
	InvalidInsHandler_s,		// B5
	Lde_E_A,		// B6
	Ste_E_A,		// B7
	InvalidInsHandler_s,		// B8
	InvalidInsHandler_s,		// B9
	InvalidInsHandler_s,		// BA
	Adde_E_A,		// BB
	Cmps_E_A,		// BC
	Divd_E_A,		// BD
	Divq_E_A,		// BE
	Muld_E_A,		// BF
	Subf_M_A,		// C0
	Cmpf_M_A,		// C1
	InvalidInsHandler_s,		// C2
	InvalidInsHandler_s,		// C3
	InvalidInsHandler_s,		// C4
	InvalidInsHandler_s,		// C5
	Ldf_M_A,		// C6
	InvalidInsHandler_s,		// C7
	InvalidInsHandler_s,		// C8
	InvalidInsHandler_s,		// C9
	InvalidInsHandler_s,		// CA
	Addf_M_A,		// CB
	InvalidInsHandler_s,		// CC
	InvalidInsHandler_s,		// CD
	InvalidInsHandler_s,		// CE
	InvalidInsHandler_s,		// CF
	Subf_D_A,		// D0
	Cmpf_D_A,		// D1
	InvalidInsHandler_s,		// D2
	InvalidInsHandler_s,		// D3
	InvalidInsHandler_s,		// D4
	InvalidInsHandler_s,		// D5
	Ldf_D_A,		// D6
	Stf_D_A,		// D7
	InvalidInsHandler_s,		// D8
	InvalidInsHandler_s,		// D9
	InvalidInsHandler_s,		// DA
	Addf_D_A,		// DB
	InvalidInsHandler_s,		// DC
	InvalidInsHandler_s,		// DD
	InvalidInsHandler_s,		// DE
	InvalidInsHandler_s,		// DF
	Subf_X_A,		// E0
	Cmpf_X_A,		// E1
	InvalidInsHandler_s,		// E2
	InvalidInsHandler_s,		// E3
	InvalidInsHandler_s,		// E4
	InvalidInsHandler_s,		// E5
	Ldf_X_A,		// E6
	Stf_X_A,		// E7
	InvalidInsHandler_s,		// E8
	InvalidInsHandler_s,		// E9
	InvalidInsHandler_s,		// EA
	Addf_X_A,		// EB
	InvalidInsHandler_s,		// EC
	InvalidInsHandler_s,		// ED
	InvalidInsHandler_s,		// EE
	InvalidInsHandler_s,		// EF
	Subf_E_A,		// F0
	Cmpf_E_A,		// F1
	InvalidInsHandler_s,		// F2
	InvalidInsHandler_s,		// F3
	InvalidInsHandler_s,		// F4
	InvalidInsHandler_s,		// F5
	Ldf_E_A,		// F6
	Stf_E_A,		// F7
	InvalidInsHandler_s,		// F8
	InvalidInsHandler_s,		// F9
	InvalidInsHandler_s,		// FA
	Addf_E_A,		// FB
	InvalidInsHandler_s,		// FC
	InvalidInsHandler_s,		// FD
	InvalidInsHandler_s,		// FE
	InvalidInsHandler_s,		// FF
};

int HD6309Exec_s(int CycleFor)
{

	//static unsigned char opcode = 0;
	CycleCounter = 0;
	gCycleFor = CycleFor;
	while (CycleCounter < CycleFor) {

		if (PendingInterupts)
		{
			if (PendingInterupts & 4)
				cpu_nmi_s();

			if (PendingInterupts & 2)
				cpu_firq_s();

			if (PendingInterupts & 1)
			{
				if (IRQWaiter == 0)	// This is needed to fix a subtle timming problem
					cpu_irq_s();		// It allows the CPU to see $FF03 bit 7 high before
				else				// The IRQ is asserted.
					IRQWaiter -= 1;
			}
		}

		if (SyncWaiting_s == 1)	//Abort the run nothing happens asyncronously from the CPU
			return(0);

		unsigned char memByte = MemRead8_s(PC_REG++);
		JmpVec1_s[memByte](); // Execute instruction pointed to by PC_REG
		CycleCounter += 5;
	}//End While

	return(CycleFor - CycleCounter);
}

void Page_2_s(void) //10
{
	//JmpVec2[MemRead8_s(PC_REG++)](); // Execute instruction pointed to by PC_REG
}

void Page_3_s(void) //11
{
	//JmpVec3[MemRead8_s(PC_REG++)](); // Execute instruction pointed to by PC_REG
}

void cpu_firq_s(void)
{
	
	if (!cc_s[F])
	{
		InInterupt_s=1; //Flag to indicate FIRQ has been asserted
		switch (MD_FIRQMODE)
		{
		case 0:
			cc_s[E]=0; // Turn E flag off
			MemWrite8_s( pc_s.B.lsb,--S_REG);
			MemWrite8_s( pc_s.B.msb,--S_REG);
			MemWrite8_s(getcc_s(),--S_REG);
			cc_s[I]=1;
			cc_s[F]=1;
			PC_REG=MemRead16_s(VFIRQ);
		break;

		case 1:		//6309
			cc_s[E]=1;
			MemWrite8_s( pc_s.B.lsb,--S_REG);
			MemWrite8_s( pc_s.B.msb,--S_REG);
			MemWrite8_s( u_s.B.lsb,--S_REG);
			MemWrite8_s( u_s.B.msb,--S_REG);
			MemWrite8_s( y_s.B.lsb,--S_REG);
			MemWrite8_s( y_s.B.msb,--S_REG);
			MemWrite8_s( x_s.B.lsb,--S_REG);
			MemWrite8_s( x_s.B.msb,--S_REG);
			MemWrite8_s( dp_s.B.msb,--S_REG);
			if (MD_NATIVE6309)
			{
				MemWrite8_s((F_REG),--S_REG);
				MemWrite8_s((E_REG),--S_REG);
			}
			MemWrite8_s(B_REG,--S_REG);
			MemWrite8_s(A_REG,--S_REG);
			MemWrite8_s(getcc_s(),--S_REG);
			cc_s[I]=1;
			cc_s[F]=1;
			PC_REG=MemRead16_s(VFIRQ);
		break;
		}
	}
	PendingInterupts=PendingInterupts & 253;
	return;
}

void cpu_irq_s(void)
{
	if (InInterupt_s==1) //If FIRQ is running postpone the IRQ
		return;			
	if ((!cc_s[I]) )
	{
		cc_s[E]=1;
		MemWrite8_s( pc_s.B.lsb,--S_REG);
		MemWrite8_s( pc_s.B.msb,--S_REG);
		MemWrite8_s( u_s.B.lsb,--S_REG);
		MemWrite8_s( u_s.B.msb,--S_REG);
		MemWrite8_s( y_s.B.lsb,--S_REG);
		MemWrite8_s( y_s.B.msb,--S_REG);
		MemWrite8_s( x_s.B.lsb,--S_REG);
		MemWrite8_s( x_s.B.msb,--S_REG);
		MemWrite8_s( dp_s.B.msb,--S_REG);
		if (MD_NATIVE6309)
		{
			MemWrite8_s((F_REG),--S_REG);
			MemWrite8_s((E_REG),--S_REG);
		}
		MemWrite8_s(B_REG,--S_REG);
		MemWrite8_s(A_REG,--S_REG);
		MemWrite8_s(getcc_s(),--S_REG);
		PC_REG=MemRead16_s(VIRQ);
		cc_s[I]=1; 
	} //Fi I test
	PendingInterupts=PendingInterupts & 254;
	return;
}

void cpu_nmi_s(void)
{
	cc_s[E]=1;
	MemWrite8_s( pc_s.B.lsb,--S_REG);
	MemWrite8_s( pc_s.B.msb,--S_REG);
	MemWrite8_s( u_s.B.lsb,--S_REG);
	MemWrite8_s( u_s.B.msb,--S_REG);
	MemWrite8_s( y_s.B.lsb,--S_REG);
	MemWrite8_s( y_s.B.msb,--S_REG);
	MemWrite8_s( x_s.B.lsb,--S_REG);
	MemWrite8_s( x_s.B.msb,--S_REG);
	MemWrite8_s( dp_s.B.msb,--S_REG);
	if (MD_NATIVE6309)
	{
		MemWrite8_s((F_REG),--S_REG);
		MemWrite8_s((E_REG),--S_REG);
	}
	MemWrite8_s(B_REG,--S_REG);
	MemWrite8_s(A_REG,--S_REG);
	MemWrite8_s(getcc_s(),--S_REG);
	cc_s[I]=1;
	cc_s[F]=1;
	PC_REG=MemRead16_s(VNMI);
	PendingInterupts=PendingInterupts & 251;
	return;
}

// unsigned short CalculateEA_s(unsigned char postbyte)
// {
// 	static unsigned short int ea = 0;
// 	static signed char byte = 0;
// 	static unsigned char Register;

// 	Register = ((postbyte >> 5) & 3) + 1;
	
// 	if (postbyte & 0x80)
// 	{
// 		switch (postbyte & 0x1F)
// 		{
// 		case 0: // Post-inc by 1
// 			ea = (*xfreg16_s[Register]);
// 			(*xfreg16_s[Register])++;
// 			//CycleCounter += 2;
// 			break;

// 		case 1: // Post-inc by 2
// 			ea = (*xfreg16_s[Register]);
// 			(*xfreg16_s[Register]) += 2;
// 			//CycleCounter += 3;
// 			break;

// 		case 2: // Pre-dec by 1
// 			(*xfreg16_s[Register]) -= 1;
// 			ea = (*xfreg16_s[Register]);
// 			//CycleCounter += 2;
// 			break;

// 		case 3: // Pre-dec by 2
// 			(*xfreg16_s[Register]) -= 2;
// 			ea = (*xfreg16_s[Register]);
// 			//CycleCounter += 3;
// 			break;

// 		case 4: // No offest
// 			ea = (*xfreg16_s[Register]);
// 			break;

// 		case 5: // B.reg offset
// 			ea = (*xfreg16_s[Register]) + ((signed char)q_s.Byte.lswlsb);
// 			//CycleCounter += 1;
// 			break;

// 		case 6: // A.reg offset
// 			ea = (*xfreg16_s[Register]) + ((signed char)q_s.Byte.lswmsb);
// 			//CycleCounter += 1;
// 			break;

// 		case 7: // E.reg offset
// 			ea = (*xfreg16_s[Register]) + ((signed char)q_s.Byte.mswmsb);
// 			//CycleCounter += 1;
// 			break;

// 		case 8: // 8 bit offset
// 			ea = (*xfreg16_s[Register]) + (signed char)MemRead8_s(pc_s.Reg++);
// 			//CycleCounter += 1;
// 			break;

// 		case 9: // 16 bit ofset
// 			ea = (*xfreg16_s[Register]) + IMMADDRESS(pc_s.Reg);
// 			//CycleCounter += 4;
// 			pc_s.Reg += 2;
// 			break;

// 		case 10: // F.reg offset
// 			ea = (*xfreg16_s[Register]) + ((signed char)q_s.Byte.mswlsb);
// 			//CycleCounter += 1;
// 			break;

// 		case 11: // D.reg offset
// 			ea = (*xfreg16_s[Register]) + q_s.Word.lsw; //Changed to unsigned 03/14/2005 NG Was signed
// 			//CycleCounter += 4;
// 			break;

// 		case 12: // 8 bit PC Relative
// 			ea = (signed short)pc_s.Reg + (signed char)MemRead8_s(pc_s.Reg) + 1;
// 			//CycleCounter += 1;
// 			pc_s.Reg++;
// 			break;

// 		case 13: // 16 bit PC Relative
// 			ea = PC_REG + IMMADDRESS(pc_s.Reg) + 2;
// 			//CycleCounter += 5;
// 			pc_s.Reg += 2;
// 			break;

// 		case 14: // W.reg offset
// 			ea = (*xfreg16_s[Register]) + q_s.Word.msw;
// 			//CycleCounter += 4;
// 			break;

// 		case 15: // W.reg
// 			byte = (postbyte >> 5) & 3;
// 			switch (byte)
// 			{
// 			case 0: // No offset from W.reg
// 				ea = q_s.Word.msw;
// 				break;
// 			case 1: // 16 bit offset from W.reg
// 				ea = q_s.Word.msw + IMMADDRESS(pc_s.Reg);
// 				pc_s.Reg += 2;
// 				break;
// 			case 2: // Post-inc by 2 from W.reg
// 				ea = q_s.Word.msw;
// 				q_s.Word.msw += 2;
// 				break;
// 			case 3: // Pre-dec by 2 from W.reg
// 				q_s.Word.msw -= 2;
// 				ea = q_s.Word.msw;
// 				break;
// 			}
// 			break;

// 		case 16: // W.reg
// 			byte = (postbyte >> 5) & 3;
// 			switch (byte)
// 			{
// 			case 0: // Indirect no offset from W.reg
// 				ea = MemRead16_s(q_s.Word.msw);
// 				break;
// 			case 1: // Indirect 16 bit offset from W.reg
// 				ea = MemRead16_s(q_s.Word.msw + IMMADDRESS(pc_s.Reg));
// 				pc_s.Reg += 2;
// 				break;
// 			case 2: // Indirect post-inc by 2 from W.reg
// 				ea = MemRead16_s(q_s.Word.msw);
// 				q_s.Word.msw += 2;
// 				break;
// 			case 3: // Indirect pre-dec by 2 from W.reg
// 				q_s.Word.msw -= 2;
// 				ea = MemRead16_s(q_s.Word.msw);
// 				break;
// 			}
// 			break;


// 		case 17: // Indirect Post-inc by 2
// 			ea = (*xfreg16_s[Register]);
// 			(*xfreg16_s[Register]) += 2;
// 			ea = MemRead16_s(ea);
// 			//CycleCounter += 6;
// 			break;

// 		case 18: //10010
// 			//CycleCounter += 6;
// 			break;

// 		case 19: // Indirect Pre-dec by 2
// 			(*xfreg16_s[Register]) -= 2;
// 			ea = (*xfreg16_s[Register]);
// 			ea = MemRead16_s(ea);
// 			//CycleCounter += 6;
// 			break;

// 		case 20: // Indirect no offset
// 			ea = (*xfreg16_s[Register]);
// 			ea = MemRead16_s(ea);
// 			//CycleCounter += 3;
// 			break;

// 		case 21: // Indirect B.reg offset
// 			ea = (*xfreg16_s[Register]) + ((signed char)q_s.Byte.lswlsb);
// 			ea = MemRead16_s(ea);
// 			//CycleCounter += 4;
// 			break;

// 		case 22: // Indirect A.reg offset
// 			ea = (*xfreg16_s[Register]) + ((signed char)q_s.Byte.lswmsb);
// 			ea = MemRead16_s(ea);
// 			//CycleCounter += 4;
// 			break;

// 		case 23: // Indirect E.reg offset
// 			ea = (*xfreg16_s[Register]) + ((signed char)q_s.Byte.mswmsb);
// 			ea = MemRead16_s(ea);
// 			//CycleCounter += 4;
// 			break;

// 		case 24: // Indirect 8 bit offset
// 			ea = (*xfreg16_s[Register]) + (signed char)MemRead8_s(pc_s.Reg++);
// 			ea = MemRead16_s(ea);
// 			//CycleCounter += 4;
// 			break;

// 		case 25: // Indirect 16 bit offset
// 			ea = (*xfreg16_s[Register]) + IMMADDRESS(pc_s.Reg);
// 			ea = MemRead16_s(ea);
// 			//CycleCounter += 7;
// 			pc_s.Reg += 2;
// 			break;
// 		case 26: // Indirect F.reg offset
// 			ea = (*xfreg16_s[Register]) + ((signed char)q_s.Byte.mswlsb);
// 			ea = MemRead16_s(ea);
// 			//CycleCounter += 4;
// 			break;

// 		case 27: // Indirect D.reg offset
// 			ea = (*xfreg16_s[Register]) + q_s.Word.lsw;
// 			ea = MemRead16_s(ea);
// 			//CycleCounter += 7;
// 			break;

// 		case 28: // Indirect 8 bit PC relative
// 			ea = (signed short)pc_s.Reg + (signed char)MemRead8_s(pc_s.Reg) + 1;
// 			ea = MemRead16_s(ea);
// 			//CycleCounter += 4;
// 			pc_s.Reg++;
// 			break;

// 		case 29: // Indirect 16 bit PC relative
// 			ea = pc_s.Reg + IMMADDRESS(pc_s.Reg) + 2;
// 			ea = MemRead16_s(ea);
// 			//CycleCounter += 8;
// 			pc_s.Reg += 2;
// 			break;

// 		case 30: // Indirect W.reg offset
// 			ea = (*xfreg16_s[Register]) + q_s.Word.msw;
// 			ea = MemRead16_s(ea);
// 			//CycleCounter += 7;
// 			break;

// 		case 31: // Indirect extended
// 			ea = IMMADDRESS(pc_s.Reg);
// 			ea = MemRead16_s(ea);
// 			//CycleCounter += 8;
// 			pc_s.Reg += 2;
// 			break;

// 		} //END Switch
// 	}
// 	else // 5 bit offset
// 	{
// 		byte = (postbyte & 31);
// 		byte = (byte << 3);
// 		byte = byte / 8;
// 		ea = *xfreg16_s[Register] + byte; //Was signed
// 		//CycleCounter += 1;
// 	}
// 	return(ea);
// }

//void setcc_s(UINT8 bincc)
//{
//	cc_s[C] = !!(bincc & CFx68);
//	cc_s[V] = !!(bincc & VFx68);
//	cc_s[Z] = !!(bincc & ZFx68);
//	cc_s[N] = !!(bincc & NFx68);
//	cc_s[I] = !!(bincc & IFx68);
//	cc_s[H] = !!(bincc & HFx68);
//	cc_s[F] = !!(bincc & FFx68);
//	cc_s[E] = !!(bincc & EFx68);
//	return;
//}

//UINT8 getcc_s(void)
//{
//	UINT8 bincc = 0;
//	bincc |= cc_s[C] == 1 ? CFx68 : 0;
//	bincc |= cc_s[V] == 1 ? VFx68 : 0;
//	bincc |= cc_s[Z] == 1 ? ZFx68 : 0;
//	bincc |= cc_s[N] == 1 ? NFx68 : 0;
//	bincc |= cc_s[I] == 1 ? IFx68 : 0;
//	bincc |= cc_s[H] == 1 ? HFx68 : 0;
//	bincc |= cc_s[F] == 1 ? FFx68 : 0;
//	bincc |= cc_s[E] == 1 ? EFx68 : 0;
//	return bincc;
//}

//void setcc_s(UINT8 bincc)
//{
//	x86Flags = 0;
//	x86Flags |= (bincc & CFx68) == CFx68 ? CFx86 : 0;
//	x86Flags |= (bincc & VFx68) == VFx68 ? VFx86 : 0;
//	x86Flags |= (bincc & ZFx68) == ZFx68 ? ZFx86 : 0;
//	x86Flags |= (bincc & NFx68) == NFx68 ? NFx86 : 0;
//	x86Flags |= (bincc & HFx68) == HFx68 ? HFx86 : 0;
//	cc_s[I] = !!(bincc & (IFx68));
//	cc_s[F] = !!(bincc & (FFx68));
//	cc_s[E] = !!(bincc & (EFx68));
//	return;
//}
//
//UINT8 getcc_s(void)
//{
//	UINT8 bincc = 0;
//	bincc |= (x86Flags & CFx86) == CFx86 ? CFx68 : 0;
//	bincc |= (x86Flags & VFx86) == VFx86 ? VFx68 : 0;
//	bincc |= (x86Flags & ZFx86) == ZFx86 ? ZFx68 : 0;
//	bincc |= (x86Flags & NFx86) == NFx86 ? NFx68 : 0;
//	bincc |= (x86Flags & HFx86) == HFx86 ? HFx68 : 0;
//	bincc |= cc_s[I] == 1 ? IFx68 : 0;
//	bincc |= cc_s[F] == 1 ? FFx68 : 0;
//	bincc |= cc_s[E] == 1 ? EFx68 : 0;
//	return bincc;
//}

void setmd_s (UINT8 binmd)
{
	//unsigned char bit;
	//for (bit=0;bit<=7;bit++)
	//	md_s[bit]=!!(binmd & (1<<bit));
	mdbits_s = binmd & 3;
	return;
}

UINT8 getmd_s(void)
{
	//unsigned char binmd=0,bit=0;
	//for (bit=0;bit<=7;bit++)
	//	if (md_s[bit])
	//		binmd=binmd | (1<<bit);
	//	return(binmd);
	return mdbits_s & 0xc0;
}
	
void HD6309AssertInterupt_s(unsigned char Interupt,unsigned char waiter)// 4 nmi 2 firq 1 irq
{
	SyncWaiting_s=0;
	PendingInterupts=PendingInterupts | (1<<(Interupt-1));
	IRQWaiter=waiter;
	return;
}

void HD6309DeAssertInterupt_s(unsigned char Interupt)// 4 nmi 2 firq 1 irq
{
	PendingInterupts=PendingInterupts & ~(1<<(Interupt-1));
	InInterupt_s=0;
	return;
}

MSABI void InvalidInsHandler_s(void)
{	
	mdbits_s |= MD_ILLEGALINST_BIT;
	// mdbits_s=getmd_s();
	ErrorVector_s();
	return;
}

void IgnoreInsHandler(void)
{
	return;
}

void MSABI DivbyZero_s(void)
{
	mdbits_s |= MD_DIVBYZERO_BIT;
	// mdbits_s=getmd_s();
	ErrorVector_s();
	return;
}

void MSABI ErrorVector_s(void)
{
	cc_s[E]=1;
	MemWrite8_s( pc_s.B.lsb,--S_REG);
	MemWrite8_s( pc_s.B.msb,--S_REG);
	MemWrite8_s( u_s.B.lsb,--S_REG);
	MemWrite8_s( u_s.B.msb,--S_REG);
	MemWrite8_s( y_s.B.lsb,--S_REG);
	MemWrite8_s( y_s.B.msb,--S_REG);
	MemWrite8_s( x_s.B.lsb,--S_REG);
	MemWrite8_s( x_s.B.msb,--S_REG);
	MemWrite8_s( dp_s.B.msb,--S_REG);
	if (MD_NATIVE6309)
	{
		MemWrite8_s((F_REG),--S_REG);
		MemWrite8_s((E_REG),--S_REG);
		CycleCounter+=2;
	}
	MemWrite8_s(B_REG,--S_REG);
	MemWrite8_s(A_REG,--S_REG);
	MemWrite8_s(getcc_s(),--S_REG);
	PC_REG=MemRead16_s(VTRAP);
	CycleCounter+=(12 + InsCycles[MD_NATIVE6309][M54]);	//One for each byte +overhead? Guessing from PSHS
	return;
}

unsigned int MSABI MemRead32_s(unsigned short Address)
{
	return ( (MemRead16_s(Address)<<16) | MemRead16(Address+2) );

}
void MSABI MemWrite32_s(unsigned int data,unsigned short Address)
{
	MemWrite16_s( data>>16,Address);
	MemWrite16_s( data & 0xFFFF,Address+2);
	return;
}

unsigned char GetSorceReg_s(unsigned char Tmp)
{
	unsigned char Source=(Tmp>>4);
	unsigned char Dest= Tmp & 15;
	unsigned char Translate[]={0,0};
	if ( (Source & 8) == (Dest & 8) ) //like size registers
		return(Source );
return(0);
}

void HD6309ForcePC_s(unsigned short NewPC)
{
	PC_REG=NewPC;
	PendingInterupts=0;
	SyncWaiting_s=0;
	return;
}

unsigned short GetPC_s(void)
{
	return(PC_REG);
}

