//FEDI KHAYATI;
//TSAMI MARIA ELENI;

#include <stdio.h>    // Used for printf() statements
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <stdint.h>
#include <wiringPi.h> // Include WiringPi library!

#include <pthread.h>

#include <math.h>
#include <time.h>

#include "devices.h"

#include <semaphore.h>


#define NUM_THREADS 4
#define TILT_THRESHOLD 30


// semaphore for critical region 
sem_t sem_S1;
sem_t sem_S2;
sem_t sem_D;

int symptom_S1 = 0; // HEAD TILT
int symptom_S2 = 0; // SHARP TURN
int symptom_D0 = 0; // CORRECT DISTANCE
int symptom_D1 = 0;	// INSECURE DISTANCE
int symptom_D2 = 0;	// IMRUDENT DISTANCE
int symptom_D3 = 0; // DANGER COLLISION



float mapSensorToSteeringRotation(int ADC){
	return ADC*(360.0/1023.0)-180.0;
}

float mapSensorToSpeed(int ADC){
	return (ADC/1023.0)*140.0; // km/h
}

void *risk_thread (){

	printf ("Risk Thread 1 \n");
    int s1 = 0, s2 = 0, d0 =0, d1 = 0, d2 = 0, d3 = 0;


	while(1){

        sem_wait(&sem_S1);
        s1 = symptom_S1;
        sem_post(&sem_S1);
                
        sem_wait(&sem_S2);
        s2 = symptom_S2;
        sem_post(&sem_S2);
            
        sem_wait(&sem_D);
        d0 = symptom_D0;
        d1 = symptom_D1;
        d2 = symptom_D2;
        d3 = symptom_D3;
        sem_post(&sem_D);

        set_led_1 (0);
        set_led_2 (0);
			
		// Analyze conditions and take actions
        if ((s1 || s2) && d0) {
            set_led_1 (1);// S1 or S2 and D0: turn on Light 1
            //printf("Turn on Light 1 \n");
        }

        if (s1 && s2 && d0) {
            set_led_2 (1); // S1 and S2 and D0: turn on Light 2
            //printf("Turn on Light 2 \n");
        }

        if ((s1 || s2) && d1) {
            set_led_1 (1); // S1 or S2 and D1: turn on Light 1
            set_led_2 (1); // S1 or S2 and D1: turn on Light 2
            moveServo(60); // Activate brake level 1
            //printf("Activate brake level 1 \n");
        }

        if ((s1 || s2) && d2) {
            set_led_1 (1); // S1 or S2 and D2: turn on Light 1
            set_led_2 (1); // S1 or S2 and D2: turn on Light 2
            moveServo(120); // Activate brake level 2
            //printf("Activate brake level 2 \n");
        }

        if (d3) {
            moveServo(180); // D3: Activate brake level 3
            //printf("Activate brake level 3 \n");
        }
		
		delay(200);
	}

	pthread_exit (NULL);
}


void *turn_steering_thread (){  

  //printf ("Turn Steering Thread \n");
  
  float new_position = 0, prev_position = 0;
  int ADC2, ADC3;
  float speed = 0;
  
  while(1){
		// read steering rotation	
		ADC2 = read_single_ADC_sensor(2); // Read only channel 2
        new_position = mapSensorToSteeringRotation(ADC2);
        //printf("Steering rotation: %.2f\n", new_position);
        
        // Read Speed
		ADC3= read_single_ADC_sensor (3); // it reads only channel 3
		speed = mapSensorToSpeed(ADC3);
		
		if(speed > 40 && (fabs(new_position - prev_position) > 20)){
			// printf("sharp turn \n"); // S2
			sem_wait(&sem_S2);
			symptom_S2 = 1;
			sem_post(&sem_S2);
		}else{
			sem_wait(&sem_S2);
			symptom_S2 = 0;
			sem_post(&sem_S2);
		}
		prev_position = new_position;

		delay (350);
	}

  pthread_exit (NULL);
}

