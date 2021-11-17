#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <time.h>
#include <strings.h>
#include <ncurses.h>
#include <math.h>
#include <signal.h>
#include <string.h>
#include <ctype.h>
#include "main.h"
#if defined(__QNX__)
    #include <hw/pci.h>
    #include <hw/inout.h>
    #include <sys/neutrino.h>
    #include <sys/mman.h>															
    #define	INTERRUPT		iobase[1] + 0				// Badr1 + 0 : also ADC register
    #define	MUXCHAN			iobase[1] + 2				// Badr1 + 2
    #define	TRIGGER			iobase[1] + 4				// Badr1 + 4
    #define	AUTOCAL			iobase[1] + 6				// Badr1 + 6
    #define DA_CTLREG		iobase[1] + 8				// Badr1 + 8
    #define	AD_DATA			iobase[2] + 0				// Badr2 + 0
    #define	AD_FIFOCLR		iobase[2] + 2				// Badr2 + 2
    #define	TIMER0			iobase[3] + 0				// Badr3 + 0
    #define	TIMER1			iobase[3] + 1				// Badr3 + 1
    #define	TIMER2			iobase[3] + 2				// Badr3 + 2
    #define	COUNTCTL		iobase[3] + 3				// Badr3 + 3
    #define	DIO_PORTA		iobase[3] + 4				// Badr3 + 4
    #define	DIO_PORTB		iobase[3] + 5				// Badr3 + 5
    #define	DIO_PORTC		iobase[3] + 6				// Badr3 + 6
    #define	DIO_CTLREG		iobase[3] + 7				// Badr3 + 7
    #define	PACER1		    iobase[3] + 8				// Badr3 + 8
    #define	PACER2		    iobase[3] + 9				// Badr3 + 9
    #define	PACER3			iobase[3] + a				// Badr3 + a
    #define	PACERCTL		iobase[3] + b				// Badr3 + b
    #define DA_Data			iobase[4] + 0				// Badr4 + 0
    #define	DA_FIFOCLR		iobase[4] + 2				// Badr4 + 2
    uintptr_t iobase[6];
    #define DEBUG		1
#endif

int badr[5];	


void *daqthread(void *arg){
/* Function to read value from PCI-DAS1602/16 Board and Keyboard Input (WASD + Enter)
Includes :
Potentiometer value reading and conversion into amplitude
Toggle switch value reading and conversion to waveType
WASD Key reading to navigate the menu and to increase or decrease frequency */
	void *hdl;
	unsigned int i;
	int ch;

//**********************************************************************************************
// Setting up PCI-DAS1602/16 Board
//**********************************************************************************************   
#if defined(__QNX__)
		struct pci_dev_info info;
        memset(&info,0,sizeof(info));
        if(pci_attach(0)<0) {
            perror("pci_attach");
            exit(EXIT_FAILURE);}

        info.VendorId=0x1307;
        info.DeviceId=0x01;

        if ((hdl=pci_attach_device(0, PCI_SHARE|PCI_INIT_ALL, 0, &info))==0) {
            perror("pci_attach_device");
            exit(EXIT_FAILURE);}	

        for(i=0;i<5;i++) {
            badr[i]=PCI_IO_ADDR(info.CpuBaseAddress[i]);
            iobase[i]=mmap_device_io(0x0f,badr[i]);	}	
        
        if(ThreadCtl(_NTO_TCTL_IO,0)==-1) {
            perror("Thread Control");
            exit(1);}			
        
        out16(INTERRUPT,0x60c0);				    // sets interrupts	 - Clears
        out16(TRIGGER,0x2081);					// sets trigger control: 10MHz, clear, Burst off,SW trig. default:20a0
        out16(AUTOCAL,0x007f);					// sets automatic calibration : default

        out16(AD_FIFOCLR,0); 						// clear ADC buffer
        out16(MUXCHAN,0x0D00);          // previous group select 0x0C10
#endif    
    //Initial input
    inputs.generateWave = 0;
    inputs.currentInput = 1;
    
    while(1){
        // Take Mutex to update user inputs
        pthread_mutex_lock(&mutex);
        ch = getchar();
        if(ch == 115 || ch == 119){ //When W or S key is pressed
            if(inputs.currentInput == 1){ //Select either 'Generate Wave' or adjust frequency
                inputs.currentInput = 3;
            }
            else{
                inputs.currentInput = 1;
            }
        }
        else if(ch == 100 || ch == 97){  //When cursor is at frequency, read A or D key to adjust frequency
            if (inputs.currentInput==1){
                if(ch == 100 && inputs.freq<200){
                    inputs.freq += 1;
                } 

                else if(inputs.freq>3){
                    inputs.freq -= 1;
                }
            }
        }
        else if(ch==13 && inputs.currentInput == 3){ //Enter key to generate wave if the cursor is at generate wave button
            inputs.currentInput = 1;    
            inputs.generateWave = 1;
        }
        readswitch(&inputs); //Read toggle switch value to get waveType
        readpot(&inputs); //Read potentiometer value to get amplitude
        pthread_mutex_unlock(&mutex); //Unlock mutex
    } 	
}

