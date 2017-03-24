#include "mbed.h"
#include "rtos.h"
#include <stdlib.h>
#include "math.h"
#include "RawSerial.h"
#include "ctype.h"
#include "string"

//-----------------Define States for the FSM------------------------------
typedef enum {
    STATE0 = 0, STATE1, STATE2, STATE3, STATE4,
} STATE;
//------------------Functions to input command from Putty----------------------
void Puttyinput();

float R_target = 0;
float V_target = 0;
RawSerial pc(USBTX, USBRX);
double R_inv = 1;
double R_int = 0, R_dec = 0, R = 0;
double i = 0, i_dec = 0;
double V_inv = 1;
double inv = 1;
double V_int = 0, V_dec = 0, V = 0;;
double j = 0, j_dec = 0;
int mode = 0;
char ch;

string notes[16];
int duration[16];
int count = -1;
void getmelody();
void getfrq(int count);

float u_period[16];


void Puttyinput(){
    STATE current_state = STATE0;
    int indicator =1;
    while (1) {
        printf("Input command: \n\r");
        while ((ch = pc.getc()) != '\r') {
            pc.putc(ch);
            if(ch == 'T')
            {
                getmelody();
                indicator = 2;
                break;
            }
            switch (current_state) {
                case STATE0:
                    if (ch == 'R')
                        current_state = STATE1;
                    else if (ch == 'V')
                        current_state = STATE3;
                    break;
                case STATE1:
                    if (ch == '-') {
                        current_state = STATE1;
                        R_inv = -1;
                    } else if (ch == '.')
                        current_state = STATE2;
                    else if (ch == 'V')
                        current_state = STATE3;
                    else if (isdigit(ch)){
                        i++;
                        current_state = STATE1;
                        R_int = R_int*10 + (double)int(ch - 48);
                    }
                    break;
                case STATE2:
                    if (ch == 'V')
                        current_state = STATE3;
                    else if (isdigit(ch)) {
                        i_dec++;
                        current_state = STATE2;
                        if(i_dec == 1){
                            R_dec = R_dec + (double)int(ch-48)/10;
                        }
                        if(i_dec == 2){
                            R_dec = R_dec +(double)int(ch-48)/100;
                        }
                    }
                    break;
                case STATE3:
                    if (ch == '-') {
                        current_state = STATE3;
                        V_inv = -1;
                    } else if (ch == '.')
                        current_state = STATE4;
                    else if (isdigit(ch)) {
                        j++;
                        current_state = STATE3;
                        V_int = V_int*10 + (double)int(ch-48);
                    }
                    break;
                case STATE4:
                    if (isdigit(ch)) {
                        j_dec++;
                        current_state = STATE4;
                        if (j_dec == 1) {
                            V_dec = V_dec + (double) int(ch - 48) / 10;
                        }
                        if(j_dec == 2){
                            V_dec = V_dec + (double) int(ch - 48) / 100;
                        }
                    }
                    break;
                default:
                    current_state = STATE0;
                    break;
            }
        }   //end inner while
        if(indicator == 1){
            R = R_inv * (R_int + R_dec);
            V = V_inv * (V_int + V_dec);
            
            if((R_inv==-1.0) && (V_inv==1.0)) inv =-1.0;
            if((R_inv==1.0) && (V_inv==1.0))  inv =1.0;
            if((R_inv==1.0) && (V_inv==-1.0)) inv =-1.0;
            
            if(R!=0 && V!= 0) mode = 3;
            if(R!=0 && V==0) mode=2;
            if(R==0 && V!=0) mode=1;
            
            //pc.printf("R: %f\n\r",R);
            //pc.printf("V: %f\n\r",V);
            //pc.printf("reverse: %d\n\r",inv);
            pc.printf("Mode is: %d\n\r",mode);
        }
        
        break;
    }
    R_target = (float)R;
    V_target = (float)V;
}
int size = 0;
void getmelody(){
    char ch;
    while((ch = getchar()) != '\r'){
        pc.putc(ch);
        if(ch >= 'A' && ch <= 'G'){
            count++;
            notes[count] = ch;
        }
        if((ch == '#') || (ch == '^')){
            notes[count] = notes[count] + ch;
        }
        if(isdigit(ch)){
            duration[count] = int(ch-48);
        }
    }
    
    getfrq(count);
}

