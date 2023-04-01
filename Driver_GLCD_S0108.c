/****************************************************************************/
/*  MESNARD Emmanuel                                               ISIMA    */
/*  Octobre 2020                                                            */
/*                                                                          */
/*                                                                          */
/*        Driver pour afficheur Graphique LCD (GLCD)                        */
/*        compatible controleur Samsung S0108, SB0108, KS0108               */
/*                                                                          */
/* Driver_GLCD_S0108.c              MPLAB X                    PIC 18F542   */
/****************************************************************************/

// Compatible avec le Graphical LCD Xiamen Ocular GDM12864B de la carte
// EasyPIC 6.0

#define _Driver_GLCD_S0108_C

#include <xc.h>
#include "TypesMacros.h"
#include "Driver_GLCD_S0108.h"


// Variables globales
extern ROM_INT8U *Font; // Police en cours d'utilisation
extern INT8U FontWidth, FontHeight, FontSpace, FontOffset;
extern INT8U CP_Line, CP_Row; // CP : current position


//================================================================   
//      Fonctions d'envoi de commandes ou de donnees
//================================================================

void GLCD_SendCmd(INT8U Instruction) {
    GLCD_RS = 0; // c'est une commande
    GLCD_RW = 0; // on �crit une commande (�criture)
    GLCD_Data_OUT = Instruction;
    TRIS_GLCD_Data = 0; // Cette donn�e est en sortie 
    GLCD_Pulse_E();
}

void GLCD_SendData(INT8U Donnee) {
    GLCD_RS = 1;
    GLCD_RW = 0; // Data mode
    GLCD_Data_OUT = Donnee;
    TRIS_GLCD_Data = 0;
    GLCD_Pulse_E();
}

// Generation d'un front sur la broche GLCD_E (PORTB(4)) servant a la validation

void GLCD_Pulse_E(void) {
    GLCD_E = 1;
    Delai_us(1); // Attente duree d'1 micro seconde (au moins 450 ns)
    GLCD_E = 0; // front descendant effectif
}

// Initialisation du GLCD

void GLCD_Init(void) {
    // orientation des broches,
    TRIS_GLCD_Data = 0; // Donn�e en sortie
    TRIS_GLCD_CS1 = 0;
    TRIS_GLCD_CS2 = 0;
    TRIS_GLCD_RS = 0;
    TRIS_GLCD_RW = 0;
    TRIS_GLCD_E = 0;
    TRIS_GLCD_RST = 0;

    // attente power on,
    Delai_ms(15);

    // gestion du RST
    GLCD_CS1 = 1;
    GLCD_CS2 = 1;
    Delai_us(1);

    GLCD_RST = 1;
    Delai_us(1);

    // Pour terminer, envoi de quelques commandes
    GLCD_SendCmd(GLCD_DISP_OFF); // Normalement, deja effectue
    GLCD_SendCmd(GLCD_SET_START_LINE | 0x00); // Idem
    GLCD_SendCmd(GLCD_SET_PAGE | 0x00); // Positionne les adresses a 0
    GLCD_SendCmd(GLCD_SET_COLUMN | 0x00);
    GLCD_SendCmd(GLCD_DISP_ON); // Allume et efface l'ecran
    GLCD_ClrScreen();
}

//================================================================   
//      Fonctions de lecture de l'etat ou de donnees
//================================================================   

// Lecture de l'etat : 0b Busy 0 Disp Rst 0 0 0 0
//         si Busy = 0 : "Ready", sinon "In operation"
//         si Disp = 0 : "Disp On", sinon "Disp Off"
//         si Rst = 0 : "Normal", sinon "In Reset"

