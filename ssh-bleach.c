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
#include <stdarg.h>
#include <errno.h>

int verbose = 0;

void panic (char *fmt, ...)
{
    va_list ap;

    va_start (ap, fmt);
    vfprintf (stderr, fmt, ap);
    va_end (ap);
    exit (1);
}
    
int cvs_server (char *line) {
    if (verbose)
	printf ("cvs server\n");
    (void) execl ("/usr/bin/cvs", "cvs", "server", NULL);
    panic ("exec /usr/bin/cvs failed: \"%s\"\n", strerror (errno));
}

const char bad_chars[] = "\"$%'*;<>?[\\]`|";

char *get_quoted_arg (char *line)
{
    char    *first = strchr (line, '\'');
    char    *old;
    char    *last;
    char    *new, *n;
    char    c;
    
    if (!first)
	panic ("Expected quoted argument\n");
    last = strrchr (line, '\'');
    if (!last || last == first)
	panic ("Missing close quote\n");
    first++;
    new = malloc (last - first + 1);
    if (!new)
	panic ("Out of memory\n");
    n = new;
    for (old = first; old < last; old++) {
	c = *old;
	if (strchr (bad_chars, c))
	    panic ("Invalid character in argument \"%c\"\n", c);
	*n++ = c;
    }
    *n++ = '\0';
    if (verbose)
	printf ("First argument is \"%s\"\n", new);
    return new;
}

int git_receive_pack (char *line) {
    char *arg = get_quoted_arg (line);
    if (! arg)
	panic ("Missing argument to git-receive-pack\n");

    if (verbose)
	printf ("git-receive-pack '%s'\n", arg);
    execl ("/usr/local/bin/git-receive-pack", 
	   "git-receive-pack",
	   arg,
	   NULL);
    panic ("exec /usr/local/bin/git-receive-pack failed: \"%s\"\n", strerror (errno));
}

int git_upload_pack (char *line) {
    char *arg = get_quoted_arg (line);
    if (! arg)
	panic ("Missing argument to git-upload-pack\n");

    if (verbose)
	printf ("git-upload-pack '%s'\n", arg);
    (void) execl ("/usr/local/bin/git-upload-pack", 
		  "git-upload-pack",
		  arg,
		  NULL);
    panic ("exec /usr/local/bin/git-receive-pack failed: \"%s\"\n", strerror (errno));
}

struct {
    char *name;
    int (*command) (char *line);
} commands[] = {
    { "cvs server", cvs_server },
    { "git-receive-pack", git_receive_pack },
    { "git-upload-pack", git_upload_pack },
    { 0 }
};

int main (int argc, char **argv)
{
    char    *line = getenv ("SSH_ORIGINAL_COMMAND");
    int	    i;
    
    if (argv[1] && !strcmp (argv[1], "-v"))
	verbose = 1;
    if (!line)
	panic ("Missing original command\n");
    for (i = 0; commands[i].name; i++)
	if (! strncmp (line, commands[i].name, strlen (commands[i].name)))
	    if ((*commands[i].command) (line) != 0)
		break;
    panic ("Unsupported command \"%s\"\n", line);
}
