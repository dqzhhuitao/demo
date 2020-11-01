	//--------------------------------------------------------------------
// Wuhan Hiconics Drive Technology Co.,Ltd
// CopyRight 	2010-2013
// Funtion:		pwm and deadband compensation
// Author:		
// Date:		2013-08-22
//--------------------------------------------------------------------

#include "..\common\F28035.h"
#include "..\common\ctrl_gVarsDeclaration.h"			//Global parameters include file
#include "..\common\func_gVarsDeclaration.h"
#include "..\common\func_KeyLed.h"

struct PWM_MODU_STRUCT	PWMA, PWMB, PWMC;
volatile struct ePWM_CMPR_FLAG	CMPR_FLAG;
volatile struct LAST_CMPR_REG	LAST_CMPR;

S16 s16_an=0;
S16 s16_bn=0;
S16 s16_cn=0;
U16 u_16cmpr_1=0;
U16 u_16cmpr_2=0;
U16 u_16cmpr_3=0;
S16 s16_a1=0;
S16 s16_a2=0;
S16 s16_a3=0;
S16 s16_b1=0;
S16 s16_b2=0;
S16 s16_b3=0;
S16 s16_c1=0;
S16 s16_c2=0;
S16 s16_c3=0;


U16 OverModuFlag = 0;
U16 VolSectorIn60D = 0;
U16 HoldAngle = 0;
U16 VolAngel = 0;

U16 u16_m_vect_vf = 0;

void OverModuDeal(void);

void SUB_DISPWM(void)
{
	EPwm1Regs.AQCSFRC.all = 0x0009;
	EPwm2Regs.AQCSFRC.all = 0x0009;
	EPwm3Regs.AQCSFRC.all = 0x0009;
}

void SUB_ENPWM(void)
{
	EPwm1Regs.AQCSFRC.all = 0x0000;
	EPwm2Regs.AQCSFRC.all = 0x0000;
	EPwm3Regs.AQCSFRC.all = 0x0000;
}


void InitEPwm1(void)
{
	// EPWM Module 1 config for inverter
	// Setup TB
	EPwm1Regs.TBPRD = DEFAULT_PRD;					// Set timer period
	EPwm1Regs.TBPHS.half.TBPHS = 0; 				// Set Phase register to zero
	EPwm1Regs.TBCTL.bit.CTRMODE = TB_COUNT_UPDOWN;	// Symmetrical mode
	EPwm1Regs.TBCTL.bit.PHSEN = TB_DISABLE; 		// Master module
	EPwm1Regs.TBCTL.bit.PRDLD = TB_SHADOW;
	EPwm1Regs.TBCTL.bit.HSPCLKDIV = TB_DIV2;
	EPwm1Regs.TBCTL.bit.SYNCOSEL = TB_CTR_ZERO; 	// Sync down-stream module

	EPwm1Regs.CMPCTL.bit.SHDWAMODE = CC_SHADOW;
	EPwm1Regs.CMPCTL.bit.SHDWBMODE = CC_SHADOW;
	EPwm1Regs.CMPCTL.bit.LOADAMODE = CC_CTR_ZERO_PRD;	// load on CTR=Zero or PRD
	EPwm1Regs.CMPCTL.bit.LOADBMODE = CC_CTR_ZERO_PRD;

	// Setup compare 
	EPwm1Regs.CMPA.half.CMPA = DEFAULT_PRD>>1;

	// set AQ
	EPwm1Regs.AQCTLA.bit.CAU = AQ_SET; 
	EPwm1Regs.AQCTLA.bit.CAD = AQ_CLEAR;
	EPwm1Regs.AQCTLB.bit.CAU = AQ_SET; 
	EPwm1Regs.AQCTLB.bit.CAD = AQ_CLEAR; 
	
	
	// Active Low PWMs - Setup Deadband
	EPwm1Regs.DBCTL.bit.OUT_MODE = DB_FULL_ENABLE;
	EPwm1Regs.DBCTL.bit.POLSEL = DB_ACTV_LOC;		//EPWMxA is inverted
	EPwm1Regs.DBCTL.bit.IN_MODE = DBA_RED_DBB_FED;
	EPwm1Regs.DBRED = DEFAULT_DEADBAND_TIME;
	EPwm1Regs.DBFED = DEFAULT_DEADBAND_TIME;
   
	//TZ
	EALLOW;
	EPwm1Regs.TZSEL.bit.OSHT1 = TZ_ENABLE;
	EPwm1Regs.TZSEL.bit.OSHT3 = TZ_ENABLE;
//	EPwm1Regs.TZSEL.bit.CBC3 = TZ_ENABLE;			//select TZ1,2,3  as INTERRUPT source TO disable EPWMxA,EPWMxB
	EPwm1Regs.TZSEL.bit.OSHT2 = TZ_DISABLE;			//select TZ2 as INTERRUPT source TO disable EPWMxA,EPWMxB
	EPwm1Regs.TZCTL.bit.TZA = TZ_FORCE_HI;
	EPwm1Regs.TZCTL.bit.TZB = TZ_FORCE_HI;
	EPwm1Regs.TZEINT.bit.CBC = 0;					//ENABLE TZ1 INTERRUPT
	EPwm1Regs.TZEINT.bit.OST = 1;
	
	//EPwm1Regs.TZFLG
	//EPwm1Regs.TZCLR
	//EPwm1Regs.TZFRC
	EDIS;
	
	// Interrupt where we will change the Deadband
	EPwm1Regs.ETSEL.bit.INTSEL = ET_CTR_ZERO;	  // Select INT on Zero event
	EPwm1Regs.ETSEL.bit.INTEN = 1;				  // Enable INT
	EPwm1Regs.ETPS.bit.INTPRD = ET_1ST; 		  // Generate INT on 1st event
	
	//EPwm1Regs.ETFLG
	EPwm1Regs.ETCLR.bit.INT = 1;
	//EPwm1Regs.ETFRC
}


