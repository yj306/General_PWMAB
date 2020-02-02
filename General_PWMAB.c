//###########################################################################
// Multi pulse test firmware
// Yunlei Jiang
//
//
// Included Files
//
#include "F28x_Project.h"

//
// Defines
//
#define EPWM1_MAX_DB   0x03FF
#define EPWM2_MAX_DB   0x03FF
#define EPWM3_MAX_DB   0x03FF
#define EPWM1_MIN_DB   0
#define EPWM2_MIN_DB   0
#define EPWM3_MIN_DB   0
#define DB_UP          1
#define DB_DOWN        0

//
// Globals
//
Uint32 EPwm1TimerIntCount;
Uint32 EPwm2TimerIntCount;
Uint32 EPwm3TimerIntCount;
Uint16 EPwm1_DB_Direction;
Uint16 EPwm2_DB_Direction;
Uint16 EPwm3_DB_Direction;

Uint16 DPT_count = 0;
Uint16 DPT_maximum = 10;   // maximum pulse we may have
Uint16 sw_per;    // period = sw_per/TBCLK(200MHz) = 10 us
Uint16 Tn1;        // 1st pulse comparator = Tn1/TBCLK = 4us
Uint16 Tn2;        // 2nd pulse comparator = Tn2/TBCLK = 1us
Uint32 INT_Count;

//
// Function Prototypes
//
void InitEPwm1Example(void);
void InitEPwm4Example(void);

__interrupt void epwm1_isr(void);


//
// Main
//
void main(void)
{
//
// Step 1. Initialize System Control:
// PLL, WatchDog, enable Peripheral Clocks
// This example function is found in the F2837xD_SysCtrl.c file.
//
    InitSysCtrl();

//
// Step 2. Initialize GPIO:
// This example function is found in the F2837xD_Gpio.c file and
// illustrates how to set the GPIO to its default state.
//
//    InitGpio();

//
// enable PWM1, PWM2 and PWM3
//
    CpuSysRegs.PCLKCR2.bit.EPWM1=1;
    CpuSysRegs.PCLKCR2.bit.EPWM2=1;
    CpuSysRegs.PCLKCR2.bit.EPWM3=1;
    CpuSysRegs.PCLKCR2.bit.EPWM4=1;
    CpuSysRegs.PCLKCR2.bit.EPWM5=1;

//
// For this case just init GPIO pins for ePWM1, ePWM2, ePWM3
// These functions are in the F2837xD_EPwm.c file
//
    InitEPwm1Gpio();
    InitEPwm2Gpio();
    InitEPwm3Gpio();
    InitEPwm4Gpio();

//
// Step 3. Clear all interrupts and initialize PIE vector table:
// Disable CPU interrupts
//
    DINT;

//
// Initialize the PIE control registers to their default state.
// The default state is all PIE interrupts disabled and flags
// are cleared.
// This function is found in the F2837xD_PieCtrl.c file.
//
    InitPieCtrl();

//
// Disable CPU interrupts and clear all CPU interrupt flags:
//
    IER = 0x0000;
    IFR = 0x0000;

//
// Initialize the PIE vector table with pointers to the shell Interrupt
// Service Routines (ISR).
// This will populate the entire table, even if the interrupt
// is not used in this example.  This is useful for debug purposes.
// The shell ISR routines are found in F2837xD_DefaultIsr.c.
// This function is found in F2837xD_PieVect.c.
//
    InitPieVectTable();

//
// Interrupts that are used in this example are re-mapped to
// ISR functions found within this file.
//
    EALLOW; // This is needed to write to EALLOW protected registers
    PieVectTable.EPWM1_INT = &epwm1_isr;

    EDIS;   // This is needed to disable write to EALLOW protected registers

//
// Step 4. Initialize the Device Peripherals:
//
    EALLOW;
    CpuSysRegs.PCLKCR0.bit.TBCLKSYNC =0;
    EDIS;

    InitEPwm1Example();
    InitEPwm4Example();

    EALLOW;
    CpuSysRegs.PCLKCR0.bit.TBCLKSYNC =1;
    EDIS;

//
// Step 5. User specific code, enable interrupts:
// Initialize counters:
//
    EPwm1TimerIntCount = 0;
    EPwm2TimerIntCount = 0;
    EPwm3TimerIntCount = 0;

//
// Enable CPU INT3 which is connected to EPWM1-3 INT:
//
    IER |= M_INT3;

//
// Enable EPWM INTn in the PIE: Group 3 interrupt 1-3
//
    PieCtrlRegs.PIEIER3.bit.INTx1 = 1;
    PieCtrlRegs.PIEIER3.bit.INTx2 = 1;
    PieCtrlRegs.PIEIER3.bit.INTx3 = 1;

//
// Enable global Interrupts and higher priority real-time debug events:
//
    EINT;  // Enable Global interrupt INTM
    ERTM;  // Enable Global realtime interrupt DBGM

//
// Step 6. IDLE loop. Just sit and loop forever (optional):
//
    for(;;)
    {
        asm ("          NOP");
    }
}

