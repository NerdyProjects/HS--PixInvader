
#line 1 "../writeEeprom.c" /0












 
  
#line 1 "..\MAIN.H" /0





 
 
 
 
 
 
 
 
 
 
 
  
#line 1 "Z:\HOME\MATTHIAS\PROJEKTE\HARDWARENAHE SYSTEMENTWICKLUNG\C51\INC\ATMEL/REGX52.H" /0






 
 
 
 
 


 
 sfr P0      = 0x80;
 sfr SP      = 0x81;
 sfr DPL     = 0x82;
 sfr DPH     = 0x83;
 sfr PCON    = 0x87;
 sfr TCON    = 0x88;
 sfr TMOD    = 0x89;
 sfr TL0     = 0x8A;
 sfr TL1     = 0x8B;
 sfr TH0     = 0x8C;
 sfr TH1     = 0x8D;
 sfr P1      = 0x90;
 sfr SCON    = 0x98;
 sfr SBUF    = 0x99;
 sfr P2      = 0xA0;
 sfr IE      = 0xA8;
 sfr P3      = 0xB0;
 sfr IP      = 0xB8;
 sfr T2CON   = 0xC8;
 sfr T2MOD   = 0xC9;
 sfr RCAP2L  = 0xCA;
 sfr RCAP2H  = 0xCB;
 sfr TL2     = 0xCC;
 sfr TH2     = 0xCD;
 sfr PSW     = 0xD0;
 sfr ACC     = 0xE0;
 sfr B       = 0xF0;
 


 
 sbit P0_0 = 0x80;
 sbit P0_1 = 0x81;
 sbit P0_2 = 0x82;
 sbit P0_3 = 0x83;
 sbit P0_4 = 0x84;
 sbit P0_5 = 0x85;
 sbit P0_6 = 0x86;
 sbit P0_7 = 0x87;
 


 
 
 
 
 
 
 
 
 
 


 
 sbit IT0  = 0x88;
 sbit IE0  = 0x89;
 sbit IT1  = 0x8A;
 sbit IE1  = 0x8B;
 sbit TR0  = 0x8C;
 sbit TF0  = 0x8D;
 sbit TR1  = 0x8E;
 sbit TF1  = 0x8F;
 


 
 
 
 
 
 
 
 
 
 
 
 
 


 
 sbit P1_0 = 0x90;
 sbit P1_1 = 0x91;
 sbit P1_2 = 0x92;
 sbit P1_3 = 0x93;
 sbit P1_4 = 0x94;
 sbit P1_5 = 0x95;
 sbit P1_6 = 0x96;
 sbit P1_7 = 0x97;
 
 sbit T2   = 0x90;        
 sbit T2EX = 0x91;        
 


 
 sbit RI   = 0x98;
 sbit TI   = 0x99;
 sbit RB8  = 0x9A;
 sbit TB8  = 0x9B;
 sbit REN  = 0x9C;
 sbit SM2  = 0x9D;
 sbit SM1  = 0x9E;
 sbit SM0  = 0x9F;
 


 
 sbit P2_0 = 0xA0;
 sbit P2_1 = 0xA1;
 sbit P2_2 = 0xA2;
 sbit P2_3 = 0xA3;
 sbit P2_4 = 0xA4;
 sbit P2_5 = 0xA5;
 sbit P2_6 = 0xA6;
 sbit P2_7 = 0xA7;
 


 
 sbit EX0  = 0xA8;        
 sbit ET0  = 0xA9;        
 sbit EX1  = 0xAA;        
 sbit ET1  = 0xAB;        
 sbit ES   = 0xAC;        
 sbit ET2  = 0xAD;        
 
 sbit EA   = 0xAF;        
 


 
 sbit P3_0 = 0xB0;
 sbit P3_1 = 0xB1;
 sbit P3_2 = 0xB2;
 sbit P3_3 = 0xB3;
 sbit P3_4 = 0xB4;
 sbit P3_5 = 0xB5;
 sbit P3_6 = 0xB6;
 sbit P3_7 = 0xB7;
 
 sbit RXD  = 0xB0;        
 sbit TXD  = 0xB1;        
 sbit INT0 = 0xB2;        
 sbit INT1 = 0xB3;        
 sbit T0   = 0xB4;        
 sbit T1   = 0xB5;        
 sbit WR   = 0xB6;        
 sbit RD   = 0xB7;        
 


 
 sbit PX0  = 0xB8;
 sbit PT0  = 0xB9;
 sbit PX1  = 0xBA;
 sbit PT1  = 0xBB;
 sbit PS   = 0xBC;
 sbit PT2  = 0xBD;
 


 
 sbit CP_RL2= 0xC8;       
 sbit C_T2 = 0xC9;        
 sbit TR2  = 0xCA;        
 sbit EXEN2= 0xCB;        
 sbit TCLK = 0xCC;        
 sbit RCLK = 0xCD;        
 sbit EXF2 = 0xCE;        
 sbit TF2  = 0xCF;        
 


 
 
 
 


 
 sbit P    = 0xD0;
 sbit FL   = 0xD1;
 sbit OV   = 0xD2;
 sbit RS0  = 0xD3;
 sbit RS1  = 0xD4;
 sbit F0   = 0xD5;
 sbit AC   = 0xD6;
 sbit CY   = 0xD7;
 



 
 
 
 
 
 
 
 
 
 
 
 
 
#line 17 "..\MAIN.H" /0
 
 
#line 19 "..\MAIN.H" /1
  
 
#line 21 "..\MAIN.H" /0
 
#line 22 "..\MAIN.H" /1
  
 
#line 24 "..\MAIN.H" /0
 
 
#line 26 "..\MAIN.H" /1
  
  
  
  
  
 
#line 32 "..\MAIN.H" /0
 
 
#line 34 "..\MAIN.H" /1
  
  
  
  
  
  
 
#line 41 "..\MAIN.H" /0
 
 
#line 43 "..\MAIN.H" /1
  
  
  
 
#line 47 "..\MAIN.H" /0
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 




 
 
 
 
#line 14 "../writeEeprom.c" /0
 
  
#line 1 "..\SPI.H" /0





 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
#line 15 "../writeEeprom.c" /0
 
 
 extern unsigned long get4Bytes(void);
 
 



 
 unsigned char handleSPI(void)
 {
 long recv;
 unsigned char b3, b2, b1, b0;
 recv = get4Bytes();
 b3 = recv >> 24;
 b2 = recv >> 16;
 b1 = recv >> 8;
 b0 = recv;
 switch (b0) {
 case 0x01: {
 unsigned char xdata *dst = (unsigned char *) (((unsigned short) b1) << 8 | b2);
 *dst = b3;
 break;
 }
 case 0x02:
 {
 unsigned char cnt = b3;
 unsigned char i = 0;
 xdata unsigned char pageBuf[66];	 
 unsigned char xdata *dst = (unsigned char *) (((unsigned short) b1) << 8 | b2);
 if(cnt > 64)
 {
 return 1;
 }
 for (i = 0; i < cnt; i += 3) {
 recv = get4Bytes();
 b3 = recv >> 24;
 b2 = recv >> 16;
 b1 = recv >> 8;
 b0 = recv;
 if (b0 != 0x80)
 {
 
 return 1;
 }
 pageBuf[i] = b1;
 pageBuf[i + 1] = b2;
 pageBuf[i + 2] = b3;
 }
 for (i = 0; i < cnt; ++i) {
 *(dst++) = pageBuf[i];
 }
 break;
 }
 case 0xF0:
 return 0;
 break;
 }
 }