void InitEPwm2(void)
{
	// EPWM Module 2 config for inverter
	// Setup TB
	EPwm2Regs.TBPRD = DEFAULT_PRD;					// Set timer period
	EPwm2Regs.TBPHS.half.TBPHS = 0; 				// Set Phase register to zero
	EPwm2Regs.TBCTL.bit.CTRMODE = TB_COUNT_UPDOWN;	// Symmetrical mode
	EPwm2Regs.TBCTL.bit.PHSEN = TB_DISABLE; 		// Master module
	EPwm2Regs.TBCTL.bit.PRDLD = TB_SHADOW;
	EPwm2Regs.TBCTL.bit.HSPCLKDIV = TB_DIV2;
	EPwm2Regs.TBCTL.bit.SYNCOSEL = TB_SYNC_IN;	// Sync flow-through

	EPwm2Regs.CMPCTL.bit.SHDWAMODE = CC_SHADOW;
	EPwm2Regs.CMPCTL.bit.SHDWBMODE = CC_SHADOW;
	EPwm2Regs.CMPCTL.bit.LOADAMODE = CC_CTR_ZERO_PRD;	// load on CTR=Zero or PRD
	EPwm2Regs.CMPCTL.bit.LOADBMODE = CC_CTR_ZERO_PRD;

	// Setup compare 
	EPwm2Regs.CMPA.half.CMPA = DEFAULT_PRD>>1;
	
	// set AQ
	EPwm2Regs.AQCTLA.bit.CAU = AQ_SET; 
	EPwm2Regs.AQCTLA.bit.CAD = AQ_CLEAR;
	EPwm2Regs.AQCTLB.bit.CAU = AQ_SET; 
	EPwm2Regs.AQCTLB.bit.CAD = AQ_CLEAR; 
	
	// Active Low PWMs - Setup Deadband
	EPwm2Regs.DBCTL.bit.OUT_MODE = DB_FULL_ENABLE;
	EPwm2Regs.DBCTL.bit.POLSEL = DB_ACTV_LOC;		//EPWMxA is inverted
	EPwm2Regs.DBCTL.bit.IN_MODE = DBA_RED_DBB_FED;
	EPwm2Regs.DBRED = DEFAULT_DEADBAND_TIME;
	EPwm2Regs.DBFED = DEFAULT_DEADBAND_TIME;
   
	//TZ
	EALLOW;
	EPwm2Regs.TZSEL.bit.OSHT1 = TZ_ENABLE;
	EPwm2Regs.TZSEL.bit.OSHT3 = TZ_ENABLE;
//	EPwm2Regs.TZSEL.bit.CBC3 = TZ_ENABLE;
	EPwm2Regs.TZSEL.bit.OSHT2 = TZ_DISABLE;			//select TZ2 as INTERRUPT source TO disable EPWMxA,EPWMxB
	EPwm2Regs.TZCTL.bit.TZA = TZ_FORCE_HI;
	EPwm2Regs.TZCTL.bit.TZB = TZ_FORCE_HI;
	EPwm2Regs.TZEINT.bit.CBC = 0;					//ENABLE TZ2 INTERRUPT
	
	//EPwm2Regs.TZFLG
	//EPwm2Regs.TZCLR
	//EPwm2Regs.TZFRC
	EDIS;
	
	// Interrupt where we will change the Deadband
	EPwm2Regs.ETSEL.bit.INTSEL = ET_CTR_PRD;	 // Select INT on Zero event
	EPwm2Regs.ETSEL.bit.INTEN = 0;				 // Disable INT
	EPwm2Regs.ETPS.bit.INTPRD = ET_1ST; 		 // Generate INT on 1st event
	
	//EPwm2Regs.ETFLG
	EPwm2Regs.ETCLR.bit.INT = 1;
	//EPwm2Regs.ETFRC
}