//
// epwm1_isr - EPWM1 ISR
//
__interrupt void epwm1_isr(void)
{
    sw_per = 500;    // period = sw_per/TBCLK(100MHz) = 10 us
    Tn1 = 250;        // 1st pulse comparator = Tn1/TBCLK = 1.5us
    Tn2 = 100;        // 2nd pulse comparator = Tn2/TBCLK = 1.5us
    INT_Count++;

    if(INT_Count > 200000)
    {

        INT_Count = 0;
        DPT_count = 1;
    }

    if(DPT_count == 1)
    {
        EPwm4Regs.TBPRD = sw_per;    // set TBPRD for EPWM1
        EPwm1Regs.TBPRD = sw_per;    // set TBPRD for EPWM1
        // set EPWM1A reg for lower switch
        EPwm4Regs.CMPB.bit.CMPB = Tn1;                  // Set CMPA = Tn1
        EPwm4Regs.AQCTLB.bit.CBU = AQ_CLEAR;            // force low when TBCTR = CMPA
        EPwm4Regs.AQCTLB.bit.ZRO = AQ_SET;            // force high when TBCTR = 0

        EPwm1Regs.CMPB.bit.CMPB = Tn1;                  // Set CMPA = Tn1
        EPwm1Regs.AQCTLB.bit.CBU = AQ_CLEAR;            // force low when TBCTR = CMPA
        EPwm1Regs.AQCTLB.bit.ZRO = AQ_SET;            // force high when TBCTR = 0

        EPwm4Regs.CMPA.bit.CMPA = 0;                    // Set CMPB = 0
        EPwm4Regs.AQCTLA.bit.CAU = AQ_CLEAR;            // force low when TBCTR = CMPB
        EPwm4Regs.AQCTLA.bit.ZRO = AQ_CLEAR;              // force low when TBCTR = 0


        EPwm1Regs.CMPA.bit.CMPA = 0;                    // Set CMPB = 0
        EPwm1Regs.AQCTLA.bit.CAU = AQ_CLEAR;            // force low when TBCTR = CMPB
        EPwm1Regs.AQCTLA.bit.ZRO = AQ_CLEAR;              // force low when TBCTR = 0

        DPT_count ++;                                   // for 2nd pulse
    }
    else
    {
        if (DPT_count <= DPT_maximum)
        {
            EPwm4Regs.CMPB.bit.CMPB = Tn2;              // for the 2nd - nth pulse
            EPwm1Regs.CMPB.bit.CMPB = Tn2;              // for the 2nd - nth pulse

            DPT_count ++;
        }
        else
        {
            EPwm4Regs.CMPB.bit.CMPB = 0;                // terminate all the pulses
            EPwm4Regs.AQCTLA.bit.ZRO = AQ_CLEAR;        // force low when TBCTR = 0

            EPwm1Regs.CMPB.bit.CMPB = 0;                // terminate all the pulses
            EPwm1Regs.AQCTLA.bit.ZRO = AQ_CLEAR;        // force low when TBCTR = 0
        }

    }
    EPwm1TimerIntCount++;

    // Clear INT flag for this timer
    //
    EPwm1Regs.ETCLR.bit.INT = 1;

    //
    // Acknowledge this interrupt to receive more interrupts from group 3
    //
    PieCtrlRegs.PIEACK.all = PIEACK_GROUP3;
}


