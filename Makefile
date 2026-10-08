KMOD = asp

SRCS = asp.c
SRCS+= asp.h 
SRCS+= bus_if.h
SRCS+= pci_if.h

.include <bsd.kmod.mk>