void InitEPwm3(void)
{
	// EPWM Module 3 config for inverter
	// Setup TB
	EPwm3Regs.TBPRD = DEFAULT_PRD;					// Set timer period
	EPwm3Regs.TBPHS.half.TBPHS = 0; 				// Set Phase register to zero
	EPwm3Regs.TBCTL.bit.CTRMODE = TB_COUNT_UPDOWN;	// Symmetrical mode
	EPwm3Regs.TBCTL.bit.PHSEN = TB_DISABLE; 		// Master module
	EPwm3Regs.TBCTL.bit.PRDLD = TB_SHADOW; 
	EPwm3Regs.TBCTL.bit.HSPCLKDIV = TB_DIV2;
	EPwm3Regs.TBCTL.bit.SYNCOSEL = TB_SYNC_IN;	// Sync flow-through

	EPwm3Regs.CMPCTL.bit.SHDWAMODE = CC_SHADOW;
	EPwm3Regs.CMPCTL.bit.SHDWBMODE = CC_SHADOW;
	EPwm3Regs.CMPCTL.bit.LOADAMODE = CC_CTR_ZERO_PRD;	// load on CTR=Zero or PRD
	EPwm3Regs.CMPCTL.bit.LOADBMODE = CC_CTR_ZERO_PRD;

	// Setup compare 
	EPwm3Regs.CMPA.half.CMPA = DEFAULT_PRD>>1;
	
	// set AQ
	EPwm3Regs.AQCTLA.bit.CAU = AQ_SET; 
	EPwm3Regs.AQCTLA.bit.CAD = AQ_CLEAR;
	EPwm3Regs.AQCTLB.bit.CAU = AQ_SET;
	EPwm3Regs.AQCTLB.bit.CAD = AQ_CLEAR;

	// Active Low PWMs - Setup Deadband
	EPwm3Regs.DBCTL.bit.OUT_MODE = DB_FULL_ENABLE;
	EPwm3Regs.DBCTL.bit.POLSEL = DB_ACTV_LOC;		//EPWMxA is inverted
	EPwm3Regs.DBCTL.bit.IN_MODE = DBA_RED_DBB_FED;
	EPwm3Regs.DBRED = DEFAULT_DEADBAND_TIME;
	EPwm3Regs.DBFED = DEFAULT_DEADBAND_TIME;
   
	//TZ
	EALLOW;
	EPwm3Regs.TZSEL.bit.OSHT1 = TZ_ENABLE;
	EPwm3Regs.TZSEL.bit.OSHT3 = TZ_ENABLE;
//	EPwm3Regs.TZSEL.bit.CBC3=TZ_ENABLE;
	EPwm3Regs.TZSEL.bit.OSHT2 = TZ_DISABLE;			//select TZ2 as INTERRUPT source TO disable EPWMxA,EPWMxB
	EPwm3Regs.TZCTL.bit.TZA=TZ_FORCE_HI;
	EPwm3Regs.TZCTL.bit.TZB=TZ_FORCE_HI;
	EPwm3Regs.TZEINT.bit.CBC = 0;					//ENABLE TZ2 INTERRUPT
	
	//EPwm3Regs.TZFLG
	//EPwm3Regs.TZCLR
	//EPwm3Regs.TZFRC
	EDIS;
	
	// Interrupt where we will change the Deadband
//	EPwm3Regs.ETSEL.bit.INTSEL = ET_CTR_ZERO;	  // Select INT on Zero event
//	EPwm3Regs.ETSEL.bit.INTEN = 1;				  // Enable INT
//	EPwm3Regs.ETPS.bit.INTPRD = ET_1ST; 		  // Generate INT on 1st event
	
	//EPwm3Regs.ETFLG
	//EPwm3Regs.ETCLR
	//EPwm3Regs.ETFRC
}



