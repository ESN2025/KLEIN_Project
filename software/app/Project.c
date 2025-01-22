#include "sys/alt_stdio.h"
#include "system.h"

//I2C include
#include "opencores_i2c.h"
//TIMER include
#include "altera_avalon_timer_regs.h"
#include "altera_avalon_timer.h"
//IRQ include
#include "sys/alt_irq.h"
#include "sys/alt_sys_init.h"
//PIO include
#include "altera_avalon_pio_regs.h"


//VARIABLES
int sampleOffset;
int ledId;


//UTILITY FUNCTION
alt_u32 convertDataToDisplayFormat(int value){
	
	//INFO
	alt_u32 info;
	if(sampleOffset==0){ //ADD X, Y, Z AND =
		info = (2 << 6) + (1 << 3);
	}else if(sampleOffset==2){
		info = (3 << 6) + (1 << 3);
	}else{
		info = (4 << 6) + (1 << 3);
	}
	int ledFactor = 1;
	if(value & 0x200){ //ADD MINUS
		info +=5;
		value = ~(value);
		value+=1;
		value &= 0x1FF;
		ledFactor = -1;
	}
	
	//TO G
	value *=4; //to G
	value /=10; //2 chiffre significatif
	
	//LED
	if(ledFactor==-1){
		ledId=1 << ((value/20)+4);
	}else{
		ledId=0x10 >> ((value/20));
	}
	

	
	//BCD
	alt_u32 shift = 0;
	alt_u32 data = 0;
	while (value > 0) {
		data += (value % 10) << shift;
		shift += 5;
		value = value / 10;
	}
	return (info << 15) + data + (1 << 14);
}



//I2C FUNCTION
void AccWrite(int registerAdd, int value){
	I2C_start(I2C_BASE,0x1D,0);
	I2C_write(I2C_BASE,registerAdd,0);
	I2C_write(I2C_BASE,value,1);
	return;
}

alt_u32 AccRead(int registerAdd){
	I2C_start(I2C_BASE,0x1D,0);
	I2C_write(I2C_BASE,registerAdd,0);
	I2C_start(I2C_BASE,0x1D,1);
	return I2C_read(I2C_BASE,1);
}

alt_u32 AccReadData(int registerAdd){
	I2C_start(I2C_BASE,0x1D,0);
	I2C_write(I2C_BASE,registerAdd,0);
	I2C_start(I2C_BASE,0x1D,1);
	alt_u32 data1 = I2C_read(I2C_BASE,0);
	alt_u32 data2 = I2C_read(I2C_BASE,1);
	return ((data2 << 2) + (data1 >> 6));
}



//READ REGISTER FUNCTION
void calibrateAcc(){
	//ID
	alt_printf("\nID : %x",AccRead(0x00));
	//POWER CTL
	AccWrite(0x2D,0x8);
	alt_printf("\nPWR : %x",AccRead(0x2D));
	//FIFO
	AccWrite(0x38,0x80);
	alt_printf("\nFIFO : %x",AccRead(0x38));
	//DATA FORMAT
	//AccWrite(0x31,0x0);
	AccWrite(0x31,0x4);
	alt_printf("\nDATA FORMAT : %x",AccRead(0x31));
	//CALIBRAGE
	AccWrite(0x1E,0x03);
	alt_printf("\nX_OFFSET : %x",AccRead(0x1E));
	AccWrite(0x1F,0x03);
	alt_printf("\nY_OFFSET : %x",AccRead(0x1F));
	AccWrite(0x20,0x02);
	alt_printf("\nZ_OFFSET : %x",AccRead(0x20));
	return;
}



//IRQ FUNCTION
static void readSample(void * context, alt_u32 id){
	alt_u32 value = AccReadData(0x32 + sampleOffset);
	IOWR_ALTERA_AVALON_PIO_DATA(PIO_OUT_DATA_BASE,convertDataToDisplayFormat(value));
	IOWR_ALTERA_AVALON_PIO_DATA(PIO_OUT_LED_BASE,ledId);
	//clear
	IOWR_ALTERA_AVALON_TIMER_STATUS(TIMER_BASE,0);
}

static void changeSampleOffset(void * context, alt_u32 id){
	if(sampleOffset==4){
		sampleOffset=0;
	}else{
		sampleOffset+=2;
	}
	//clear
	IOWR_ALTERA_AVALON_PIO_EDGE_CAP(PIO_IN_BASE,0x01);
}



int main() {
	alt_printf("\nProject : Accelerometre");
	
	//VARAIBLE INITIALISATION
	sampleOffset = 0;
	ledId=1;
	
	//I2C INIT
	I2C_init(I2C_BASE,ALT_CPU_FREQ,100000);
	
	//SET ACC CONFIGURATION
	calibrateAcc();
	
	//TIMER IRQ
	alt_irq_register(TIMER_IRQ, NULL, (void*)readSample);
	//BUTTON IRQ
	IOWR_ALTERA_AVALON_PIO_IRQ_MASK(PIO_IN_BASE, 0x01);
	alt_irq_register(PIO_IN_IRQ, NULL, (void*)changeSampleOffset);
	
	
	return 0;
}

