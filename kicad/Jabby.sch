EESchema Schematic File Version 4
EELAYER 30 0
EELAYER END
$Descr A4 11693 8268
encoding utf-8
Sheet 1 1
Title "Jabby board"
Date "2022-02-25"
Rev "0"
Comp "vittorio benintende"
Comment1 ""
Comment2 ""
Comment3 ""
Comment4 ""
$EndDescr
$Comp
L Jabby-rescue:Mini360-dsn-mini-360 U2
U 1 1 620AABB6
P 3700 2800
F 0 "U2" H 3700 3287 60  0000 C CNN
F 1 "Mini360" H 3700 3181 60  0000 C CNN
F 2 "" H 3700 2800 60  0001 C CNN
F 3 "" H 3700 2800 60  0001 C CNN
	1    3700 2800
	1    0    0    -1  
$EndComp
$Comp
L Connector:Conn_01x04_Male J1
U 1 1 620AC0C4
P 1750 2350
F 0 "J1" H 1858 2631 50  0000 C CNN
F 1 "Conn_Ctrl_Pnl_01x04_Male" V 1650 2300 50  0000 C CNN
F 2 "" H 1750 2350 50  0001 C CNN
F 3 "~" H 1750 2350 50  0001 C CNN
	1    1750 2350
	0    1    1    0   
$EndComp
$Comp
L power:VCC #PWR01
U 1 1 620BE87F
P 1500 800
F 0 "#PWR01" H 1500 650 50  0001 C CNN
F 1 "VCC" H 1515 973 50  0000 C CNN
F 2 "" H 1500 800 50  0001 C CNN
F 3 "" H 1500 800 50  0001 C CNN
	1    1500 800 
	1    0    0    -1  
$EndComp
Text GLabel 900  1000 0    50   Input ~ 0
U
$Comp
L power:PWR_FLAG #FLG01
U 1 1 620C0DB5
P 1150 900
F 0 "#FLG01" H 1150 975 50  0001 C CNN
F 1 "PWR_FLAG" H 1150 1073 50  0000 C CNN
F 2 "" H 1150 900 50  0001 C CNN
F 3 "~" H 1150 900 50  0001 C CNN
	1    1150 900 
	1    0    0    -1  
$EndComp
Wire Wire Line
	1500 800  1500 1000
Wire Wire Line
	900  1000 1150 1000
Wire Wire Line
	1150 900  1150 1000
Connection ~ 1150 1000
Wire Wire Line
	1150 1000 1500 1000
Wire Wire Line
	1550 2550 1550 2900
Wire Wire Line
	1550 2900 3000 2900
Wire Wire Line
	1850 2700 1850 2600
Text GLabel 1650 2700 3    50   Input ~ 0
A
Text GLabel 1750 2700 3    50   Input ~ 0
B
Wire Wire Line
	1650 2550 1650 2700
Wire Wire Line
	1750 2550 1750 2700
$Comp
L power:GND #PWR02
U 1 1 620C65E6
P 1550 3050
F 0 "#PWR02" H 1550 2800 50  0001 C CNN
F 1 "GND" H 1555 2877 50  0000 C CNN
F 2 "" H 1550 3050 50  0001 C CNN
F 3 "" H 1550 3050 50  0001 C CNN
	1    1550 3050
	1    0    0    -1  
$EndComp
Text GLabel 1950 2600 2    50   Input ~ 0
U
Wire Wire Line
	1550 2900 1550 3050
Connection ~ 1550 2900
Wire Wire Line
	1850 2700 3100 2700
Wire Wire Line
	1950 2600 1850 2600
Connection ~ 1850 2600
Wire Wire Line
	1850 2600 1850 2550
Wire Wire Line
	4300 2700 4500 2700
$Comp
L power:+5V #PWR010
U 1 1 620CAEED
P 4500 2500
F 0 "#PWR010" H 4500 2350 50  0001 C CNN
F 1 "+5V" H 4515 2673 50  0000 C CNN
F 2 "" H 4500 2500 50  0001 C CNN
F 3 "" H 4500 2500 50  0001 C CNN
	1    4500 2500
	1    0    0    -1  
