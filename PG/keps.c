/*
 * SATPG.C
 *
 * Compile:
 *   gcc satpg.c -o satpg -lm
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define _USE_MATH_DEFINES
#include <math.h>
#include <time.h>

#define MAXSATS 256
#define MAXNAME 64

#define KEPFILE "d:\keps.txt"

typedef struct
{
    char name[MAXNAME];
    char l1[128];
    char l2[128];

    double inc;
    double mm;

} SAT;

SAT sats[MAXSATS];
int satcount = 0;

/* ------------------------------------------------ */

void trim(char *s)
{
    int l = strlen(s);

    while(l > 0 &&
          (s[l-1] == '\n' || s[l-1] == '\r'))
    {
        s[l-1] = 0;
        l--;
    }
}

/* ------------------------------------------------ */
void load_tles(void)
{
    FILE *fp;
    char name[256];
    char l1[256];
    char l2[256];

    int i;
    int dup;

    fp = fopen(KEPFILE, "r");

    if(!fp)
    {
        printf("Cannot open %s\n", KEPFILE);
	exit(127);
        return;
    }

    while(fgets(name, sizeof(name), fp))
    {
        trim(name);

        /*
         * Skip blank lines
         */

        if(strlen(name) < 1)
            continue;

        /*
         * Ignore lines beginning with:
         * 1
         * 2
         * #
         */

        if(name[0] == '1' ||
           name[0] == '2' ||
           name[0] == '#')
            continue;

        /*
         * Try reading TLE pair
         */

        if(!fgets(l1, sizeof(l1), fp))
            break;

        if(!fgets(l2, sizeof(l2), fp))
            break;

        trim(l1);
        trim(l2);

        /*
         * Validate TLE lines
         */

        if(strncmp(l1, "1 ", 2) != 0)
            continue;

        if(strncmp(l2, "2 ", 2) != 0)
            continue;

        /*
         * Duplicate filter
         */

        dup = 0;

        for(i=0;i<satcount;i++)
        {
            if(stricmp(name,
                          sats[i].name) == 0)
            {
                dup = 1;
                break;
            }
        }

        if(dup)
            continue;

        /*
         * Store
         */

        strncpy(sats[satcount].name,
                name,
                MAXNAME-1);

        strcpy(sats[satcount].l1, l1);
        strcpy(sats[satcount].l2, l2);

        sscanf(l2,
               "2 %*d %lf %*f %*s %*f %*f %lf",
               &sats[satcount].inc,
               &sats[satcount].mm);

        satcount++;

        if(satcount >= MAXSATS)
            break;
    }

    fclose(fp);
}

/* ------------------------------------------------ */

char *az_to_text(double az)
{
    if(az < 22.5) return "N";
    if(az < 67.5) return "NE";
    if(az < 112.5) return "E";
    if(az < 157.5) return "SE";
    if(az < 202.5) return "S";
    if(az < 247.5) return "SW";
    if(az < 292.5) return "W";
    if(az < 337.5) return "NW";

    return "N";
}

#include <math.h>

#define DEG2RAD (M_PI / 180.0)
#define RAD2DEG (180.0 / M_PI)

/*
 * Observer location
 * Mitcham UK example
 */

#define OBS_LAT 51.40
#define OBS_LON -0.16

/*
 * Earth / GEO constants
 */

#define EARTH_RADIUS 6378.137
#define GEO_RADIUS   42164.0

/*
 * Extract sub-satellite longitude
 *
 * For GEO satellites:
 * longitude ≈ RAAN + Mean Anomaly
 */

double get_geo_longitude(SAT *s)
{
    double raan;
    double mean_anom;
	double lon;
    /*
     * TLE line 2 fields:
     *
     * 2 NNNNN INC RAAN ECC ARGP MA MM
     */

    sscanf(s->l2,
           "2 %*d %*lf %lf %*s %*lf %lf",
           &raan,
           &mean_anom);

    lon = raan + mean_anom;

    while(lon > 180.0)
        lon -= 360.0;

    while(lon < -180.0)
        lon += 360.0;

    return lon;
}

/*
 * Real GEO az/el calculation
 */