INT8U GLCD_ReadStatus(void) {
    INT8U CurrentStatus;

    GLCD_RS = 0;
    GLCD_RW = 1; // Status Read
    GLCD_Data_OUT = 0xFF; // Efface le bus
    TRIS_GLCD_Data = 0xFF;
    GLCD_E = 1;
    Delai_us(5);
    CurrentStatus = GLCD_Data_IN; // Lecture du bus
    GLCD_E = 0;
    Delai_ms(15);
    return (CurrentStatus);
}

// Lecture d'une donnee

INT8U GLCD_ReadData(void) {
    INT8U CurrentData;

    GLCD_RS = 1;
    GLCD_RW = 1; // Data Read
    GLCD_Data_OUT = 0xFF; // Efface le bus
    TRIS_GLCD_Data = 0xFF;
    GLCD_E = 1;
    CurrentData = GLCD_Data_IN; // Lecture du bus
    GLCD_E = 0;
    return (CurrentData);
}

//================================================================   
//      Fonctions basiques : position et remplissage   
//================================================================   

// Positionne en X et Y

void GLCD_SetPositionXY(INT8U X_Page, INT8U Y_Column) {
    GLCD_SendCmd(GLCD_SET_X_ADDRESS | X_Page);
    GLCD_SendCmd(GLCD_SET_Y_ADDRESS | Y_Column);
}

// Remplissage total de l'ecran avec motif

void GLCD_Fill(INT8U Sprite) {
    INT8U x; // Page x sur 3 bits
    INT8U y; // Colonne y sur 6 bits   

    GLCD_CS1 = 1; // Remplissage des deux cotes a la fois
    GLCD_CS2 = 1;
    for (x = 0; x < 8; x++) {
        GLCD_SetPositionXY(x, 0x00);
        for (y = 0; y < 64; y++) GLCD_SendData(Sprite);
    }
}

// Effacement ecran
// definit par #define GLCD_ClrScreen() GLCD_Fill(0x00)


//================================================================   
//      Fonctions de gestion d'un pixel : Set et Clr    
//================================================================   

// Dessine un pixel dans l'espace 64 x 128  (XX x YY)


// TODO: inversion X et Y

void GLCD_SetPixel(INT8U XX, INT8U YY) {
    INT8U x; // Page x sur 3 bits
    INT8U y; // Colonne y sur 6 bits  
    INT8U BitNbr; // Numero de bit sur l'octet considere
    INT8U octet; // Octet de donnee lu a l'adresse indiquee

    x = XX >> 3; // XX / 8
    BitNbr = XX & 7; // XX % 8

    // Calcul de la position y (Colonne, gauche ou droite)
    if (YY < 64) {
        GLCD_CS1 = 0;
        GLCD_CS2 = 1; // Partie gauche
        y = YY;
    } else {
        GLCD_CS1 = 1;
        GLCD_CS2 = 0; // Partie droite
        y = YY - 64;
    }

    // Lecture de l'ancienne valeur de l'octet a cette position x, y
    GLCD_SetPositionXY(x, y);
    octet = GLCD_ReadData();
    
    GLCD_SetPositionXY(x, y);
    octet = GLCD_ReadData();
    
    // Mise a 1 du bit demande(BitNbr) sur l'octet lu
    octet |= 1 << BitNbr;
    
    // Ecriture effective de cet octet sur l'ecran  
    GLCD_SetPositionXY(x, y);
    GLCD_SendData(octet);
}

void GLCD_ClrPixel(INT8U XX, INT8U YY) {
    INT8U x; // Page x sur 3 bits
    INT8U y; // Colonne y sur 6 bits  
    INT8U BitNbr; // Numero de bit sur l'octet considere
    INT8U octet; // Octet de donnee lu a l'adresse indiquee

    x = XX >> 3; // XX / 8
    BitNbr = XX & 7; // XX % 8

    // Calcul de la position y (Colonne, gauche ou droite)
    if (YY < 64) {
        GLCD_CS1 = 0;
        GLCD_CS2 = 1; // Partie gauche
        y = YY;
    } else {
        GLCD_CS1 = 1;
        GLCD_CS2 = 0; // Partie droite
        y = YY - 64;
    }

    // Lecture de l'ancienne valeur de l'octet a cette position x, y
    GLCD_SetPositionXY(x, y);
    octet = GLCD_ReadData();
    
    GLCD_SetPositionXY(x, y);
    octet = GLCD_ReadData();
    
    // Mise a 1 du bit demande(BitNbr) sur l'octet lu
    octet &= ~(1 << BitNbr);
    
    // Ecriture effective de cet octet sur l'ecran  
    GLCD_SetPositionXY(x, y);
    GLCD_SendData(octet);
}

