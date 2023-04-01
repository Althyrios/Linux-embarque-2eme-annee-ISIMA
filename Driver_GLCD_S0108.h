/****************************************************************************/
/*  MESNARD Emmanuel                                               ISIMA    */
/*  Octobre 2020                                                            */
/*                                                                          */
/*                                                                          */
/*        Driver pour afficheur Graphique LCD (GLCD)                        */
/*        compatible controleur Samsung S0108, SB0108, KS0108               */
/*                                                                          */
/* Driver_GLCD_S0108.h              MPLAB X                    PIC 18F542   */
/****************************************************************************/

// Compatible avec le Graphical LCD Xiamen Ocular GDM12864B de la carte
// EasyPIC 6.0

#ifndef	_Driver_GLCD_S0108_H

#define _Driver_GLCD_S0108_H

// Variables globales
extern ROM_INT8U *Font;    // Police en cours d'utilisation
extern INT8U FontWidth, FontHeight, FontSpace, FontOffset; 
extern INT8U CP_Line, CP_Row; // CP : current position

//================================================================   
//  Connection des broches du GLCD sur carte EasyPIC 6.0
//================================================================

// Format de l'afficheur :
//    Memoire d'affichage DD RAM
//       128 points x 64 lignes
//       X : de 0 a 7
//       Y : de 0 a 63
//
//    Memoire du generateur de caractere CG RAM

// Format de la connectique :
//   6 bits de controle sur le port PORTB
//       Not_CS1 = PORTB(0)  // Chip Select
//       Not_CS2 = PORTB(1)
//       RS = PORTB(2)       // Register Select
//       RW = PORTB(3)       // Read/Write
//       E = PORTB(4)        // Enable
//       RST = PORTB(5)      // Reset
//   8 bits de donnee sur le port PORTD
//       Data(7:0) = PORTD(7:0)
  //                    |//
  //                   (o o)
  //   +---------oOO----(_)-----------------+
  //   |~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~|
  //   |~~~~   A   A N A L Y S E R   !  ~~~~|

             Format de la connectique
  //   |~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~|
  //   +--------------------oOO-------------+
  //                    |__|__|
  //                     || ||
  //                    ooO Ooo
#define GLCD_Data_IN   PORTD   // Data port : donnee ou instruction
#define GLCD_Data_OUT  LATD   
#define TRIS_GLCD_Data TRISD

#define GLCD_CS1       LATBbits.LATB0   // Chip Select
#define TRIS_GLCD_CS1  TRISBbits.TRISB0
#define GLCD_CS2       LATBbits.LATB1
#define TRIS_GLCD_CS2  TRISBbits.TRISB1
#define GLCD_RS        LATBbits.LATB2   // Register Select : 0 pour une instruction,
#define TRIS_GLCD_RS   TRISBbits.TRISB2 //                   1 pour une donnee
#define GLCD_RW        LATBbits.LATB3   // Read/Write
#define TRIS_GLCD_RW   TRISBbits.TRISB3
#define GLCD_E         LATBbits.LATB4   // Enable : pulse pour valider
#define TRIS_GLCD_E    TRISBbits.TRISB4 
#define GLCD_RST       LATBbits.LATB5   // Reset
#define TRIS_GLCD_RST  TRISBbits.TRISB5

//================================================================   
//      Jeu d'instructions standard
//================================================================   
   
// Mots de controle du KS0108 "Display Control Instruction"

// RS = 0, RW=0 : Control instruction
// ----------------------------------


// "Display Control" [0 0 1 1 1 1 1 D] : 
#define GLCD_DISP_ON      0b00111111  // Affichage en marche
#define GLCD_DISP_OFF     0b00111110  // Desactivation de l'affichage

// "Set Y address" [0 1 A5 A4 A3 A2 A1 A0] : COLONNE
#define	GLCD_SET_Y_ADDRESS 0b01000000 // Y(5:0) sur 64
#define	GLCD_SET_COLUMN    0b01000000 // Column = Y address