//===========================================================================
// 0%和100%的占空比补偿程序, 计算下一个周期的设置
//===========================================================================
//#pragma CODE_SECTION(ePWM_DUTY_COMPEN, "ramfuncs");
void ePWM_DUTY_COMPEN(void)
{
		if((LAST_CMPR.CMPR1 != 0) && (EPwm1Regs.CMPA.half.CMPA == 0))
		{
			CMPR_FLAG.NONZERO_TO_ZERO1 = 1;
			CMPR_FLAG.ZERO_TO_NONZERO1 = 0;

			EPwm1Regs.AQCTLA.bit.ZRO = AQ_SET;
			EPwm1Regs.AQCTLB.bit.ZRO = AQ_SET;
		}
		else if((LAST_CMPR.CMPR1 == 0) && (EPwm1Regs.CMPA.half.CMPA != 0))
		{
			CMPR_FLAG.ZERO_TO_NONZERO1 = 1;
			CMPR_FLAG.NONZERO_TO_ZERO1 = 0;

			EPwm1Regs.AQCTLA.bit.ZRO = AQ_CLEAR;
			EPwm1Regs.AQCTLB.bit.ZRO = AQ_CLEAR;
			EPwm1Regs.AQCTLA.bit.CAD = AQ_NO_ACTION;
			EPwm1Regs.AQCTLB.bit.CAD = AQ_NO_ACTION;
			EPwm1Regs.CMPCTL.bit.LOADAMODE = CC_CTR_ZERO_PRD;	// load on CTR=Zero or PRD
		}
		else
		{
			CMPR_FLAG.ZERO_TO_NONZERO1 = 0;
			CMPR_FLAG.NONZERO_TO_ZERO1 = 0;
		}


		if((LAST_CMPR.CMPR2 != 0) && (EPwm2Regs.CMPA.half.CMPA == 0))
		{
			CMPR_FLAG.NONZERO_TO_ZERO2 = 1;
			CMPR_FLAG.ZERO_TO_NONZERO2 = 0;

			EPwm2Regs.AQCTLA.bit.ZRO = AQ_SET;
			EPwm2Regs.AQCTLB.bit.ZRO = AQ_SET;
		}
		else if((LAST_CMPR.CMPR2 == 0) && (EPwm2Regs.CMPA.half.CMPA != 0))
		{
			CMPR_FLAG.ZERO_TO_NONZERO2 = 1;
			CMPR_FLAG.NONZERO_TO_ZERO2 = 0;

			EPwm2Regs.AQCTLA.bit.ZRO = AQ_CLEAR;
			EPwm2Regs.AQCTLB.bit.ZRO = AQ_CLEAR;
			EPwm2Regs.AQCTLA.bit.CAD = AQ_NO_ACTION;
			EPwm2Regs.AQCTLB.bit.CAD = AQ_NO_ACTION;
			EPwm2Regs.CMPCTL.bit.LOADAMODE = CC_CTR_ZERO_PRD;	// load on CTR=Zero or PRD
		}
		else
		{
			CMPR_FLAG.ZERO_TO_NONZERO2 = 0;
			CMPR_FLAG.NONZERO_TO_ZERO2 = 0;
		}

		if((LAST_CMPR.CMPR3 != 0) && (EPwm3Regs.CMPA.half.CMPA == 0))
		{
			CMPR_FLAG.NONZERO_TO_ZERO3 = 1;
			CMPR_FLAG.ZERO_TO_NONZERO3 = 0;

			EPwm3Regs.AQCTLA.bit.ZRO = AQ_SET;
			EPwm3Regs.AQCTLB.bit.ZRO = AQ_SET;
		}
		else if((LAST_CMPR.CMPR3 == 0) && (EPwm3Regs.CMPA.half.CMPA != 0))
		{
			CMPR_FLAG.ZERO_TO_NONZERO3 = 1;
			CMPR_FLAG.NONZERO_TO_ZERO3 = 0;

			EPwm3Regs.AQCTLA.bit.ZRO = AQ_CLEAR;
			EPwm3Regs.AQCTLB.bit.ZRO = AQ_CLEAR;
			EPwm3Regs.AQCTLA.bit.CAD = AQ_NO_ACTION;
			EPwm3Regs.AQCTLB.bit.CAD = AQ_NO_ACTION;
			EPwm3Regs.CMPCTL.bit.LOADAMODE = CC_CTR_ZERO_PRD;	// load on CTR=Zero or PRD
		}
		else
		{
			CMPR_FLAG.ZERO_TO_NONZERO3 = 0;
			CMPR_FLAG.NONZERO_TO_ZERO3 = 0;
		}
}

//===========================================================================