$EndComp
Wire Wire Line
	4500 2500 4500 2700
Connection ~ 4500 2700
Wire Wire Line
	4500 2700 4800 2700
$Comp
L Switch:SW_DIP_x01 SW1
U 1 1 620CCC57
P 5100 2700
F 0 "SW1" H 5100 2967 50  0000 C CNN
F 1 "SW_DIP_x01" H 5100 2876 50  0000 C CNN
F 2 "" H 5100 2700 50  0001 C CNN
F 3 "~" H 5100 2700 50  0001 C CNN
	1    5100 2700
	1    0    0    -1  
$EndComp
$Comp
L power:GND #PWR012
U 1 1 620CD700
P 6200 5950
F 0 "#PWR012" H 6200 5700 50  0001 C CNN
F 1 "GND" H 6205 5777 50  0000 C CNN
F 2 "" H 6200 5950 50  0001 C CNN
F 3 "" H 6200 5950 50  0001 C CNN
	1    6200 5950
	1    0    0    -1  
$EndComp
Wire Wire Line
	6200 5600 6200 5700
NoConn ~ 5800 4400
$Comp
L power:+3.3V #PWR013
U 1 1 620CEFB2
P 6300 3750
F 0 "#PWR013" H 6300 3600 50  0001 C CNN
F 1 "+3.3V" H 6315 3923 50  0000 C CNN
F 2 "" H 6300 3750 50  0001 C CNN
F 3 "" H 6300 3750 50  0001 C CNN
	1    6300 3750
	1    0    0    -1  
$EndComp
$Comp
L Connector:Conn_01x05_Male J2
U 1 1 620D1747
P 8750 4050
F 0 "J2" H 8800 4350 50  0000 R CNN
F 1 "Conn_GSM_01x05_Male" H 9150 4500 50  0000 R CNN
F 2 "" H 8750 4050 50  0001 C CNN
F 3 "~" H 8750 4050 50  0001 C CNN
	1    8750 4050
	-1   0    0    1   
$EndComp
$Comp
L Interface_UART:MAX485E U1
U 1 1 620DED9C
P 2900 4600
F 0 "U1" H 2700 3850 50  0000 C CNN
F 1 "MAX485E" H 2700 3950 50  0000 C CNN
F 2 "" H 2900 3900 50  0001 C CNN
F 3 "https://datasheets.maximintegrated.com/en/ds/MAX1487E-MAX491E.pdf" H 2900 4650 50  0001 C CNN
	1    2900 4600
	-1   0    0    1   
$EndComp
$Comp
L Device:R R1
U 1 1 620E28A7
P 3550 3800
F 0 "R1" H 3620 3846 50  0000 L CNN
F 1 "10K" H 3620 3755 50  0000 L CNN
F 2 "" V 3480 3800 50  0001 C CNN
F 3 "~" H 3550 3800 50  0001 C CNN
	1    3550 3800
	1    0    0    -1  
$EndComp
$Comp
L Device:R R2
U 1 1 620E2D4E
P 3800 3800
F 0 "R2" H 3870 3846 50  0000 L CNN
F 1 "10K" H 3870 3755 50  0000 L CNN
F 2 "" V 3730 3800 50  0001 C CNN
F 3 "~" H 3800 3800 50  0001 C CNN
	1    3800 3800
	1    0    0    -1  
$EndComp
$Comp
L Device:R R3
U 1 1 620E2FB6
P 4050 3800
F 0 "R3" H 4120 3846 50  0000 L CNN
F 1 "10k" H 4120 3755 50  0000 L CNN
F 2 "" V 3980 3800 50  0001 C CNN
F 3 "~" H 4050 3800 50  0001 C CNN
	1    4050 3800
	1    0    0    -1  
