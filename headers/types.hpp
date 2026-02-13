#ifndef TYPES_HPP
#define TYPES_HPP

#include <iostream>

class Data
{
public:
    int_fast64_t receive_ts;
    double price;
    
    Data(int_fast64_t _time, double _price) {
        receive_ts = _time;
        price = _price;
    }
};


#endif