//===========================================================================
// 0%和100%的占空比补偿预备程序, 该程序的输出直接影响本次周期的输出
//===========================================================================
#pragma CODE_SECTION(ePWM_DUTY_COMPEN_PREPARE, "ramfuncs");
void ePWM_DUTY_COMPEN_PREPARE(void)
{
	LAST_CMPR.CMPR1 = EPwm1Regs.CMPA.half.CMPA;
	LAST_CMPR.CMPR2 = EPwm2Regs.CMPA.half.CMPA;
	LAST_CMPR.CMPR3 = EPwm3Regs.CMPA.half.CMPA;

	if((CMPR_FLAG.NONZERO_TO_ZERO1 == 1) || (CMPR_FLAG.ZERO_TO_NONZERO1 == 1))
	{
		EPwm1Regs.CMPCTL.bit.LOADAMODE = CC_CTR_ZERO;	// load on CTR=Zero
		EPwm1Regs.AQCTLA.bit.ZRO = AQ_NO_ACTION;
		EPwm1Regs.AQCTLB.bit.ZRO = AQ_NO_ACTION;
		EPwm1Regs.AQCTLA.bit.CAD = AQ_CLEAR;
		EPwm1Regs.AQCTLB.bit.CAD = AQ_CLEAR;
	}
	if((CMPR_FLAG.NONZERO_TO_ZERO2 == 1) || (CMPR_FLAG.ZERO_TO_NONZERO2 == 1))
	{
		EPwm2Regs.CMPCTL.bit.LOADAMODE = CC_CTR_ZERO;	// load on CTR=Zero
		EPwm2Regs.AQCTLA.bit.ZRO = AQ_NO_ACTION;
		EPwm2Regs.AQCTLB.bit.ZRO = AQ_NO_ACTION;
		EPwm2Regs.AQCTLA.bit.CAD = AQ_CLEAR;
		EPwm2Regs.AQCTLB.bit.CAD = AQ_CLEAR;
	}
	if((CMPR_FLAG.NONZERO_TO_ZERO3 == 1) || (CMPR_FLAG.ZERO_TO_NONZERO3 == 1))
	{
		EPwm3Regs.CMPCTL.bit.LOADAMODE = CC_CTR_ZERO;	// load on CTR=Zero
		EPwm3Regs.AQCTLA.bit.ZRO = AQ_NO_ACTION;
		EPwm3Regs.AQCTLB.bit.ZRO = AQ_NO_ACTION;
		EPwm3Regs.AQCTLA.bit.CAD = AQ_CLEAR;
		EPwm3Regs.AQCTLB.bit.CAD = AQ_CLEAR;
	}
}



//===========================================================================
//#pragma CODE_SECTION(UpdatePWMPrd, "ramfuncs");
void UpdatePWMPrd(void)
{
	EPwm1Regs.TBPRD = REC_T_PWMH;		//REC_T_PWMH:半载波周期高字
	EPwm2Regs.TBPRD = REC_T_PWMH;		//REC_T_PWMH:半载波周期高字
	EPwm3Regs.TBPRD = REC_T_PWMH;		//REC_T_PWMH:半载波周期高字
}

void ForcePWMHigh(void)
{
	//强迫EPWM1A,EPWM1B为高
	EPwm1Regs.AQSFRC.bit.RLDCSF = 3; 		//立即加载
	EPwm1Regs.AQCSFRC.bit.CSFA = 1; 		//软件强迫输出A为High
	EPwm1Regs.AQCSFRC.bit.CSFB = 2;  		//软件强迫输出B为High
	//强迫EPWM2A,EPWM2B为高
	EPwm2Regs.AQSFRC.bit.RLDCSF = 3; 		//立即加载
	EPwm2Regs.AQCSFRC.bit.CSFA = 1; 		//软件强迫输出A为High
	EPwm2Regs.AQCSFRC.bit.CSFB = 2;  		//软件强迫输出B为High
	//强迫EPWM2A,EPWM2B为高
	EPwm3Regs.AQSFRC.bit.RLDCSF = 3; 		//立即加载
	EPwm3Regs.AQCSFRC.bit.CSFA = 1; 		//软件强迫输出A为High
	EPwm3Regs.AQCSFRC.bit.CSFB = 2;  		//软件强迫输出B为High
}






//===========================================================================
//PWM单元初始化程序
//===========================================================================
void SUB_PWM_INI(void)
{
	REC_T_PWMCH = (U32)ePWM_TIMER1_FREQ * 5000/RecCarrierFrq;
	
	REC_T_PWMH = REC_T_PWMCH;

	PWMA.UW_SIGN = 0;
	PWMA.UW_CNT1 = 0;
	PWMA.UW_CNT2 = 0;
	PWMA.PWM_WP = REC_T_PWMCH>>1;
	PWMA.PWM_WN = REC_T_PWMCH>>1;

	PWMB.UW_SIGN = 0;
	PWMB.UW_CNT1 = 0;
	PWMB.UW_CNT2 = 0;
	PWMB.PWM_WP = REC_T_PWMCH>>1;
	PWMB.PWM_WN = REC_T_PWMCH>>1;

	PWMC.UW_SIGN = 0;
	PWMC.UW_CNT1 = 0;
	PWMC.UW_CNT2 = 0;
	PWMC.PWM_WP = REC_T_PWMCH>>1;
	PWMC.PWM_WN = REC_T_PWMCH>>1;

	u16_udc_sw = 0;
	u16_n_modu = 0;

	EPwm1Regs.CMPA.half.CMPA = REC_T_PWMCH>>1;
	EPwm2Regs.CMPA.half.CMPA = REC_T_PWMCH>>1;
	EPwm3Regs.CMPA.half.CMPA = REC_T_PWMCH>>1;
	EPwm1Regs.CMPA.half.CMPA = REC_T_PWMCH>>1;
	EPwm2Regs.CMPA.half.CMPA = REC_T_PWMCH>>1;
	EPwm3Regs.CMPA.half.CMPA = REC_T_PWMCH>>1;

	EPwm1Regs.TBPRD = REC_T_PWMCH;		//T_PWMH:半载波周期高字
	EPwm2Regs.TBPRD = REC_T_PWMCH;		//T_PWMH:半载波周期高字
	EPwm3Regs.TBPRD = REC_T_PWMCH;		//T_PWMH:半载波周期高字
			
}