void fake_track(SAT *s,
                double *az,
                double *el,
                double *rng)
{
    double sat_lon = get_geo_longitude(s);

    double obs_lat = OBS_LAT * DEG2RAD;
    double obs_lon = OBS_LON * DEG2RAD;

    double sat_lon_rad = sat_lon * DEG2RAD;

    /*
     * GEO satellite position
     */

    double xs = GEO_RADIUS * cos(sat_lon_rad);
    double ys = GEO_RADIUS * sin(sat_lon_rad);
    double zs = 0.0;

    /*
     * Observer position
     */

    double xo = EARTH_RADIUS *
                cos(obs_lat) *
                cos(obs_lon);

    double yo = EARTH_RADIUS *
                cos(obs_lat) *
                sin(obs_lon);

    double zo = EARTH_RADIUS *
                sin(obs_lat);

    /*
     * Vector observer -> satellite
     */

    double dx = xs - xo;
    double dy = ys - yo;
    double dz = zs - zo;

    *rng = sqrt(dx*dx + dy*dy + dz*dz);

    /*
     * ENU conversion
     */

    double east =
        -sin(obs_lon)*dx +
         cos(obs_lon)*dy;

    double north =
        -sin(obs_lat)*cos(obs_lon)*dx
        -sin(obs_lat)*sin(obs_lon)*dy
        +cos(obs_lat)*dz;

    double up =
         cos(obs_lat)*cos(obs_lon)*dx
        +cos(obs_lat)*sin(obs_lon)*dy
        +sin(obs_lat)*dz;

    *az = atan2(east, north) * RAD2DEG;

    if(*az < 0)
        *az += 360.0;

    *el = atan2(up,
                sqrt(east*east + north*north))
                * RAD2DEG;
}




void draw_plot(double az, double el)
{
    char g[9][34];
    int r,c;

    int x,y;

    for(r=0;r<9;r++)
    {
        for(c=0;c<33;c++)
            g[r][c] = ' ';

        g[r][33] = 0;
    }

    strcpy(g[0], "                N");
    strcpy(g[4], "W --------------+-------------- E");
    strcpy(g[8], "                S");

    double radius = (90.0 - el) / 90.0;

    double ang = (az - 90.0) * M_PI / 180.0;

    x = 16 + (int)(radius * 12 * cos(ang));
    y = 4 + (int)(radius * 4 * sin(ang));

    if(x >=0 && x < 33 && y >=0 && y < 9)
        g[y][x] = '*';

    putchar('\r');
    putchar('\n');

    for(r=0;r<9;r++)
        printf("%s\r\n", g[r]);

    putchar('\r');
    putchar('\n');
}

/* ------------------------------------------------ */

void show_sat(SAT *s)
{
    double az,el,rng;

    fake_track(s, &az, &el, &rng);

    if(el > 0)
        printf("%s GOOD\r\n", s->name);
    else
        printf("%s NOT VISIBLE\r\n", s->name);

    printf("AZ %.0f (%s)\r\n",
           az,
           az_to_text(az));

    if(el > 0)
        printf("EL %.0f deg\r\n", el);
    else
        printf("EL BELOW HORIZON\r\n");

    printf("RNG %.0f km\r\n", rng);

    putchar('\r');
    putchar('\n');


    if(el > 0)
    {
        printf("Aim antenna: %s ",
               az_to_text(az));

        if(el < 15)
            printf("low.\r\n");
        else if(el < 45)
            printf("mid sky.\r\n");
        else
            printf("high.\r\n");
    }
    else
    {
        printf("Satellite below horizon.\r\n");
    }

    draw_plot(az, el);

    if(s->inc > 95 && s->inc < 100)
        printf("Sun-synchronous polar LEO\r\n");

    printf("%.2f orbits/day\r\n", s->mm);
}

/* ------------------------------------------------ */

SAT *find_sat(char *s)
{
    int i;

    for(i=0;i<satcount;i++)
    {
        if(stricmp(s, sats[i].name) == 0)
            return &sats[i];

        if(atoi(s) == (i+1))
            return &sats[i];
    }

    return NULL;
}

/* ------------------------------------------------ */

int main(int argc, char **argv)
{
    int level;
    int i;

    load_tles();

    if(argc < 3)
    {
        printf("Bad PG invocation\r\n");
        return 0;
    }

    level = atoi(argv[2]);

    /*
     * FIRST INVOCATION
     */

    if(level == 0)
    {
        printf("Satellite tracker\r\n");
        printf("-----------------\r\n\r\n");

        for(i=0;i<satcount;i++)
        {
            printf("%2d: %s\r\n",
                   i+1,
                   sats[i].name);
        }

        printf("\r\n");
        printf("Enter sat number or name (0=Exit):\r\n");

        return 1;
    }

    /*
     * SECOND + INVOCATION
     */

    if(argc < 6)
    {
        printf("No satellite specified.\r\n");
        return 0;
    }


    if(atoi(argv[5])==0 && strlen(argv[5])==1 )
    {
        printf("Bye!\r\n");
        return 0;
    }

    SAT *s = find_sat(argv[5]);

    if(!s)
    {
        printf("Satellite not found.\r\n");
        printf("\r\nEnter sat number or name (0=Exit):\r\n");
	return 1;
    }

    putchar('\r');
    putchar('\n');


    show_sat(s);

    printf("\r\nEnter sat number or name (0=Exit):\r\n");

    return 1;
}