void *keyboardthread(void *arg){
/* Function to capture input from Keyboard to navigate the main menu and change waveType, amplitude, and frequency value
W = UP
S = DOWN
A = DECREASE
D = INCREASE */
	int ch;
    inputs.generateWave = 0; //don't generate wave before Generate Wave button is pressed
    while(1){
        // Take Mutex to update user inputs
        pthread_mutex_lock(&mutex);
        ch = getchar();
        if(ch == 115){ //Move cursor up when W key is pressed      
            inputs.currentInput = (inputs.currentInput + 1) % 5;
        }
        else if(ch == 119){ //Move cursor down when S key is pressed
            if (inputs.currentInput == 0){
            	inputs.currentInput = 4;
            }
            else{
            	inputs.currentInput--;
            } 
        }
        else if(ch == 100 || ch == 97){  //When cursor is at waveType, read A or D key to adjust waveType
            if (inputs.currentInput==0){
                if(ch == 100){
                    inputs.waveType++;
                }
                else {
                    inputs.waveType--;
                }
                inputs.waveType = ((inputs.waveType - 1) % 4) + 1;
            }
            else if (inputs.currentInput==1){ //When cursor is at waveType, read A or D key to adjust frequency
                if(ch == 100 && inputs.freq<MAXIMUM_FREQUENCY){
                    inputs.freq += 1;
                } 

                else if(inputs.freq>MINIMUM_FREQUENCY){
                    inputs.freq -= 1;
                }
            }
            else if (inputs.currentInput==2){//When cursor is at waveType, read A or D key to adjust amplitude
                if(ch == 100 && inputs.amp<MAXIMUM_AMPLITUDE){
                    inputs.amp += 0.01;
                }
                else if(inputs.amp>MINIMUM_AMPLITUDE){
                    inputs.amp -= 0.01;
                }
            }
        }
        else if(ch==13 && inputs.currentInput == 3){  //Enter key to generate wave if the cursor is at generate wave button
            inputs.currentInput = 0;    
            inputs.generateWave = 1;
        }
        else if(ch==13 && inputs.currentInput == 4){ //Save and Exit when Enter is pressed if the cursor is at that button
            erase();
            save_config("config.txt", &inputs); //Call a function to save metronome setting to config.txt
            endwin(); //Close the ncurses window
            exit(0);// Exit the program
        }
        pthread_mutex_unlock(&mutex);
    } 	
}