//===========================================================================
#pragma CODE_SECTION(SUB_CUR_POS, "ramfuncs");
void SUB_CUR_POS(void)
{
	U16	AX, DX;

	DeadTimeComp = DeadTimeCompSet;

	DX = SUB_VECT_ANGLE(RecCurrentFedD, RecCurrentFedQ);

	s32_theta_mt += ((S32)((S16)DX) * 65536 - s32_theta_mt)>>6;
	u16_theta_mt = s32_theta_mt>>16;

//================================================
//确定扇区
//================================================

	AX = ThetaAtDQ + u16_theta_mt;
	AX = (AX + 5462)/10923;				//得到扇区号0~5, 加30度是因为-30度~30度表示0扇区，其余类推
															//10923 = 65536/6
//======================================
//在同步坐标系楔过计算电流矢量位置确定各相电流极性
//=================================================================================
//电角度	|	-30~30°	|	30°~90° |	90°~150° | 150°~210°| 210°~270°	|270°~330°|
//---------------------------------------------------------------------------------
//扇区	|	0		|	 1		|	 2	  	 |		 3	|	4		|	5	|
//---------------------------------------------------------------------------------
//Ia		|   +		|	 +		|	 -		 |		 -	|	-		|	+	|
//--------------------------------------------------------------------------------
//Ib		|	-		|	 +		|	 +		 |		 +	|	-		|	-	|
//--------------------------------------------------------------------------------
//Ic		|	-		|	 -		|	 -		 |		 +	|	+		|	+	|
//---------------------------------------------------------------------------------
//I_OLAR	|	0x0001	|	 0x0003	|	 0x0002	 |	 0x0006	|	0x0004	|0x0005	|
//=================================================================================
	if(AX >= 6)
		AX = AX - 6;

	u16_i_polar = TABLE_SECTOR[AX];		//各相电流极性保存在I_POLAR的低3位
}