void *head_tilt_thread() {
    //printf("Head Tilt Thread:\n");

    int new_Rx = 0, prev_Rx = 0;
    int new_Ry = 0, prev_Ry = 0;
    float steering;
    int ADC2 = 0;

    while (1) {
        // Read gyroscope data
        new_Rx = Read_Giroscope_X();
        new_Ry = Read_Giroscope_Y();
        //printf("Rotation: X = %d, Y = %d\n", new_Rx, new_Ry);

        // Check for forward/backward tilt (X-axis)
        if (fabs(new_Rx) > TILT_THRESHOLD && fabs(prev_Rx) > TILT_THRESHOLD) {
            /*if (new_Rx > TILT_THRESHOLD) {
                 printf("HEAD TILT: Forward\n");
            } else if (new_Rx < -TILT_THRESHOLD) {
                printf("HEAD TILT: Backward\n");
            }*/
            sem_wait(&sem_S1);
			symptom_S1 = 1;
			sem_post(&sem_S1);
        }else{
			sem_wait(&sem_S1);
			symptom_S1 = 0;
			sem_post(&sem_S1);
		}

        // Check for left/right tilt (Y-axis)
        if (fabs(new_Ry) > TILT_THRESHOLD && fabs(prev_Ry) > TILT_THRESHOLD) {
            // Read steering sensor
            ADC2 = read_single_ADC_sensor(2); // Read only channel 2
            steering = mapSensorToSteeringRotation(ADC2);
            //printf("Steering rotation: %.2f\n", steering);

            if (new_Ry > TILT_THRESHOLD) { // Right tilt
                if (steering > 0) {
                    //printf("Head tilt matches steering to the right. No drowsiness.\n");
                    sem_wait(&sem_S1);
					symptom_S1 = 0;
					sem_post(&sem_S1);
                } else {
                    //printf("HEAD TILT: Right (possible drowsiness)\n");
                    sem_wait(&sem_S1);
					symptom_S1 = 1;
					sem_post(&sem_S1);
                }
            } else if (new_Ry < -TILT_THRESHOLD) { // Left tilt
                if (steering < 0) {
                    //printf("Head tilt matches steering to the left. No drowsiness.\n");
                    sem_wait(&sem_S1);
					symptom_S1 = 0;
					sem_post(&sem_S1);
                } else {
                    //printf("HEAD TILT: Left (possible drowsiness)\n");
                    sem_wait(&sem_S1);
					symptom_S1 = 1;
					sem_post(&sem_S1);
                }
            }else{
				sem_wait(&sem_S1);
				symptom_S1 = 0;
				sem_post(&sem_S1);
			}
        }


        prev_Rx = new_Rx;
        prev_Ry = new_Ry;

        delay(400); // Wait 400 ms
    }

    pthread_exit(NULL);
}


void *distance_thread(){

	//printf ("Distance Thread:  \n");
	float distance = 0, safety_distance = 0;     
	int ADC3_Value; 
	
	while(1){
		// Read distance     
		distance = getDistance();
		printf("Distance: %.2f cm\n", distance);

		// Read Speed
		ADC3_Value = read_single_ADC_sensor (3); // it reads only channel 3
		
		// Calculate safety distance
		safety_distance = (ADC3_Value/10)^2; 
		printf("Safety: %.2f cm\n", safety_distance);

		if (distance < safety_distance/3){ // D3
			printf("Danger collision\n");
			sem_wait(&sem_D);
			symptom_D0 = 0;
			symptom_D1 = 0;
			symptom_D2 = 0;
			symptom_D3 = 1;
			sem_post(&sem_D);
		 
		}else if(distance < safety_distance/2){ // D2
			printf("Imprudent Distance\n");
			sem_wait(&sem_D);
			symptom_D0 = 0;
			symptom_D1 = 0;
			symptom_D2 = 1;
			symptom_D3 = 0;
			sem_post(&sem_D);

		}else if(distance < safety_distance){ // D1
			printf("Insecure Distance\n");
			sem_wait(&sem_D);
			symptom_D0 = 0;
			symptom_D1 = 1;
			symptom_D2 = 0;
			symptom_D3 = 0;
			sem_post(&sem_D);		 
		}else {// D0
			printf("Correct Distance\n");
			sem_wait(&sem_D);
			symptom_D0 = 1;
			symptom_D1 = 0;
			symptom_D2 = 0;
			symptom_D3 = 0;
			sem_post(&sem_D);
		}

		delay (300);
	}

	pthread_exit (NULL);
}


int main(void)
{
    int n;
    n = init_devices ();
    set_led_1 (0);
    set_led_2 (0);
    
    sem_init(&sem_S1, 0, 1); // init semaphore
    sem_init(&sem_S2, 0, 1);
    sem_init(&sem_D, 0, 1);

    pthread_t thread [NUM_THREADS]; 

    pthread_create(&thread[0], NULL, risk_thread, NULL);
    pthread_create(&thread[1], NULL, turn_steering_thread, NULL);
    pthread_create(&thread[2], NULL, head_tilt_thread, NULL);
    pthread_create(&thread[3], NULL, distance_thread, NULL);

    pthread_join(thread[0], NULL);
    pthread_join(thread[1], NULL);
    pthread_join(thread[2], NULL);
    pthread_join(thread[3], NULL);

    close_devices ();

    return (0);
}