//
// InitEPwm1Example - Initialize EPWM1 configuration
//
void InitEPwm1Example()
{
    EPwm1Regs.TBPRD = 500;                       // Set timer period, PWM frequency = 200kHz Tsw = 5us  PWM_CLK = 100Mhz
    EPwm1Regs.TBPHS.bit.TBPHS = 0x0000;           // Phase is 0
    EPwm1Regs.TBCTR = 0x0000;                     // Clear counter

    //
    // Setup counter mode
    //
    EPwm1Regs.TBCTL.bit.CTRMODE = TB_COUNT_UP;     // Count up
    EPwm1Regs.TBCTL.bit.PHSEN = TB_DISABLE;        // Disable phase loading
    EPwm1Regs.TBCTL.bit.PHSEN = TB_SHADOW;         // Enable shadow register for TBPRD
    EPwm1Regs.TBCTL.bit.HSPCLKDIV = TB_DIV1;       // TBCLK = SYSCLKOUT
    EPwm1Regs.TBCTL.bit.CLKDIV = TB_DIV1;          // ratio again

    // Setup shadow mode
    EPwm1Regs.CMPCTL.bit.SHDWAMODE = CC_SHADOW;    // Enable shadow register for ePWM1A
    EPwm1Regs.CMPCTL.bit.SHDWBMODE = CC_SHADOW;    // Enable shadow register for ePWM1B
    EPwm1Regs.CMPCTL.bit.LOADAMODE = CC_CTR_ZERO;  // Load when TBCTR = 0 for ePWMA
    EPwm1Regs.CMPCTL.bit.LOADBMODE = CC_CTR_ZERO;  // Load when TBCTR = 0 for ePWMB

    //
    // Setup compare
    //
    EPwm1Regs.CMPA.bit.CMPA = 100;

    //
    // Set actions
    //
    //EPwm1Regs.AQCTLA.bit.CAU = AQ_CLEAR;            // force low when TBCTR = CMPA
    //EPwm1Regs.AQCTLA.bit.ZRO = AQ_SET;              // force high when TBCTR = 0

    //EPwm1Regs.AQCTLB.bit.CBU = AQ_CLEAR;          // Set PWM1A on Zero
    //EPwm1Regs.AQCTLB.bit.ZRO = AQ_CLEAR;

    //
    // Active Low PWMs - Setup Deadband
    //
   // EPwm1Regs.DBCTL.bit.OUT_MODE = DB_FULL_ENABLE;
  //  EPwm1Regs.DBCTL.bit.POLSEL = DB_ACTV_LO;
   // EPwm1Regs.DBCTL.bit.IN_MODE = DBA_ALL;
   // EPwm1Regs.DBRED.bit.DBRED = EPWM1_MIN_DB;
   // EPwm1Regs.DBFED.bit.DBFED = EPWM1_MIN_DB;
   // EPwm1_DB_Direction = DB_UP;

    //
    // Setup interrupt
    //
    EPwm1Regs.ETSEL.bit.INTSEL = ET_CTR_ZERO;    // Enable event when TBCTR = zero
    EPwm1Regs.ETSEL.bit.INTEN = 1;               // Enable INT generation
    EPwm1Regs.ETPS.bit.INTPRD = ET_1ST;          // Generate INT on 1st event

}

void InitEPwm4Example()
{
    EPwm4Regs.TBPRD = 500;                       // Set timer period, PWM frequency = 200kHz Tsw = 5us  PWM_CLK = 100Mhz
    EPwm4Regs.TBPHS.bit.TBPHS = 0x0000;           // Phase is 0
    EPwm4Regs.TBCTR = 0x0000;                     // Clear counter

    //
    // Setup counter mode
    //
    EPwm4Regs.TBCTL.bit.CTRMODE = TB_COUNT_UP;     // Count up
    EPwm4Regs.TBCTL.bit.PHSEN = TB_DISABLE;        // Disable phase loading
    EPwm4Regs.TBCTL.bit.PHSEN = TB_SHADOW;         // Enable shadow register for TBPRD
    EPwm4Regs.TBCTL.bit.HSPCLKDIV = TB_DIV1;       // TBCLK = SYSCLKOUT
    EPwm4Regs.TBCTL.bit.CLKDIV = TB_DIV1;          // ratio again

    // Setup shadow mode
    EPwm4Regs.CMPCTL.bit.SHDWAMODE = CC_SHADOW;    // Enable shadow register for ePWM1A
    EPwm4Regs.CMPCTL.bit.SHDWBMODE = CC_SHADOW;    // Enable shadow register for ePWM1B
    EPwm4Regs.CMPCTL.bit.LOADAMODE = CC_CTR_ZERO;  // Load when TBCTR = 0 for ePWMA
    EPwm4Regs.CMPCTL.bit.LOADBMODE = CC_CTR_ZERO;  // Load when TBCTR = 0 for ePWMB

    //
    // Setup compare
    //
    EPwm4Regs.CMPA.bit.CMPA = 0;



}

//
// End of file
//
