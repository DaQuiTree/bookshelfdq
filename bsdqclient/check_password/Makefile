# contrib/slapd-modules/check_password/Makefile 
# Copyright 2007 Michael Steinmann, Calivia. All Rights Reserved.  
# Updated by Pierre-Yves Bonnetain, B&A Consultants, 2008 
# Updated by Trevor Vaughan, Onyx Point Inc., 2011 
# CC=gcc

# Where to look for the CrackLib dictionaries
#
CRACKLIB=/usr/local/share/cracklib/pw_dict

# Turn on local debugging.
OPT=-g -O2 -Wall -DHAVE_CRACKLIB -DCRACKLIB_DICTPATH="\"$(CRACKLIB)\""

LDAP_INC_PATH=/usr/local/openldap-2.4.47

# Where to find the OpenLDAP headers.
#
LDAP_INC=-I$(LDAP_INC_PATH)/include -I$(LDAP_INC_PATH)/servers/slapd -I$(LDAP_INC_PATH)/build-servers/include

# Where to find the CrackLib headers.
#
#CRACK_INC=/usr/local/include

INCS=$(LDAP_INC) $(CRACK_INC)

LDAP_LIB=-lldap_r -llber

# Comment out this line if you do NOT want to use the cracklib.
# You may have to add an -Ldirectory if the libcrak is not in a standard
# location
#
CRACKLIB_LIB=-lcrack

LIBS=$(LDAP_LIB) $(CRACKLIB_LIB)


all: check_password.o #check_password_test

check_password.o:
	$(CC) $(OPT) -c $(INCS) check_password.c

clean:
	$(RM) check_password.o
