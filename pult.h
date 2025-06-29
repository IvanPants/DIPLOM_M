#ifndef PULT_H
#define PULT_H


#define APP_NAME "pult"

// Тестовая программа

//#define TEST                    1


// Адреса имитаторов на ModBus

#define BU_ADDR                 1

#define IMM_DMAS_ADDR           44

#define IMM_RS485_1_ADDR        60
#define IMM_RS485_2_ADDR        64
#define IMM_RS485_3_ADDR        62

#define IMM_OP_1_ADDR           70
#define IMM_OP_2_ADDR           71

#define IMM_BLK_ADDR            10

#define IMM_PZU_ADDR            101

// Адреса ПК-ИЦМ

#define DMAS2_START_ADDR        1
#define DMAS6_START_ADDR        4
#define DMAS2_DELAY_ADDR        7
#define DMAS6_DELAY_ADDR        8

#define DMAS2_ENABLE_ADDR       1
#define DMAS6_ENABLE_ADDR       2

#define INPUT_SWITCH_ADDR       9

// Адреса МАС


#define MAS_NUMBER_ADDR         1
#define LK_NUMBER_ADDR          2


#define ADDR_LK_ADDR            3
#define BUFFER_SIZE_ADDR        4
#define ADDER_ADDR              5
#define CONST1_ADDR             6
#define CONST2_ADDR             7

#define DURATION_ADDR           8

#ifndef OLD_TK170

#define REGIME_ADDR             9
#define SF_ADDR_ADDR            10
#define SF_INF1_ADDR            11
#define SF_INF2_ADDR            12
#define SF_MAC1_RESULT          13
#define SF_MAC2_RESULT          14
#define SF_MAC3_RESULT          15
#define SF_MAC4_RESULT          16

#define BUFFER_ADDR_OP          17
#define BUFFER2_ADDR_OP         18

#else

#define REGIME_ADDR             9
#define SF_ADDR_ADDR            10
#define SF_INF1_ADDR            11
#define SF_INF2_ADDR            12
#define SF_MAC1_RESULT          13
#define SF_MAC2_RESULT          14
#define SF_MAC3_RESULT          15
#define SF_MAC4_RESULT          16
#endif


#define BUFFER_ADDR_RS          9
#define BUFFER2_ADDR_RS         10



// Адреса БЛК

#define BLK_ON_OFF_ADDR         4
#define BLK_SELECT_ADDR         0
#define BLK_SENSOR_SET_ADDR     0


// Адреса Блок упавления

#define BU_KADR_TYPE_ADDR        20


#define KADR_RTSC               0
#define KADR_RTSCM              1
#define KADR_VAAR               2
#define KADR_RTSCM1             3
#endif