void *outputthread(void *arg) {
/*Function for the user output which includes ncurses window as graphical display output and a wave generator*/
    // Declare wave attributes
    unsigned int data[10000];
    unsigned int i, n;
    float dummy, a;
    int cnt = 0;
    //Initializing board
     #if defined(__QNX__)
        void *hdl;
        struct pci_dev_info info;
        uintptr_t iobase[6];
        memset(&info,0,sizeof(info));
        if(pci_attach(0)<0) {perror("pci_attach");exit(EXIT_FAILURE);}
        info.VendorId=0x1307;
        info.DeviceId=0x01;
        if ((hdl=pci_attach_device(0, PCI_SHARE|PCI_INIT_ALL, 0, &info))==0) {perror("pci_attach_device");exit(EXIT_FAILURE);}		
        for(i=0;i<5;i++) {badr[i]=PCI_IO_ADDR(info.CpuBaseAddress[i]);iobase[i]=mmap_device_io(0x0f,badr[i]);	}	
        if(ThreadCtl(_NTO_TCTL_IO,0)==-1) {perror("Thread Control");exit(1);}			
    #endif	

    //Intialize ncurses
    startNcurses();

    while(1) {
        erase();
        // Print an arrow in the selected location
        if(inputs.currentInput==0){
            attron(A_STANDOUT);
            mvprintw(0,0,"> ");
            attroff(A_STANDOUT);
            mvprintw(1,0,"  ");
            mvprintw(2,0,"  ");
            mvprintw(4,0,"  ");
            mvprintw(6,0,"  ");
        }
        else if(inputs.currentInput==1){
            mvprintw(0,0,"  ");
            attron(A_STANDOUT);
            mvprintw(1,0,"> ");
            attroff(A_STANDOUT);
            mvprintw(2,0,"  ");
            mvprintw(4,0,"  ");
            mvprintw(6,0,"  ");
        }
        else if(inputs.currentInput==2){
            mvprintw(0,0,"  ");
            mvprintw(1,0,"  ");
            attron(A_STANDOUT);
            mvprintw(2,0,"> ");
            attroff(A_STANDOUT);
            mvprintw(4,0,"  ");
            mvprintw(6,0,"  ");
        }
        else if(inputs.currentInput==3){
            mvprintw(0,0,"  ");
            mvprintw(1,0,"  ");
            mvprintw(2,0,"  ");
            attron(A_STANDOUT);
            mvprintw(4,0,"> ");
            attroff(A_STANDOUT);
            mvprintw(6,0,"  ");
        }
        else{
            mvprintw(0,0,"  ");
            mvprintw(1,0,"  ");
            mvprintw(2,0,"  ");
            mvprintw(4,0,"  ");
            attron(A_STANDOUT);
            mvprintw(6,0,"> ");
            attroff(A_STANDOUT);
        }

        //printing of wave type and highlight when selected
        if (inputs.currentInput == 0){attron(A_STANDOUT);}
        mvprintw(0, 2, "WAVE TYPE :  %s", "   ");
        switch(inputs.waveType) {
            case 1: 
                mvprintw(0, 2, "WAVE TYPE :  %s", "Sine");
                break;
            case 2:
                mvprintw(0, 2, "WAVE TYPE :  %s", "Square");
                break;
            case 3:
                mvprintw(0, 2, "WAVE TYPE :  %s", "Sawtooth");
                break;
            case 4:
                mvprintw(0, 2, "WAVE TYPE :  %s", "Triangular");
                break;
            case -1:
                mvprintw(0, 2, "WAVE TYPE :  %s", "Invalid input for WAVETYPE! Re-enter!");
                break;
            default:
                mvprintw(0, 2, "WAVE TYPE :  NOT SPECIFIED");
        }
        attroff(A_STANDOUT);
        // printing of frequency and highlight when selected
        if (inputs.currentInput == 1){attron(A_STANDOUT);}
        move(1,12);
        clrtoeol();
        attron(COLOR_PAIR(3));
        mvprintw(1, 2, "FREQUENCY :  %.2lf Hz", inputs.freq);
        attroff(COLOR_PAIR(3));
        attroff(A_STANDOUT);
        // printing of amplitude
        if (inputs.currentInput == 2){attron(A_STANDOUT);}
        move(2,12);
        clrtoeol();
        attron(COLOR_PAIR(4));
        mvprintw(2, 2, "AMPLITUDE :  %.2lf V", inputs.amp);
        attroff(COLOR_PAIR(4));
        attroff(A_STANDOUT);

        // printing button and highlight when selected
        if (inputs.currentInput == 3){attron(A_STANDOUT);}
        if (inputs.generateWave == 0){
            move(4,12);
            clrtoeol();
            attron(COLOR_PAIR(2));
            mvprintw(4, 2, "Generate Wave");
            attroff(COLOR_PAIR(2));
            //mvprintw(5,0,"%d, %d\n",n, cnt);
        }
        //Calibrate the input values into appropiate amplitude and frequency to generate an accurate wave
        else{
            a = (inputs.amp*20/26);
            if(inputs.waveType == 1){
           		n = (int) ((100 * (1/inputs.freq))/0.077);
                for(i=0;i<n;i++) {
                    dummy = sineWave(i, n, a);
                    data[i]= (unsigned) dummy;
                }
            }
            else if(inputs.waveType ==2){
            	n = (int) ((100 * (1/inputs.freq))/0.004);
                for(i=0;i<n;i++) {
                    dummy = squareWave(i, n, a);
                    data[i]= (unsigned) dummy;
                }
            }
            else if(inputs.waveType ==3){
            	n = (int) ((100 * (1/inputs.freq))/0.004);
                for(i=0;i<n;i++) {
                    dummy = sawtoothWave(i, n, a);
                    data[i]= (unsigned) dummy;
                }
            }
            else if(inputs.waveType ==4){
            	n = (int) ((100 * (1/inputs.freq))/0.0045);
                for(i=0;i<n;i++) {
                    dummy = triangularWave(i, n, a);
                    data[i]= (unsigned) dummy;
                }
            }
            cnt = 0;
            inputs.generateWave = 0;           
        }
        attroff(A_STANDOUT);
        attron(COLOR_PAIR(1));
        if(inputs.currentInput == 4){attron(A_STANDOUT);} //Save and Exit Button and highlight when selected
        mvprintw(6, 2, "Save & Exit");
        attroff(A_STANDOUT);

        refresh();

        #if defined (__QNX__)
	      	out16(DA_CTLREG,0x0a23);			// DA Enable, #0, #1, SW 5V unipolar		2/6
   	        out16(DA_FIFOCLR, 0);					// Clear DA FIFO  buffer
   	        out16(DA_Data,(short) data[cnt]);																																		
   	        out16(DA_CTLREG,0x0a43);			// DA Enable, #1, #1, SW 5V unipolar		2/6
  	        out16(DA_FIFOCLR, 0);					// Clear DA FIFO  buffer
	        out16(DA_Data,(short) data[cnt]);	
	        cnt++;
	        if (cnt>=n){
	        	cnt = 0;
	        }																															
        #endif
    }
    endwin();
}

