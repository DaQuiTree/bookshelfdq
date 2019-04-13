/*
 * This program is copyright Alec Muffett 1993, portions copyright other authors.
 * The authors disclaim all responsibility or liability with respect to it's usage
 * or its effect upon hardware or computer systems.
 */

#include <stdio.h>
#include "config.h"
#include "crack.h"
#ifdef HAVE_INTTYPES_H
#include <inttypes.h>
#endif
#include "packer.h"

int
main ()
{
    char buffer[1024];

    printf("enter potential passwords, one per line...\n");

    while (fgets (buffer, 1000, stdin))
    {
    	const char *val;

	Chop(buffer);

	val = FascistCheck(buffer, NULL);

	if (!val)
	{
		val = "ok";
	}
	printf ("%s: %s\n", buffer, val);
    }

    return (0);
}