$EndComp
$Comp
L Device:R R4
U 1 1 620E30C7
P 4500 3800
F 0 "R4" H 4570 3846 50  0000 L CNN
F 1 "10K" H 4570 3755 50  0000 L CNN
F 2 "" V 4430 3800 50  0001 C CNN
F 3 "~" H 4500 3800 50  0001 C CNN
	1    4500 3800
	1    0    0    -1  
$EndComp
$Comp
L Device:C C1
U 1 1 620E34AB
P 4900 3800
F 0 "C1" H 4900 3900 50  0000 L CNN
F 1 "10u" H 4900 3700 50  0000 L CNN
F 2 "" H 4938 3650 50  0001 C CNN
F 3 "~" H 4900 3800 50  0001 C CNN
	1    4900 3800
	1    0    0    -1  
$EndComp
$Comp
L Device:C C2
U 1 1 620E3C89
P 5200 3800
F 0 "C2" H 5315 3846 50  0000 L CNN
F 1 "0.1u" H 5315 3755 50  0000 L CNN
F 2 "" H 5238 3650 50  0001 C CNN
F 3 "~" H 5200 3800 50  0001 C CNN
	1    5200 3800
	1    0    0    -1  
$EndComp
$Comp
L Device:R R5
U 1 1 620E6B33
P 5550 3800
F 0 "R5" H 5620 3846 50  0000 L CNN
F 1 "1K" H 5620 3755 50  0000 L CNN
F 2 "" V 5480 3800 50  0001 C CNN
F 3 "~" H 5550 3800 50  0001 C CNN
	1    5550 3800
	1    0    0    -1  
$EndComp
$Comp
L Device:LED D1
U 1 1 620E7152
P 5550 4100
F 0 "D1" V 5589 3982 50  0000 R CNN
F 1 "LED" V 5498 3982 50  0000 R CNN
F 2 "" H 5550 4100 50  0001 C CNN
F 3 "~" H 5550 4100 50  0001 C CNN
	1    5550 4100
	0    -1   -1   0   
$EndComp
Wire Wire Line
	4300 2900 4300 3250
Wire Wire Line
	4300 3250 3000 3250
Wire Wire Line
	3000 3250 3000 2900
Connection ~ 3000 2900
Wire Wire Line
	3000 2900 3100 2900
Wire Wire Line
	4500 2700 4500 3500
Wire Wire Line
	3550 3500 3550 3650
Wire Wire Line
	5550 3500 5550 3650
Wire Wire Line
	5200 3500 5200 3650
Connection ~ 5200 3500
Wire Wire Line
	5200 3500 5550 3500
Wire Wire Line
	4900 3500 4900 3650
Connection ~ 4900 3500
Wire Wire Line
	4900 3500 5200 3500
Wire Wire Line
	4050 3500 4050 3650
Connection ~ 4050 3500
Wire Wire Line
	4050 3500 3800 3500
Wire Wire Line
	3800 3500 3800 3650
Connection ~ 3800 3500
Wire Wire Line
	3800 3500 3550 3500
$Comp
L power:GND #PWR011
U 1 1 620F47BB
P 5200 4400
F 0 "#PWR011" H 5200 4150 50  0001 C CNN
F 1 "GND" H 5205 4227 50  0000 C CNN
F 2 "" H 5200 4400 50  0001 C CNN
F 3 "" H 5200 4400 50  0001 C CNN
	1    5200 4400
	1    0    0    -1  
$EndComp
Wire Wire Line
	4900 3950 4900 4250
Wire Wire Line
	4900 4250 5200 4250
Wire Wire Line
	5200 3950 5200 4250
Connection ~ 5200 4250
Wire Wire Line
	5200 4250 5550 4250
Wire Wire Line
	5200 4250 5200 4400
$Comp
L MCU_Module:WeMos_D1_mini U3
U 1 1 620A9F39
P 6200 4800
F 0 "U3" H 5750 4000 50  0000 C CNN
F 1 "WeMos_D1_mini" H 5750 3900 50  0000 C CNN
F 2 "Module:WEMOS_D1_mini_light" H 6200 3650 50  0001 C CNN
F 3 "https://wiki.wemos.cc/products:d1:d1_mini#documentation" H 4350 3650 50  0001 C CNN
	1    6200 4800
	1    0    0    -1  