void readswitch(input *paramsptr){
/*A function to read toggle switch input and convert into the corresponding waveType
Input : A struct to store the waveTYpe*/
    int switchval;
    #if defined (__QNX__)
        switchval = in8(DIO_PORTA); //read the toggle switch state
    #endif
    switch (switchval) //convert the value into corresponding waveType and store in a struct
    {
    case 240: //output nothing when no switches are on
    paramsptr->waveType=0;
    break;
    case 241:
    paramsptr->waveType=1;
    break;
    case 242:
    paramsptr->waveType=2;
    break;
    case 244:
    paramsptr->waveType=3;
    break;
    case 248:
    paramsptr->waveType=4;
    default: //do nothing if more than one toggle switches are one
    break;}
}

void readpot(input *paramsptr ){
/* A function to read potentiometer value and convert into to wave amplitude
Input : A struct to store the amplitude*/
    unsigned int count;
    unsigned short chan;
    int potentioval;

    count = 0x00; // Use ADC0
    chan = ((count & 0x0f)<<4) | (0x0f & count);
    #if defined (__QNX__)
        out16(MUXCHAN,0x0D00|chan);
        delay(1);
        out16(AD_DATA,0);
        while(!(in16(MUXCHAN)&0x4000));
        potentioval = in16(AD_DATA); //read potentiometer value
    #endif
    paramsptr->amp=potentioval*MAXIMUM_AMPLITUDE/65535; //scale the potentiometer value into amplitude and store the amplitude value in a struct
}