#pragma CODE_SECTION(SUB_CAL_PWM, "ramfuncs");
U16 SUB_CAL_PWM(struct PWM_MODU_STRUCT *ptr, U16 bit_sel)
{
	S16 PWM_TMP1;
	
	PWM_TMP1 = ((S32)(qsin(u16_ua_phase)) * 18919L + (S32)(qsin(3 * u16_ua_phase)) * 3153L)>>14;    //Q15
	PWM_TMP1 = abs((S32)PWM_TMP1 * u16_m_vect>>15);    //Q12


	//======================================
	//判断调制波幅值，决定是否过调制？
	//======================================
	if(PWM_TMP1 >= 0x1000)					//PWM_TMP>1000H则过调制
	{										//过渡过程时的过调制
		if(ptr->UW_CNT1 != u16_n_modu)
		{									//过渡过程
			ptr->UW_CNT1++;
			if(ptr->UW_CNT1 > u16_n_modu)
				ptr->UW_CNT1 = u16_n_modu;
			if((u16_ua_phase & BIT15) == BIT15)
				PWM_TMP1 = REC_PWM_LIMN;		//u16_ua_phase>180°,过渡过程脉宽基准为PW_LIMN
			else
				PWM_TMP1 = REC_PWM_LIMP;		//u16_ua_phase<180°,过渡过程脉宽基准为PW_LIMP
			ptr->PWM_W = PWM_TMP1;
			ptr->PWM_WP = PWM_TMP1 + DeadTimeComp;		//前补
			if(ptr->PWM_WP >= REC_T_PWMH)
				ptr->PWM_WP = REC_T_PWMH;
			ptr->PWM_WN = PWM_TMP1 - DeadTimeComp;		//后补
			if(ptr->PWM_WN < 0)
				ptr->PWM_WN = 0;			
		}
		else
		{										//I_PWMA_B3

			ptr->UW_CNT1 = u16_n_modu;

			if((u16_ua_phase & BIT15) == BIT15)	//ptr->PWM_WN = T_PWMH, ptr->PWM_WP = T_PWMH, ptr->PWM_W = T_PWMH
			{
				ptr->PWM_WN = REC_T_PWMH;
				ptr->PWM_WP = REC_T_PWMH;
				ptr->PWM_W = REC_T_PWMH;
			}
			else
			{									//I_PWMA_B4		//u16_ua_phase<180°,ptr->PWM_WN = 0, ptr->PWM_WP = 0, ptr->PWM_W = 0
				ptr->PWM_WN = 0;
				ptr->PWM_WP = 0;
				ptr->PWM_W = 0;
			}
		}
	}
	else
	{											//I_PWMA_B5
		if(ptr->UW_CNT1!=0)
		{										//过调制返回	//u16_pwm_flag = 3;
			if(ptr->UW_CNT1<1)
				ptr->UW_CNT1 = 0;
			else
				ptr->UW_CNT1--;
									
			PWM_TMP1 = REC_PWM_LIMP;				//u16_ua_phase<180°, 脉宽基准 = s16_pwmp(调制正极限)
			if((u16_ua_phase & BIT15) == BIT15)
				PWM_TMP1 = REC_PWM_LIMN;			//u16_ua_phase>180°, 脉宽基准 = PW_LIMN(调制负极限)

			//I_PWMA_B13	加入死区补偿
			ptr->PWM_W = PWM_TMP1;
			ptr->PWM_WP = PWM_TMP1 + DeadTimeComp;
			if(ptr->PWM_WP >= REC_T_PWMH)
				ptr->PWM_WP = REC_T_PWMH;
			ptr->PWM_WN = PWM_TMP1 - DeadTimeComp;
			if(ptr->PWM_WN < 0)
				ptr->PWM_WN = 0;
		}
		else
		{											//I_PWMA_B7//PWM_TMP1 = ((S32)PWM_TMP1 * u16_t_pwmh)>>11;														
		  	PWM_TMP1 = ((S32)PWM_TMP1 * REC_T_PWMH)>>12;
					
			if((u16_ua_phase & BIT15) == BIT15)		//180～360度
			{
				//PWM_TMP1 = PWM_TMP1;			//不取反
				if((ptr->UW_SIGN & BIT0) == BIT0)	//判断UA是否从正
				{
					ptr->UW_SIGN = 0;	//负符号
					ptr->UW_CNT2 = 0;
					ptr->UW_CNT1 = 0;
				}
			}
			if((u16_ua_phase & BIT15) == 0)		//0～180度
			{
				PWM_TMP1 = -PWM_TMP1;			//取反
				if((ptr->UW_SIGN & BIT0) == 0)		//判断UA
				{
					ptr->UW_SIGN = 1;			//正符号
					ptr->UW_CNT2 = 0;
					ptr->UW_CNT1 = 0;
				}
			}
			PWM_TMP1 = ((S32)PWM_TMP1 + REC_T_PWMH)>>1;

			if(PWM_TMP1 <= REC_PWM_LIMP)
			{
				PWM_TMP1 = REC_PWM_LIMP;			//脉宽基准<=PW_LIMP,则脉宽基准 = PW_LIMP
				if(ptr->UW_CNT2 >= u16_n_modu)
				{
					//I_PWMA_B11		//如果在过渡过程结束后, 发现脉宽基准仍然<=PW_LIMP, 则脉宽基准 = 0
					ptr->PWM_WP = 0;
					ptr->PWM_WN = 0;
					ptr->UW_CNT2 = 0;
					ptr->PWM_W = 0;
				}
				else
				{				//I_PWMA_B12
					ptr->UW_CNT2++;
					ptr->PWM_W = PWM_TMP1;
					ptr->PWM_WP = PWM_TMP1 + DeadTimeComp;
					if(ptr->PWM_WP >= REC_T_PWMH)
						ptr->PWM_WP = REC_T_PWMH;
					ptr->PWM_WN = PWM_TMP1 - DeadTimeComp;
					if(ptr->PWM_WN < 0)
						ptr->PWM_WN = 0;
				}
			}
			else
			{											//I_PWMA_B10
				if(PWM_TMP1 >= REC_PWM_LIMN)
				{
					PWM_TMP1 = REC_PWM_LIMN;				//脉宽基准>=PW_LIMN,则脉宽基准 = PW_LIMN
					if(ptr->UW_CNT2 >= u16_n_modu)
					{									//I_PWMA_B11															
						ptr->PWM_WP = REC_T_PWMH;		//如果在过渡过程结束后, 发现脉宽基准仍然>=PW_LIMP, 则脉宽基准 = T_PWMH
						ptr->PWM_WN = REC_T_PWMH;
						ptr->UW_CNT2 = 0;
						ptr->PWM_W = REC_T_PWMH;
					}
					else
					{									//I_PWMA_B12
						ptr->UW_CNT2++;
						ptr->PWM_W = PWM_TMP1;
						ptr->PWM_WP = PWM_TMP1 + DeadTimeComp;
						if(ptr->PWM_WP >= REC_T_PWMH)
							ptr->PWM_WP = REC_T_PWMH;
						ptr->PWM_WN = PWM_TMP1 - DeadTimeComp;
						if(ptr->PWM_WN < 0)
							ptr->PWM_WN = 0;
					}
				}
				else
				{										//I_PWMA_B13
					ptr->PWM_W = PWM_TMP1;
					ptr->PWM_WP = PWM_TMP1 + DeadTimeComp;
					if(ptr->PWM_WP >= REC_T_PWMH)
						ptr->PWM_WP = REC_T_PWMH;
					ptr->PWM_WN = PWM_TMP1 - DeadTimeComp;
					if(ptr->PWM_WN < 0)
						ptr->PWM_WN = 0;
				}
			}
		}
	}

	//根据电流极性，给比较单元的缓存单元赋值。
	if((u16_i_polar & bit_sel) == 0)
		ptr->CMPR_BAK = ptr->PWM_WP;
	else
		ptr->CMPR_BAK = ptr->PWM_WN;
	//对比较单元的缓存单元，进行限幅处理。
	if((ptr->CMPR_BAK != 0)&&(ptr->CMPR_BAK != REC_T_PWMH))
	{
		if(ptr->CMPR_BAK >= REC_PWM_LIMN)
			ptr->CMPR_BAK = REC_PWM_LIMN;
		else
		{
			if(ptr->CMPR_BAK <= REC_PWM_LIMP)
				ptr->CMPR_BAK = REC_PWM_LIMP;
		}
	}
	return (ptr->PWM_W);
}

