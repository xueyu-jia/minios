#ifndef PCI_H
#define PCI_H

#define mk_pci_add(bus,dev,fun,reg) 0x80000000|bus<<16|dev<<11|fun<<8|reg<<2
#define MAXBUS 255
#define MAXDEV 31
#define MAXFUN 7

#endif