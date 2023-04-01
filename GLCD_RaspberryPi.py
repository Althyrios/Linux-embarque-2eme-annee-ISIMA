#!/usr/bin/env python

import RPi.GPIO as GPIO
import time
import sys 
import argparse

# Use 8-bit interface
BITS = 8

PIN_RST = 4
PIN_RS = 27
# PIN_RW = grounded; #ne changera pas d'état car on ne lira jamais l'état du registre
PIN_E = 22

PIN_CS1 = 17
PIN_CS2 = 18

# Define pins D0 through D7
PIN_D = [ 22, 23, 24, 25, 5, 6, 12, 13, 16 ]

PIN_NAMES = {
    PIN_RS: "rs",
    PIN_E: "e",
    PIN_D[0]: "d0",
    PIN_D[1]: "d1",
    PIN_D[2]: "d2",
    PIN_D[3]: "d3",
    PIN_D[4]: "d4",
    PIN_D[5]: "d5",
    PIN_D[6]: "d6",
    PIN_D[7]: "d7"
}

DELAY = 1.0e-3 # en secondes

#"Display Control" [0 0 1 1 1 1 1 D] :
GLCD_DISP_ON = 0b00111111  #Affichage en marche
GLCD_DISP_OFF = 0b00111110  #Desactivation de l'affichage

#"Set Y address" [0 1 A5 A4 A3 A2 A1 A0] : COLONNE
GLCD_SET_Y_ADDRESS = 0b01000000 #Y(5:0) sur 64
GLCD_SET_COLUMN = 0b01000000 #Column = Y address

#"Set X address" [1 0 1 1 1 A2 A1 A0] : PAGE
GLCD_SET_X_ADDRESS = 0b10111000 #X(2:0) sur 8
GLCD_SET_PAGE = 0b10111000 #Page = X address

#"Set Z address" [1 1 A5 A4 A3 A2 A1 A0] : START LINE
GLCD_SET_Z_ADDRESS = 0b11000000 #Z(5:0) sur 64
GLCD_SET_START_LINE = 0b11000000 #Start_Line = Z address

#Mots sur les donnees
#RS=1, RW=0 : Write Data
#RS=1, RW=1 : Read Data  