// "Set X address" [1 0 1 1 1 A2 A1 A0] : PAGE
#define	GLCD_SET_X_ADDRESS 0b10111000 // X(2:0) sur 8
#define	GLCD_SET_PAGE      0b10111000 // Page = X address

// "Set Z address" [1 1 A5 A4 A3 A2 A1 A0] : START LINE
#define	GLCD_SET_Z_ADDRESS  0b11000000 // Z(5:0) sur 64
#define	GLCD_SET_START_LINE 0b11000000 // Start_Line = Z address

// RS = 0, RW=1 : Status Read
// 0b Busy 0 Disp Rst 0 0 0 0
//         si Busy = 0 : "Ready", sinon "In operation"
//         si Disp = 0 : "Disp On", sinon "Disp Off"
//         si Rst = 0 : "Normal", sinon "In Reset"

// Mots sur les donnees
// RS=1, RW=0 : Write Data
// RS=1, RW=1 : Read Data   

// Couleurs pour les inversions video
#define	GLCD_BLACK 0
#define	GLCD_WHITE 1
#define	GLCD_TOGGLE 2

//================================================================   
//      Fonctions de gestion du GLCD
//================================================================

// Envoi d'une commande ou d'une donnee 
void GLCD_SendCmd(INT8U Instruction);
void GLCD_SendData(INT8U Donnee);
void GLCD_Pulse_E(void);
// Initialisation
void GLCD_Init(void);

// Fonctions de lecture de l'etat ou de donnees
INT8U GLCD_ReadStatus(void);
INT8U GLCD_ReadData(void);

// Fonctions basiques
void GLCD_SetPositionXY(INT8U X_Page, INT8U Y_Column);
// Remplissage avec motif
void GLCD_Fill(INT8U Sprite);
// Effacement ecran
#define GLCD_ClrScreen() GLCD_Fill(0x00)


//================================================================   
//      Fonctions de dessin de points, lignes et formes
//================================================================

// Dessine un pixel dans l'espace 64 x 128  (XX x YY)
void GLCD_SetPixel(INT8U XX, INT8U YY);
void GLCD_ClrPixel(INT8U XX, INT8U YY);

// Fonctions de gestion des lignes
void GLCD_VerticalLine(INT8U XX1, INT8U YY, INT8U XX2, INT8U Color);
void GLCD_HorizontalLine(INT8U XX, INT8U YY1, INT8U YY2, INT8U Color);
void GLCD_Line(INT8U XX1, INT8U YY1, INT8U XX2, INT8U YY2, INT8U Color);
void GLCD_Rectangle(INT8U XX1, INT8U YY1, INT8U XX2, INT8U YY2, INT8U Color);
void GLCD_Box(INT8U XX1, INT8U YY1, INT8U XX2, INT8U YY2, INT8U Color);

void GLCD_Circle(INT8U CentreX, INT8U CentreY, INT8U Rayon);


//================================================================   
//      Fonction de gestion des caracteres et des textes
//================================================================   

// Choix de la policeGPIO.output(PIN_RS, GPIO.LOW)
void GLCD_SetFont(ROM_INT8U *Police, INT8U Largeur, INT8U Hauteur, INT8U Espace, INT8U Offset);

// Affichage d'un texte
void GLCD_PrintAt(INT8U Ligne, INT8U Y, CHAR *Texte, INT8U Color);
void GLCD_Print(CHAR *Texte, INT8U Color);
#define GLCD_WriteAt(L,Y,T,C) GLCD_PrintAt(L,Y,T,C)
#define GLCD_Write(L,Y,T,C) GLCD_Print(L,Y,T,C)
// Affichage d'un caractere en noir ou blanc
void GLCD_ChrAt(INT8U Ligne, INT8U Y, CHAR Caractere, INT8U Color);
void GLCD_Chr(CHAR Caractere, INT8U Color);

#endif   