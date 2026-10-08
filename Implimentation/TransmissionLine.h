#include "TransmissionLine.h"

TransmissionLine::TransmissionLine(int from, int to, int distance)
{
    this->from = from;
    this->to = to;
    this->distance = distance;
    this->active = true;
}