class GLCD(object):
    #Generation d'un front sur la broche GLCD_E (PORTB(4)) servant a la validation
    def GLCD_Pulse_E(self):
        GPIO.output(PIN_E, GPIO.HIGH)
        time.sleep(1)
        GPIO.output(PIN_E, GPIO.LOW)
    
    def GLCD_SendCmd(instruction):
        GPIO.output(PIN_RS, GPIO.LOW)
        #GPIO.output(PIN_RW, GPIO.LOW)
        GPIO.output(PIN_D, instruction)
        self.GLCD_Pulse_E()
    
    def GLCD_SendData(donnee):
        GPIO.output(PIN_RS, GPIO.HIGH)
        #GPIO.output(PIN_RW, GPIO.LOW)
        GPIO.output(PIN_D, donnee)
        self.GLCD_Pulse_E()

    #Initialisation du GLCD
    def __init__(self):
        GPIO.output(PIN_RS, GPIO.LOW) #init de RS à 0
        #GPIO.output(PIN_RW, GPIO.LOW) #init de R/W à 0 pas nécessaire car ne change jamais d'état
        GPIO.output(PIN_E, GPIO.LOW) #init de E à 0
        GPIO.output(PIN_RST, GPIO.LOW) #init de RST à 0

        time.sleep(15*DELAY)

        GPIO.output(PIN_CS1, GPIO.HIGH)
        GPIO.output(PIN_CS2, GPIO.HIGH)

        time.sleep(DELAY)

        GPIO.output(PIN_RST, GPIO.HIGH)

        time.sleep(DELAY)

        self.GLCD_SendCmd(GLCD_DISP_OFF)
        self.GLCD_SendCmd(GLCD_SET_START_LINE | 0x00)
        self.GLCD_SendCmd(GLCD_SET_PAGE | 0x00)
        self.GLCD_SendCmd(GLCD_SET_COLUMN | 0x00)
        self.GLCD_SendCmd(GLCD_DISP_ON)
        self.GLCD_ClrScreen()

    #Fonctions basiques : position et remplissage   
    #================================================================ 
    #Positionne en X et Y
    def GLCD_SetPositionXY(self, X_Page, Y_Column):
        self.GLCD_SendCmd(GLCD_SET_X_ADDRESS | X_Page)
        self.GLCD_SendCmd(GLCD_SET_Y_ADDRESS | Y_Column)

    #Remplissage total de l'ecran avec motif
    def GLCD_Fill(self, Sprite):

        #Remplissage des deux cotes a la fois
        GPIO.output(PIN_CS1, GPIO.HIGH)
        GPIO.output(PIN_CS2, GPIO.HIGH)
        for x in range(0,8):
            self.GLCD_SetPositionXY(x, 0x00)
            for y in range(0,64):
                self.GLCD_SendData(Sprite)
    
    #Effacement ecran
    def GLCD_ClrScreen(self):
        self.GLCD_Fill(0x00)


    #Dessine un pixel dans l'espace 64 x 128  (XX x YY)
    #================================================================ 
    def GLCD_SetPixel(self, XX, YY):
        x = XX >> 3 #XX / 8
        BitNbr = XX & 7 #XX % 8

        #Calcul de la position y (Colonne, gauche ou droite)
        if YY < 64:
            GPIO.output(PIN_CS1, GPIO.LOW)
            GPIO.output(PIN_CS2, GPIO.HIGH) #Partie gauche
            y = YY
        else:
            GPIO.output(PIN_CS1, GPIO.HIGH)
            GPIO.output(PIN_CS2, GPIO.LOW) #Partie droite
            y = YY - 64

        # Mise a 1 du bit demande(BitNbr) sur l'octet lu
        octet = (1 << BitNbr)

        #Ecriture effective de cet octet sur l'ecran  
        self.GLCD_SetPositionXY(x, y)
        self.GLCD_SendData(octet)

    def GLCD_ClrPixel(self, XX, YY):
        x = XX >> 3 #XX / 8
        BitNbr = XX & 7 #XX % 8

        #Calcul de la position y (Colonne, gauche ou droite)
        if YY < 64:
            GPIO.output(PIN_CS1, GPIO.LOW)
            GPIO.output(PIN_CS2, GPIO.HIGH) #Partie gauche
            y = YY
        else:
            GPIO.output(PIN_CS1, GPIO.HIGH)
            GPIO.output(PIN_CS2, GPIO.LOW) #Partie droite
            y = YY - 64

        # Mise a 1 du bit demande(BitNbr) sur l'octet lu
        octet = ~(1 << BitNbr)

        #Ecriture effective de cet octet sur l'ecran  
        self.GLCD_SetPositionXY(x, y)
        self.GLCD_SendData(octet)

    #Gestion des lignes
    #================================================================ 
    def GLCD_VerticalLine(self, XX1, YY, XX2, Pattern):
        if XX1 < XX2:
            XXdeb = XX1
            XXfin = XX2
        else:
            XXdeb = XX2
            XXfin = XX1
        
        for iXX in range(XXdeb,XXfin):
            if Pattern & (1 << (iXX & 7)):
                self.GLCD_SetPixel(iXX, YY)

    def GLCD_HorizontalLine(self, XX, YY1, YY2, Pattern):
        if YY1 < YY2:
            YYdeb = YY1
            YYfin = YY2
        else:
            YYdeb = YY1
            YYfin = YY2
        
        for iYY in range(YYdeb, YYfin):
            if Pattern & (1 << (iYY & 7)):
                self.GLCD_SetPixel(XX, iYY)
    
    def GLCD_DiagonalLine(self, XX1, XX2, YY1, YY2, Pattern):
        if XX1 < XX2:
            XXdeb = XX1
            XXfin = XX2
        else:
            XXdeb = XX2
            XXfin = XX1

        if YY1 < YY2:
            YYdeb = YY1
            YYfin = YY2
        else:
            YYdeb = YY1
            YYfin = YY2

        for iXX in range(XXdeb,XXfin):
            for iYY in range(YYdeb, YYfin):
                if (Pattern & (1 << (iXX & 7)) and Pattern & (1 << (iYY & 7))):
                    self.GLCD_SetPixel(iXX, iYY)

    def GLCD_Line(self, XX1, YY1, XX2, YY2, Color):
        if XX1 == XX2:
            self.GLCD_HorizontalLine(XX1, YY1, YY2, Color)
        else:
            if YY1 == YY2:
                self.GLCD_VerticalLine(XX1, YY1, XX2, Color)
            else:
                self.GLCD_DiagonalLine(XX1, XX2, YY1, YY2, Color)
    
    #Formes
    #================================================================
    #rectangle
    def GLCD_Rectangle(self, XX1, YY1, XX2, YY2, Color):
        self.GLCD_VerticalLine(XX1, YY1, XX2, Color)
        self.GLCD_VerticalLine(XX1, YY2, XX2, Color)
        self.GLCD_HorizontalLine(XX1, YY1, YY2, Color)
        self.GLCD_HorizontalLine(XX2, YY1, YY2, Color)

    #rectangle plein
    def GLCD_Box(self, XX1, YY1, XX2, YY2, Color):
        if YY1 < YY2:
            YYdeb = YY1
            YYfin = YY2
        else:
            YYdeb = YY2
            YYfin = YY1
        
        for iYY in range(YYdeb, YYfin):
            self.GLCD_VerticalLine(XX1, iYY, XX2, Color)


  

if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    parser.add_argument('--delay', '-d', type=float, default=1e-3, help="delay (in sec) between successive clocks")
    args = parser.parse_args()
    if args.bits != 8:
        raise Exception("--bits must be 8")
    else:
        BITS = args.bits
    DELAY = args.delay

    GPIO.setmode( GPIO.BCM )

    GPIO.setup([PIN_RS, PIN_EN] + PIN_D[0:8], GPIO.OUT, initial=GPIO.LOW )

    glcd = GLCD()

    try:
        j = 0
        while (j < 20):  
            glcd.GLCD_Fill(0x00)
            
            glcd.GLCD_VerticalLine(1, DECALAGE_X, 62, 0xEE)
            glcd.GLCD_HorizontalLine(62, DECALAGE_Y, DECALAGE_X + TAILLE, 0xEE)
            
            glcd.GLCD_VerticalLine(1, DECALAGE_X + TAILLE + 1, 62, 0xFF)
        
            for k in range(0,10):            
                glcd.GLCD_SetPixel( dernier, i + DECALAGE_X + 10 + k)

            j+=1
            time.sleep(200*DELAY)
            
    except:
        GPIO.cleanup()
        raise
    GPIO.cleanup()