void getfrq(int count){
    double frq;
    for(int i = 0; i <= count; i++){
        if(notes[i] == "A") {frq= 3520;}
        if(notes[i] == "A#") {frq = 3729.31;}
        if(notes[i] == "A^") {frq = 3322.44;}
        if(notes[i] == "B") frq = 3951.07;
        if(notes[i] == "B^") frq = 3729.31;
        if(notes[i] == "C") frq = 2093;
        if(notes[i] == "C#") frq = 2217.46;
        if(notes[i] == "D") frq = 2349.32;
        if (notes[i] == "D#") frq = 2489.02;
        if (notes[i] == "D^") frq = 2217.46;
        if(notes[i] == "E") frq = 2637.02;
        if (notes[i] == "E^") frq = 2489.02;
        if(notes[i] == "F") frq = 2793.83;
        if(notes[i] == "F#") frq = 2959.96;
        if(notes[i] == "G") frq = 3135.96;
        if(notes[i] == "G#") frq = 3322.44;
        if(notes[i] == "G^") frq = 2959.96;
        
        u_period[i] = (1.0f/(float)frq)* 1000000.0f;
        size = i;
    }
}

//Photointerrupter input pins
#define I1pin D2
#define I2pin D11
#define I3pin D12

//Incremental encoder input pins
#define CHA   D7
#define CHB   D8

//Motor Drive output pins   //Mask in output byte
#define L1Lpin D4           //0x01
#define L1Hpin D5           //0x02
#define L2Lpin D3           //0x04
#define L2Hpin D6           //0x08
#define L3Lpin D9           //0x10
#define L3Hpin D10          //0x20

#define MEASUREPIN D1
DigitalOut measure(MEASUREPIN);

//-----------------Define all the variables and constant------------------
float duty_cycle = 1.0;
int8_t intState = 0;
int8_t orState = 0;
Thread thread;

//Drive state to output table
const int8_t driveTable[] = {0x12,0x18,0x09,0x21,0x24,0x06,0x00,0x00};

//Mapping from interrupter inputs to sequential rotor states. 0x00 and 0x07 are not valid
const int8_t stateMap[] = {0x07,0x05,0x03,0x04,0x01,0x00,0x02,0x07};
//const int8_t stateMap[] = {0x07,0x01,0x03,0x02,0x05,0x00,0x04,0x07}; //Alternative if phase order of input or drive is reversed

//Phase lead to make motor spin
int8_t lead = -2*(int)inv;;  //2 for forwards, -2 for backwards

//Status LED
DigitalOut led1(LED1);

//------------------------Photointerrupter inputs----------------------------
InterruptIn I1(I1pin);
InterruptIn I2(I2pin);
InterruptIn I3(I3pin);


//-----------------------Define the relative position indicator CHA&CHB-------
InterruptIn precise1(CHA);
InterruptIn precise2(CHB);

//-------------------------Motor Transistors Pins----------------------------
//Motor Drive outputs
PwmOut L1L(L1Lpin); //L1L is PWM
DigitalOut L1H(L1Hpin);
PwmOut L2L(L2Lpin); //L2L is PWM
DigitalOut L2H(L2Hpin);
PwmOut L3L(L3Lpin); //L3L is PWM
DigitalOut L3H(L3Hpin);

int period = 100;
//---------------------Set a given drive state-----------------------
void motorOut(int8_t driveState){
    //Lookup the output byte from the drive state.
    int8_t driveOut = driveTable[driveState & 0x07];
    L1L.period_us(period);
    L2L.period_us(period);
    L3L.period_us(period);
    //Turn off first
    if (~driveOut & 0x01) L1L = 0;//.write(0.00f);//driveout第0位
    if (~driveOut & 0x02) L1H = 1;         //driveout第1位
    if (~driveOut & 0x04) L2L = 0;//.write(0.00f);//driveout第2位
    if (~driveOut & 0x08) L2H = 1;         //driveout第3位
    if (~driveOut & 0x10) L3L = 0;//.write(0.00f);//driveout第4位
    if (~driveOut & 0x20) L3H = 1;         //driveout第5位
    
    //Then turn on
    if (driveOut & 0x01) L1L.write(duty_cycle);
    if (driveOut & 0x02) L1H = 0;
    if (driveOut & 0x04) L2L.write(duty_cycle);
    if (driveOut & 0x08) L2H = 0;
    if (driveOut & 0x10) L3L.write(duty_cycle);
    if (driveOut & 0x20) L3H = 0;
}

//-----------------Convert photointerrupter inputs to a rotor state-----------------
inline int8_t readRotorState(){
    return stateMap[I1 + 2*I2 + 4*I3];
}

//----------------------Basic synchronisation routine--------------------------------
int8_t motorHome() {
    //Put the motor in drive state 0 and wait for it to stabilise
    motorOut(0);
    wait(2.0);
    
    //Get the rotor state
    return readRotorState();
}

Timer t1;
Timer t2;
Timer t3;
Timer t4;
Timer t5;
Timer t6;
Timer t;
Timer T;

int counter = 0;
float R_now = 0;
float velocity = 0;
float V_need = 0;
float interval = 0;
float velocity_tmp = 0;
float V_last = 0;
float integral = 0;

