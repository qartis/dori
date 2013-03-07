#include <stdio.h>
#include <stdint.h>
#include <math.h>
#include <stdlib.h>

int mx, my, mz;
int ax, ay, az;

float map(float x, float in_min, float in_max, float out_min, float out_max)
{
    return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}

float avg;

float RadiansToDegrees(float rads)
{
  // Correct for when signs are reversed.
  if(rads < 0)
    rads += 2*M_PI;
      
  // Check for wrap due to addition of declination.
  if(rads > 2*M_PI)
    rads -= 2*M_PI;
   
  // Convert radians to degrees for readability.
  float heading = rads * 180/M_PI;
       
  return heading;
}

int main(int argc, char **argv){
    int rc;
    setbuf(stdin, NULL);
    setbuf(stdout, NULL);

    int c;
    while ((c = getchar()) != 'x') { };

    for (;;) {
        if ((rc = scanf("%d %d %d", &mx, &my, &mz)) != 3) {
            printf("read error: %d\n", rc);
            exit(1);
        }
        if ((rc = scanf("%d %d %d", &ax, &ay, &az)) != 3) {
            printf("read error: %d\n", rc);
            exit(1);
        }
        while ((c = getchar()) != 'x') { };

#if 0
        printf("accel: %d %d %d\n", ax, ay, az);
        printf("magne: %d %d %d\n", mx, my, mz);
#endif

        // We are swapping the accelerometers axis as they are opposite to the compass the way we have them mounted.
        // We are swapping the signs axis as they are opposite.
        // Configure this for your setup.
        //


        float accX = -ay / 50.0;
        float accY = -ax / 50.0;


        float rollRadians = asin(accY);
        float pitchRadians = asin(accX);


        // We cannot correct for tilt over 40 degrees with this algorthem, if the board is tilted as such, return 0.
        if(rollRadians > 0.78 || rollRadians < -0.78 || pitchRadians > 0.78 || pitchRadians < -0.78)
        {
            printf("fuck\n");
            exit(1);
        }

        // Some of these are used twice, so rather than computing them twice in the algorithem we precompute them before hand.
        float cosRoll = cos(rollRadians);
        float sinRoll = sin(rollRadians);  
        float cosPitch = cos(pitchRadians);
        float sinPitch = sin(pitchRadians);

        // The tilt compensation algorithem.
        float Xh = 0.92 * mx * cosPitch + 0.92 * mz * sinPitch;
        float Yh = 0.92 * mx * sinRoll * sinPitch + 0.92 * my * cosRoll - 0.92 * mz * sinRoll * cosPitch;

        float heading = atan2(Yh, Xh);

        avg = 0.95 * avg + 0.05 * RadiansToDegrees(heading);

        printf("%f\n", avg - 140.0);
    }
}