$EndComp
Wire Wire Line
	6100 2700 6100 4000
Text GLabel 2050 4700 0    50   Input ~ 0
B
Text GLabel 2050 4400 0    50   Input ~ 0
A
Wire Wire Line
	3550 3950 3550 4400
Wire Wire Line
	3550 4400 3400 4400
Wire Wire Line
	3300 4500 3800 4500
Wire Wire Line
	3800 4500 3800 3950
Wire Wire Line
	4050 3950 4050 4600
Wire Wire Line
	4050 4600 3800 4600
$Comp
L power:+3.3V #PWR08
U 1 1 621088A1
P 3450 5150
F 0 "#PWR08" H 3450 5000 50  0001 C CNN
F 1 "+3.3V" H 3465 5323 50  0000 C CNN
F 2 "" H 3450 5150 50  0001 C CNN
F 3 "" H 3450 5150 50  0001 C CNN
	1    3450 5150
	1    0    0    -1  
$EndComp
$Comp
L power:GND #PWR06
U 1 1 62108E9E
P 2450 3850
F 0 "#PWR06" H 2450 3600 50  0001 C CNN
F 1 "GND" H 2455 3677 50  0000 C CNN
F 2 "" H 2450 3850 50  0001 C CNN
F 3 "" H 2450 3850 50  0001 C CNN
	1    2450 3850
	1    0    0    -1  
$EndComp
Wire Wire Line
	2900 5100 2900 5350
Wire Wire Line
	2900 5350 3450 5350
Wire Wire Line
	3450 5350 3450 5150
Wire Wire Line
	2900 4000 2900 3700
Wire Wire Line
	2900 3700 2450 3700
Wire Wire Line
	2450 3700 2450 3850
Wire Wire Line
	3400 4400 3400 3500
Wire Wire Line
	3400 3500 1550 3500
Wire Wire Line
	1550 3500 1550 5500
Wire Wire Line
	1550 5500 5550 5500
Wire Wire Line
	5550 5500 5550 4800
Wire Wire Line
	5550 4800 5800 4800
Connection ~ 3400 4400
Wire Wire Line
	3400 4400 3300 4400
Wire Wire Line
	3800 4500 3800 4600
Connection ~ 3800 4500
Connection ~ 3800 4600
Wire Wire Line
	3800 4600 3300 4600
Wire Wire Line
	3300 4700 4500 4700
Wire Wire Line
	4050 3500 4500 3500
Wire Wire Line
	4500 3650 4500 3500
Connection ~ 4500 3500
Wire Wire Line
	4500 3950 4500 4700
Connection ~ 4500 4700
Wire Wire Line
	4500 4700 5800 4700
Text GLabel 4200 4600 2    50   Input ~ 0
D7
Wire Wire Line
	4200 4600 4050 4600
Connection ~ 4050 4600
Wire Wire Line
	4500 3500 4900 3500
Text GLabel 6850 5100 2    50   Input ~ 0
D7
Wire Wire Line
	2050 4400 2500 4400
Wire Wire Line
	2050 4700 2500 4700
Wire Wire Line
	6300 3750 6300 4000
$Comp
L power:PWR_FLAG #FLG0101
U 1 1 621286B3
P 6100 2500
F 0 "#FLG0101" H 6100 2575 50  0001 C CNN
F 1 "PWR_FLAG" H 6100 2673 50  0000 C CNN
F 2 "" H 6100 2500 50  0001 C CNN
F 3 "~" H 6100 2500 50  0001 C CNN
	1    6100 2500
	1    0    0    -1  
$EndComp
Connection ~ 6100 2700
Wire Wire Line
	6100 2500 6100 2700
Wire Wire Line
	6700 3850 6700 2700
Wire Wire Line
	6700 2700 6100 2700
