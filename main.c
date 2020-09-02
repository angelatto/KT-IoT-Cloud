#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <string.h>
#include <unistd.h>

#include <wiringPi.h>
#include <sys/types.h>
#include <stdint.h>
#include "iotmakers.h"

#define LED 		24
#define MAXTIMINGS 	85
#define FAN		22
#define LIGHTSEN_OUT 	0


static int DHTPIN = 
11;
static int dht22_dat[5] = {0, 0, 0, 0, 0};

int ret_temp, ret_humid, ret_light;

static uint8_t sizecvt(const int read);
int read_dht22_dat();
static int local_loop = (0);


static void SigHandler(int sig)
{
	switch(sig)
	{
		case SIGTERM :
		case SIGINT :
			printf("accept signal SIGINT[%d]\n", sig);
			digitalWrite( LED, LOW );
			digitalWrite( FAN, LOW);
			im_stop_service();
			local_loop = (0);
			break;
		default : ;
	};
	return;
} 


/* ====================================================== */
static void mycb_numdata_handler(char *tagid, double numval)
{
	printf("tagid=[%s], val=[%f] \n", tagid, numval);
}

static void mycb_strdata_handler(char *tagid, char *strval)
{
	printf("tagid=[%s], val=[%s] \n", tagid, strval);

	if( !strcmp(tagid, "LED") )
	{
		if( !strcmp(strval, "ON") )
		{
			printf(" ====== LED On.. \n");
			digitalWrite( LED, HIGH );
		}
		else if( !strcmp(strval, "OFF") )
		{
			printf(" ====== LED OFF.. \n");
			digitalWrite( LED, LOW );
		}
	}
	else if( !strcmp(tagid, "FAN") )
	{
		if( !strcmp(strval, "ON") )
		{
			printf(" ====== FAN  On.. \n");
			digitalWrite( FAN, 1 );
		}
		else if( !strcmp(strval, "OFF") )
		{
			printf(" ====== FAN  OFF.. \n");
			digitalWrite( FAN, 0 );
		}
	}
	else	printf(" =====  other control.. \n");


}

/* ============================
  Sending the collection data: im_send_numdata(); im_send_strdata();
  ============================ */
int main()
{
	int i, rc, my_temp, light_val;
	double val = 0.0;

	if( wiringPiSetup() == -1 ) 	exit(1);
	pinMode( LED, OUTPUT );
	pinMode( FAN, OUTPUT );


	signal(SIGINT,   SigHandler);	
	signal(SIGTERM,  SigHandler);	

	printf("im_init()\n");
	rc = im_init_with_config_file("./config.txt");
	if ( rc < 0  )	{
		printf("fail im_init()\n");
		return -1;
	}

	im_set_loglevel(LOG_LEVEL_DEBUG);
	im_set_numdata_handler(mycb_numdata_handler);
	im_set_strdata_handler(mycb_strdata_handler);

	printf("im_start_service()...\n");
	rc = im_start_service();
	if ( rc < 0  )	{
		printf("fail im_start_service()\n");
		im_release();
		return -1;
	}

	local_loop = (1);
	val = 0;
	while( local_loop == (1) )
	{

		light_val = get_light_sensor();

		while( read_dht22_dat() == 0)
		{
			delay(500);
		}
		my_temp = ret_temp;

		printf("im_send_numdata(), Temp = %d \n", my_temp);
		rc = im_send_numdata("Temp", my_temp, 0);
		if ( rc < 0  )	{
			printf("ErrCode[%d]\n", im_get_LastErrCode());
			break;
		}

		printf("\nim_send_strdata()...\n");
		rc = im_send_strdata("LED", "UNKOWN", 0);
		if ( rc < 0  )	{
			printf("ErrCode[%d]\n", im_get_LastErrCode());
			break;
		}

		rc = im_send_numdata("Light", light_val ,0);

		sleep(1);
	}

	printf("im_stop_service()\n");
	im_stop_service();
	printf("im_release()\n");
	im_release();
	return 0;
}


static uint8_t sizecvt(const int read)
{
  if (read > 255 || read < 0)
  {
    printf("Invalid data from wiringPi library\n");
    exit(EXIT_FAILURE);
  }
  return (uint8_t)read;
}

int read_dht22_dat()
{
    uint8_t laststate = HIGH;
    uint8_t counter = 0;
    uint8_t j = 0, i;

    dht22_dat[0] = dht22_dat[1] = dht22_dat[2] = dht22_dat[3] = dht22_dat[4] = 0;

    pinMode(DHTPIN, OUTPUT);
    digitalWrite(DHTPIN, HIGH);
    delay(10);
    digitalWrite(DHTPIN, LOW);
    delay(18);
    pinMode(DHTPIN, INPUT);
    pinMode( LIGHTSEN_OUT, INPUT);

    
    for ( i=0; i< MAXTIMINGS; i++) 
    {
    	counter = 0;
    	while (sizecvt(digitalRead(DHTPIN)) == laststate) 
	{
      		counter++;
      		delayMicroseconds(1);
      		if (counter == 255) 	break; 
    	}
    	laststate = sizecvt(digitalRead(DHTPIN));

    	if (counter == 255) break;

    	if ((i >= 4) && (i%2 == 0)) {
      		dht22_dat[j/8] <<= 1;
      		if (counter > 60) 	dht22_dat[j/8] |= 1;
      		j++;
    	}
     }

    if ((j >= 40) && (dht22_dat[4] == ((dht22_dat[0] + dht22_dat[1] + dht22_dat[2] + dht22_dat[3]) & 0xFF)) ) {
        float t, h;
		
        h = (float)dht22_dat[0] * 256 + (float)dht22_dat[1];
        h /= 10;
        t = (float)(dht22_dat[2] & 0x7F)* 256 + (float)dht22_dat[3];
        t /= 10.0;
        if ((dht22_dat[2] & 0x80) != 0)  t *= -1;
		
	ret_humid = (int)h;
	ret_temp = (int)t;
	printf("Humidity = %d Temperature = %d\n", ret_humid, ret_temp);
		
    	return ret_temp;
    }
    else
    {
    	printf("Data not good, skip\n");
    	return 0;
    }
}


int get_light_sensor(void){

	if(digitalRead(LIGHTSEN_OUT))	//day
		return 1;
	else	//night
		return 0;

}
