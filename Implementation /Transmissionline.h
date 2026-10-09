#ifndef TRANSMISSIONLINE_H
#define TRANSMISSIONLINE_H

class TransmissionLine
{
public:
    int from;
    int to;
    int distance;
    bool active;

    TransmissionLine(int from, int to, int distance);
};

#endif