$Comp
L power:GND #PWR03
U 1 1 62130829
P 6950 4050
F 0 "#PWR03" H 6950 3800 50  0001 C CNN
F 1 "GND" H 6955 3877 50  0000 C CNN
F 2 "" H 6950 4050 50  0001 C CNN
F 3 "" H 6950 4050 50  0001 C CNN
	1    6950 4050
	1    0    0    -1  
$EndComp
Wire Wire Line
	5400 2700 6100 2700
NoConn ~ 6600 4300
NoConn ~ 6600 4400
NoConn ~ 6600 4700
Wire Wire Line
	6850 5100 6600 5100
$Comp
L Transistor_BJT:2N2219 Q1
U 1 1 620EB96B
P 7800 4800
F 0 "Q1" H 7990 4846 50  0000 L CNN
F 1 "2N2222" H 7990 4755 50  0000 L CNN
F 2 "Package_TO_SOT_THT:TO-39-3" H 8000 4725 50  0001 L CIN
F 3 "http://www.onsemi.com/pub_link/Collateral/2N2219-D.PDF" H 7800 4800 50  0001 L CNN
	1    7800 4800
	1    0    0    -1  
$EndComp
$Comp
L Device:R R6
U 1 1 620ED3FA
P 7450 4800
F 0 "R6" V 7550 4850 50  0000 L CNN
F 1 "4.7K" V 7550 4700 50  0000 L CNN
F 2 "" V 7380 4800 50  0001 C CNN
F 3 "~" H 7450 4800 50  0001 C CNN
	1    7450 4800
	0    1    1    0   
$EndComp
Wire Wire Line
	6700 3850 8550 3850
$Comp
L power:GND #PWR04
U 1 1 620F49EB
P 7900 5000
F 0 "#PWR04" H 7900 4750 50  0001 C CNN
F 1 "GND" H 7905 4827 50  0000 C CNN
F 2 "" H 7900 5000 50  0001 C CNN
F 3 "" H 7900 5000 50  0001 C CNN
	1    7900 5000
	1    0    0    -1  
$EndComp
Wire Wire Line
	6950 3950 6950 4050
Wire Wire Line
	6950 3950 8550 3950
Wire Wire Line
	6600 4500 7400 4500
Wire Wire Line
	7400 4500 7400 4150
Wire Wire Line
	7400 4150 8550 4150
Wire Wire Line
	7500 4250 7500 4600
Wire Wire Line
	7500 4600 6600 4600
Wire Wire Line
	7500 4250 8550 4250
Wire Wire Line
	7900 4600 7900 4050
Wire Wire Line
	7900 4050 8550 4050
$Comp
L Connector:Conn_01x02_Male J3
U 1 1 62104B7C
P 8750 5500
F 0 "J3" H 8900 5600 50  0000 R CNN
F 1 "Conn_Device_01x02_Male" H 9250 5700 50  0000 R CNN
F 2 "" H 8750 5500 50  0001 C CNN
F 3 "~" H 8750 5500 50  0001 C CNN
	1    8750 5500
	-1   0    0    1   
$EndComp
Wire Wire Line
	6600 4900 7150 4900
Wire Wire Line
	7150 4900 7150 5400
Wire Wire Line
	7150 5400 8550 5400
Wire Wire Line
	8550 5500 7150 5500
Wire Wire Line
	7150 5500 7150 5700
Wire Wire Line
	7150 5700 6200 5700
Connection ~ 6200 5700
Wire Wire Line
	6200 5700 6200 5950
NoConn ~ 6600 5200
Text Label 8850 3850 0    50   ~ 0
+
Text Label 8850 3950 0    50   ~ 0
-
Text Label 8850 4050 0    50   ~ 0
RST
Text Label 8850 4250 0    50   ~ 0
RX
Text Label 8850 4150 0    50   ~ 0
TX
Text Label 8850 5500 0    50   ~ 0
COM
Text Label 8850 5400 0    50   Italic 0
XX
NoConn ~ 6600 4800
Wire Wire Line
	6600 5000 6900 5000
Wire Wire Line
	6900 5000 6900 4800
Wire Wire Line
	6900 4800 7300 4800
$EndSCHEMATC
