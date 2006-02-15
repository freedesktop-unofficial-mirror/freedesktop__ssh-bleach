/*
    ssh-bleach - sanitize an ssh command line

    Copyright © 2006  Keith Packard and Carl Worth

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; using version 2 of the License.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int verbose = 0;

int cvs_server (char *line) {
    if (!strcmp (line, "cvs server")) {
	if (verbose)
	    printf ("cvs server\n");
	return execl ("/usr/bin/cvs", "cvs", "server", NULL);
    }
    return 0;
}

char *get_quoted_arg (char *line)
{
    char    *first = strchr (line, '\'');
    char    *old;
    char    *last;
    char    *new, *n;
    
    if (!first)
	return NULL;
    last = strrchr (line, '\'');
    if (!last || last == first)
	return NULL;
    first++;
    new = malloc (last - first + 1);
    if (!new)
	return NULL;
    n = new;
    for (old = first; old < last; old++) {
	if (*old == '\\')
	    continue;
	*n++ = *old;
    }
    *n++ = '\0';
    if (verbose)
	printf ("First argument is \"%s\"\n", new);
    return new;
}

int git_receive_pack (char *line) {
    char *arg;
    
    if (!strncmp (line, "git-receive-pack '", 18) && 
	(arg = get_quoted_arg (line)))
    {
	if (verbose)
	    printf ("git-receive-pack '%s'\n", arg);
	return execl ("/usr/local/bin/git-receive-pack", 
		      "git-receive-pack",
		      arg,
		      NULL);
    }
    return 0;
}

int git_upload_pack (char *line) {
    char *arg;
    
    if (!strncmp (line, "git-upload-pack '", 17) && 
	(arg = get_quoted_arg (line)))
    {
	if (verbose)
	    printf ("git-upload-pack '%s'\n", arg);
	return execl ("/usr/local/bin/git-upload-pack", 
		      "git-upload-pack",
		      arg,
		      NULL);
    }
    return 0;
}

int (*commands[]) (char *line) = {
    cvs_server,
    git_receive_pack,
    git_upload_pack,
    NULL
};

int main (int argc, char **argv)
{
    char    *line = getenv ("SSH_ORIGINAL_COMMAND");
    int	    i;
    
    if (argv[1] && !strcmp (argv[1], "-v"))
	verbose = 1;
    if (!line)
	return 1;
    for (i = 0; commands[i]; i++)
	if ((*commands[i]) (line) != 0)
	    break;
    return 1;
}