//===========================================================================
// 矢量控制PWM发生程序
//===========================================================================
#pragma CODE_SECTION(SUB_VECT_PWM, "ramfuncs");
void SUB_VECT_PWM(void)
{
	if(u16_udc_sw == 1)
	{									//I_PWM_B2
		if((s16_udc + 0x0022) > 0x0f13)
		{								//I_PWM_B3	UDC>阈值,过渡过程次数 = 1
			u16_udc_sw = 1;
			u16_n_modu = 1;
		}
		else
		{								//I_PWM_B4	UDC<=阈值,过渡过程次数 = 2
			u16_udc_sw = 0;
			u16_n_modu = 2;
		}
	}
	else	
	{									//无滞环,电贡冉系阄?13C4H = 506.0V
		if(s16_udc > 0x0f13)
		{								//I_PWM_B3	UDC>阈值,过渡过程次数 = 1
			u16_udc_sw = 1;
			u16_n_modu = 1;
		}
		else
		{								//I_PWM_B4	UDC<=阈值,过渡过程次数 = 2
			u16_udc_sw = 0;
			u16_n_modu = 2;
		}
	}

	//=======================================
	//求A相电压相角UA_PHASE = THTA_MAH + U_THTAT, 
	//U_THTAT---电压矢量与M轴夹角,顺时针为正, 
	//THTA_MAH---M轴相对静止坐标系的夹角
	//=======================================
	u16_ua_phase = ThetaAtDQ + RecUoutTheta + 0x4000;// - P_F2_04;//+ RecUoutTheta + 0x4000;
	SUB_CAL_PWM(&PWMA,BIT0);
	//u_16cmpr_1=SUB_CAL_PWM(&PWMA,BIT0);
	//==========================================================
	//求B相调制系数
	//==========================================================
	u16_ua_phase = u16_ua_phase - 0x5555;	//0x5555 对应120度
	SUB_CAL_PWM(&PWMB,BIT1);
	//u_16cmpr_2=SUB_CAL_PWM(&PWMB,BIT1);
	//==========================================================
	//求C相调制系数
	//==========================================================
	u16_ua_phase = u16_ua_phase - 0x5555;
	SUB_CAL_PWM(&PWMC,BIT2);
	//u_16cmpr_3=SUB_CAL_PWM(&PWMC,BIT2);
	
	EPwm1Regs.CMPA.half.CMPA = PWMA.CMPR_BAK;
	EPwm2Regs.CMPA.half.CMPA = PWMB.CMPR_BAK;
	EPwm3Regs.CMPA.half.CMPA = PWMC.CMPR_BAK;
}





interrupt void T5PINT(void)
{
	PieCtrlRegs.PIEACK.all |= PIEACK_GROUP3;

	EINT;			// Enable Global interrupt INTM
	ERTM;			// Enable Global realtime interrupt DBGM

	EPwm5Regs.ETCLR.bit.INT = 1;

}



