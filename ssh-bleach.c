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
#include <unistd.h>

int cvs_server (char *line) {
    printf ("cvs server\n");
    return execl ("/usr/bin/cvs", "cvs", "server", NULL);
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
    printf ("First argument is \"%s\"\n", new);
    return new;
}

int git_receive_pack (char *line) {
    char *arg = get_quoted_arg (line);
    if (! arg)
	return 0;

    printf ("git-receive-pack '%s'\n", arg);
    return execl ("/usr/local/bin/git-receive-pack", 
		  "git-receive-pack",
		  arg,
		  NULL);
}

int git_upload_pack (char *line) {
    char *arg = get_quoted_arg (line);
    if (! arg)
	return 0;

    printf ("git-upload-pack '%s'\n", arg);
    return execl ("/usr/local/bin/git-upload-pack", 
		  "git-upload-pack",
		  arg,
		  NULL);
}

struct {
    char *name;
    int (*command) (char *line);
} commands[] = {
    { "cvs server", cvs_server },
    { "git-receive-pack ", git_receive_pack },
    { "git-upload-pack ", git_upload_pack },
    { 0 }
};

int main (int argc, char **argv)
{
    char    *line = getenv ("SSH_ORIGINAL_COMMAND");
    int	    i;
    
    if (!line)
	return 1;
    for (i = 0; commands[i].name; i++)
	if (! strncmp (line, commands[i].name, strlen (commands[i].name)))
	    if ((*commands[i].command) (line) != 0)
		break;
    return 1;
}