//================================================================   
//      Fonctions de gestion des lignes droites   
//================================================================  

void GLCD_VerticalLine(INT8U XX1, INT8U YY, INT8U XX2, INT8U Pattern) {
    
    // TODO : finir le code
    
    /*INT8U x; // Page x sur 3 bits
    INT8U y; // Colonne y sur 6 bits  
    INT8U BitNbr; // Numero de bit sur l'octet considere
    INT8U octet; // Octet de donnee lu a l'adresse indique
    
    INT8U xDeb, xFin;
    INT8U BitNbrDeb, BitNbrFin;

    if (XX1 < XX2) {
        xDeb = XX1 >> 3;
        BitNbrDeb = XX1 & 7;
        
        xFin = XX2 >> 3;
        BitNbrFin = XX2 & 7;
    } else {
        xDeb = XX2 >> 3;
        BitNbrDeb = XX2 & 7;
        
        xFin = XX1 >> 3;
        BitNbrFin = XX1 & 7;
    }

    // Calcul de la position y (Colonne, gauche ou droite)
    if (YY < 64) {
        GLCD_CS1 = 0;
        GLCD_CS2 = 1; // Partie gauche
        y = YY;
    } else {
        GLCD_CS1 = 1;
        GLCD_CS2 = 0; // Partie droite
        y = YY - 64;
    }*/
    INT8U XXdeb, XXfin; // Remise en ordre des YY1 et YY2
    INT8U iXX;

    if (XX1 < XX2) {
        XXdeb = XX1;
        XXfin = XX2;
    } else {
        XXdeb = XX2;
        XXfin = XX1;
    }
    
    // Vertical line de substitution 
    
    for (iXX = XXdeb; iXX < XXfin; iXX++)
        if (Pattern & (1 << (iXX & 7)))
            GLCD_SetPixel(iXX, YY);
}

void GLCD_HorizontalLine(INT8U XX, INT8U YY1, INT8U YY2, INT8U Pattern) {
    INT8U YYdeb, YYfin; // Remise en ordre des YY1 et YY2
    INT8U iYY;

    if (YY1 < YY2) {
        YYdeb = YY1;
        YYfin = YY2;
    } else {
        YYdeb = YY1;
        YYfin = YY2;
    }
    
    for (iYY = YYdeb; iYY <= YYfin; iYY ++)
        if (Pattern & (1 << (iYY & 7)))
            GLCD_SetPixel(XX, iYY);
}

void GLCD_Line(INT8U XX1, INT8U YY1, INT8U XX2, INT8U YY2, INT8U Color) {
    INT8U XXdeb, XXfin, YYdeb, YYfin; // Remise en ordre des points
    float a, b; // equation de la droite Y = aX+b
    INT8U XX, iYY; // index pour le trace de la droite

    if (XX1 == XX2) GLCD_HorizontalLine(XX1, YY1, YY2, Color);
    else if (YY1 == YY2) GLCD_VerticalLine(XX1, YY1, XX2, Color);
    else { // Trace en diagonale

        //                    |//
        //                   (o o)
        //   +---------oOO----(_)-----------------+
        //   |~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~|
        //   |~~~~  A   C O M P L E T E R   ! ~~~~|
        //   |~~~  E V E N T U E L L E M E N T ~~~|       
        //   |~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~|
        //   +--------------------oOO-------------+
        //                    |__|__|
        //                     || ||
        //                    ooO Ooo  

    }
}

