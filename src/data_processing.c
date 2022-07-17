#include "main.h"

//$GPGGA,201229.00,6026.86958,N,02217.14119,E,1,08,0.96,66.1,M,18.7,M,,*6C
//    eg3. $GPGGA,hhmmss.ss,llll.ll,a,yyyyy.yy,a,x,xx,x.x,x.x,M,x.x,M,x.x,xxxx*hh
//1    = UTC of Position
//2    = Latitude
//3    = N or S
//4    = Longitude
//5    = E or W
//6    = GPS quality indicator (0=invalid; 1=GPS fix; 2=Diff. GPS fix)
//7    = Number of satellites in use [not those in view]
//8    = Horizontal dilution of position
//9    = Antenna altitude above/below mean sea level (geoid)
//10   = Meters  (Antenna height unit)
//11   = Geoidal separation (Diff. between WGS-84 earth ellipsoid and
//    mean sea level.  -=geoid is below WGS-84 ellipsoid)
//12   = Meters  (Units of geoidal separation)
//13   = Age in seconds since last update from diff. reference station
//14   = Diff. reference station ID#
//15   = Checksum

//$GPRMC,201230.00,A,6026.86949,N,02217.14087,E,0.689,,150722,,,A*72
