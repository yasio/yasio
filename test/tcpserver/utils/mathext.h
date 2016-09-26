//
// Copyright (c) 2014 purelib - All Rights Reserved
//
#ifndef _MATNEXT_H_
#define _MATNEXT_H_

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <random>
#include <limits>
#include <cmath>

#ifndef _WIN32
namespace std {
    static const long double _Pi = 3.14159265358979323846264338327950288L;
}
#endif

namespace  mathext {

/* value range: [0, 0x7fffffff] */
static unsigned int rand(void)
{
    static std::random_device _s_device;
    static std::default_random_engine _s_engine(_s_device());

    return _s_engine();
}

/* value range: [min, limit) half closed interval */
static unsigned int rrand(unsigned int min, unsigned int limit)  
{
    return ( rand() % ((limit) - (min)) + (min) );
}

/* value range: [min, limit] closed interval */
static unsigned int rrand_ci(unsigned int min, unsigned int max)
{
    return rrand(min, max + 1);
}

/* value range: [0.f, 1.f] closed interval */
static float randf(void)
{
    static std::default_random_engine _s_engine(time(NULL));
    return (_s_engine() / static_cast<float>((std::numeric_limits<std::default_random_engine::result_type>::max)()));
}

/* value range: [min, limit] closed interval */
static float rrandf(float min, float limit) 
{
    return min + (limit - min) * randf();
}

static double round(double r)
{
    return (r > 0.0) ? std::floor(r + 0.5) : std::ceil(r - 0.5);
}

static double to_rad(double degrees)
{
    return ( (degrees) * std::_Pi / 180 );
}

static double to_deg(double radians)
{
    return ( (radians) * 180 / std::_Pi );
}

static const float EARTH_RADIUS = 6371.004f;
class latlon 
{
public:
    latlon(float lat, float lon) : latitude_(lat), longitude_(lon){}
    latlon(float latD,float latM, float latS, float lonD,float lonM,float lonS) : 
        latitude_(latD + latM / 60 + latS / 3600),
        longitude_(lonD + lonM / 60 + lonS / 3600)
    {
    }

    float haversineDistance(const latlon& to)
    { // haversine
        float lat1 = to_rad(latitude_), lon1 = to_rad(longitude_);
        float lat2 = to_rad(to.latitude_), lon2 = to_rad(to.longitude_);
        float dLat = lat2 - lat1;
        float dLon = lon2 - lon1;

        float halfLatS = std::sin(dLat/2);
        float halfLonS = std::sin(dLon/2);

        float a = halfLatS * halfLatS +
            std::cos(lat1) * std::cos(lat2) * 
            halfLonS * halfLonS;
        float c = 2 * std::atan2(std::sqrt(a), std::sqrt(1-a));
        float d = EARTH_RADIUS * c;

        return d;
    }

    float sphericalDistance(const latlon& p)
    {
        float theta1 = to_rad(latitude_), theta2 = to_rad(p.latitude_), delta = to_rad(p.longitude_-longitude_); // gives d in km
        float d = std::acos( std::sin(theta1)* std::sin(theta2) +
            std::cos(theta1)*std::cos(theta2) * std::cos(delta) ) * EARTH_RADIUS;
        return d;
    }

    float fastDistance(const latlon& p)
    { // Equirectangular approximation
        float lat1 = to_rad(latitude_), lon1 = to_rad(longitude_), lat2 = to_rad(p.latitude_), lon2 = to_rad(p.longitude_);

        float x = (lon2-lon1) * std::cos((lat1+lat2)/2);
        float y = (lat2-lat1);
        float d = std::sqrt(x*x + y*y) * EARTH_RADIUS;
        return d;
    }

    latlon midpointTo(const latlon& point)
    {
        float lat1 = to_rad(this->latitude_), lon1 = to_rad(longitude_);
        float lat2 = to_rad(point.latitude_);
        float dLon = to_rad( (point.longitude_ - longitude_) );

        float Bx = std::cos(lat2) * std::cos(dLon);
        float By = std::cos(lat2) * std::sin(dLon);

        float lat3 = std::atan2(std::sin(lat1)+std::sin(lat2),
            std::sqrt( (std::cos(lat1)+Bx)*(std::cos(lat1)+Bx) + By*By) );
        float lon3 = lon1 + std::atan2(By, std::cos(lat1) + Bx);
        lon3 = (int)(lon3+3*std::_Pi) % (int)(2*std::_Pi) - std::_Pi;  // normalise to -180..+180

        return latlon(to_deg(lat3), to_deg(lon3));
    }

    int beringTo(const latlon& point)
    {
        float lat1 = to_rad(latitude_), lat2 = to_rad(point.latitude_);
        float dLon = to_rad(point.longitude_ - this->longitude_) ;

        float y = std::sin(dLon) * std::cos(lat2);
        float x = std::cos(lat1)*std::sin(lat2) -
            std::sin(lat1)*std::cos(lat2)*std::cos(dLon);
        float brng = std::atan2(y, x);
        
        return (int)(to_deg(brng)+360) % 360;
    }

private:
    float       latitude_;  // Î¬¶È
    float       longitude_; // ¾­¶È
};

};

// static const float GOLDEN_RATIO = 1.618033988749895;

/*static float distance(CCPoint p1, CCPoint p2)
{
    float diffX = p1.x - p2.x;
    float diffY = p1.y - p2.y;
    return std::sqrt(diffX * diffX + diffY * diffY);
}*/

#endif

