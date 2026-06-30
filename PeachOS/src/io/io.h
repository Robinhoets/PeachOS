#ifndef IO_H
#define IO_H

unsigned char insb(unsigned short port);            // read one byte from given port
unsigned short insw(unsigned short port);           // read one word

void outb(unsigned short port, unsigned char val);  // one byte value to output to port - to hardware
void outw(unsigned short port, unsigned short val); // one word to port - to hardware

#endif