//--------------CONTROL as a thread-----------------
void control(){
    while(true){
        Thread::signal_wait(0x1);
        V_need = 1.7f + 0.25f*(abs(R_target) - R_now)+ 0.013f*(-velocity);
        if(V_need > abs(V_target)){V_need = abs(V_target);}
        duty_cycle = 0.4f*(V_need - velocity);
        if(R_now >= abs(R_target)){duty_cycle = 0;}
    }
}

void precisecontrol(){
    counter ++;
    t.stop();
    interval = t.read();
    velocity_tmp = (1.0f/interval)/117.0f;
    integral = integral + (V_last-velocity_tmp)*interval;
    duty_cycle  = 50.0f*(abs(V_target)-velocity_tmp) + 0.35f*(V_last-velocity_tmp)/interval + 0.5f*integral;
    V_last = velocity_tmp;
    if (duty_cycle < 0.0f){
        duty_cycle = 0.10f;
        lead = 2*(int)inv;
    }
    else {lead = -2*(int)inv;}
    t.reset();
    t.start();
    if (counter%117 == 0){
        T.stop();
        velocity = 1.0f/T.read();
        T.reset();
        T.start();
    }
    if (((float)counter)/117.0f >= (1.0f-0.01f)*abs(R_target)){
        duty_cycle = 0;
    }
    R_now =((float)counter)/117.0f;
    
    intState = readRotorState();
    motorOut((intState-orState+lead+6)%6);
}
//---------------MotorOut Equvalence Interrupt----------------------
void update_motorstate1(){
    R_now ++;
    t1.stop();
    velocity = 1.0f/t1.read();
    intState = readRotorState();
    motorOut((intState-orState+lead+6)%6);
    t1.reset();
    thread.signal_set(0x1);
    t1.start();
}
void update_motorstate2(){
    t2.stop();
    velocity = 1.0f/t2.read();
    intState = readRotorState();
    motorOut((intState-orState+lead+6)%6);
    t2.reset();
    thread.signal_set(0x1);
    t2.start();
}
void update_motorstate3(){
    t3.stop();
    velocity = 1.0f/t3.read();
    intState = readRotorState();
    motorOut((intState-orState+lead+6)%6);
    t3.reset();
    thread.signal_set(0x1);
    t3.start();
}
void update_motorstate4(){
    t4.stop();
    velocity = 1.0f/t4.read();
    intState = readRotorState();
    motorOut((intState-orState+lead+6)%6);
    t4.reset();
    thread.signal_set(0x1);
    t4.start();
}
void update_motorstate5(){
    t5.stop();
    velocity = 1.0f/t5.read();
    intState = readRotorState();
    motorOut((intState-orState+lead+6)%6);
    t5.reset();
    thread.signal_set(0x1);
    t5.start();
}
void update_motorstate6(){
    t6.stop();
    velocity = 1.0f/t6.read();
    intState = readRotorState();
    motorOut((intState-orState+lead+6)%6);
    t6.reset();
    thread.signal_set(0x1);
    t6.start();
}

int main(){
    Puttyinput();
    if ((R_target == 0)&&(V_target == 0)){
        duty_cycle = 0.5;
        while(1){
            for(int i = 0; i<=count; i++){
                period = (int)u_period[i];
                motorOut(0);
                wait(float(duration[i]));
                pc.printf("Period: %d\n\r", (int)u_period[i]);
                pc.printf("Duration: %f\n\r", float(duration[i]));
            }
        }
    }
    else{
        if(R_target == 0.0f){R_target = 1.0f/0.0f;}
        if(V_target == 0.0f){V_target = 5.0f;}
        pc.printf("PDD Rotates\n\r");
        pc.printf("R_target =: %f\n\r",R_target);
        pc.printf("V_target =: %f\n\r",V_target);
        pc.printf("Hello\n\r");
        pc.printf("Wait for 3 seconds\n\r");
        wait(3.0);
        
        lead = (int)inv*-2;
        
        orState = motorHome();
        pc.printf("Rotor origin: %x\n\r",orState);
        
        t1.start();
        t2.start();
        t3.start();
        t4.start();
        t5.start();
        t6.start();
        
        t.start();
        T.start();
        
        if(abs(V_target) >= 4.0f){
            I3.rise(&update_motorstate1);
            I3.fall(&update_motorstate2);
            I2.rise(&update_motorstate3);
            I2.fall(&update_motorstate4);
            I1.rise(&update_motorstate5);
            I1.fall(&update_motorstate6);
        }
        else{
            precise1.rise(&precisecontrol);
        }
        thread.start(control);
        duty_cycle = 1.0;
        intState = readRotorState();
        motorOut((intState-orState+lead+6)%6);
        while(1){
            //measure =1;
            //measure =0;
            wait(1.0);
            pc.printf("Revolution: %f\n\r",R_now);
            pc.printf("Speed: %f\n\r",velocity);
            pc.printf("Duty Cycle: %f\n\r",duty_cycle);
        }
    }
}