void GLCD_Rectangle(INT8U XX1, INT8U YY1, INT8U XX2, INT8U YY2, INT8U Color) {
    // Rectangle vide
    GLCD_VerticalLine(XX1, YY1, XX2, Color);
    GLCD_VerticalLine(XX1, YY2, XX2, Color);
    GLCD_HorizontalLine(XX1, YY1, YY2, Color);
    GLCD_HorizontalLine(XX2, YY1, YY2, Color);
}

void GLCD_Box(INT8U XX1, INT8U YY1, INT8U XX2, INT8U YY2, INT8U Color) {
    // Rectangle plein
    INT8U YYdeb, YYfin; // Remise en ordre des YY1 et YY2
    INT8U iYY;

    YYdeb = ((YY1 < YY2) ? YY1 : YY2);
    YYfin = ((YY1 < YY2) ? YY2 : YY1);

    for (iYY = YYdeb; (iYY <= YYfin); iYY++) {
        GLCD_VerticalLine(XX1, iYY, XX2, Color);
    }
}

void GLCD_Circle(INT8U CentreX, INT8U CentreY, INT8U Rayon) {
    // Algorithme de trace de cercle d'Andres/Bresenham conseille !

    //                    |//
    //                   (o o)
    //   +---------oOO----(_)-----------------+
    //   |~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~|
    //   |~~~~  A   C O M P L E T E R   ! ~~~~|
    //   |~~~  E V E N T U E L L E M E N T ~~~|       
    //   |~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~|
    //   +--------------------oOO-------------+
    //                    |__|__|
    //                     || ||
    //                    ooO Ooo  

}


//================================================================   
//      Fonction de gestion des caracteres et des textes
//================================================================   

// Choix de la police

void GLCD_SetFont(ROM_INT8U *Police, INT8U Largeur, INT8U Hauteur, INT8U Espace, INT8U Offset) {
    Font = Police; // Police en cours d'utilisation
    FontHeight = Hauteur;
    FontWidth = Largeur;
    FontSpace = Espace;
    FontOffset = Offset;
    CP_Line = 0; // Position courante
    CP_Row = 0;
}

// Ecriture d'un texte

void GLCD_PrintAt(INT8U Ligne, INT8U Y, CHAR *Texte, INT8U Color) {
    CP_Line = Ligne;
    CP_Row = Y;
    while (*Texte) {
        GLCD_Chr(*Texte, Color); // Ecriture effective sur LCD
        Texte++; // Caractere suivant
    }
}

// Ecriture d'un texte en position courante

void GLCD_Print(CHAR *Texte, INT8U Color) {
    while (*Texte) {
        GLCD_Chr(*Texte, Color); // Ecriture effective sur LCD
        Texte++; // Caractere suivant
    }
}

//   #define GLCD_WriteAt(L,Y,T,C) GLCD_PrintAt(L,Y,T,C)
//   #define GLCD_Write(L,Y,T,C) GLCD_Print(L,Y,T,C)

// Ecriture d'un caractere 

void GLCD_ChrAt(INT8U Ligne, INT8U Y, CHAR Caractere, INT8U Color) {
    CP_Line = Ligne;
    CP_Row = Y;
    GLCD_Chr(Caractere, Color);
}

// Ecriture d'un caractere en position courante

void GLCD_Chr(CHAR Caractere, INT8U Color) {
    
    INT8U i,j, indice;
    indice = ( Caractere - '0' + 16 ) *  FontWidth ;
    for(i=0; i < FontHeight; i++)
    {
        for(j=0; j<FontWidth; j++)
        {
            if( ( Font[indice + i] >> j ) & 1 )
                GLCD_SetPixel( CP_Line + i, CP_Row + ( FontWidth - j ) );
        }
    }
}
