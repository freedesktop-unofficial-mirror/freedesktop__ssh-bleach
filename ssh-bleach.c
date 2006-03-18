/*
    ssh-bleach - sanitize an ssh command line

    Copyright © 2006  Keith Packard, Carl Worth and Daniel Stone

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
#include <sys/types.h>
#include <pwd.h>

int verbose = 0;

void panic (char *fmt, ...) __attribute__((noreturn));

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

int svn_server (char *line) {
    if (verbose)
        printf ("svn -t\n");
    (void) execl ("/usr/bin/svn", "-t", NULL);
    panic ("exec /usr/bin/svn failed: \"%s\"\n", strerror (errno));
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

/* watch you don't overflow allowed_roots! */
struct {
    char *username;
    char *allowed_roots[16];
} annarchy_rsync_users[] = {
    { "kemper-cvs",
      { "/srv/anoncvs.freedesktop.org/", "/srv/anongit.freedesktop.org", "/srv/anonsvn.freedesktop.org", NULL }
    },
    { NULL, { NULL } }
};

int rsync (char *line) {
    struct passwd *passwd = NULL;
    char *newcmd = NULL, *lastarg = NULL;
    char **user_roots = NULL;
    int status = 0, i = 0;

    passwd = getpwuid(getuid());
    if (!passwd)
        panic("failed to get pwent for current user\n");

    for (i = 0; annarchy_rsync_users[i].username; i++) {
        if (strcmp(passwd->pw_name, annarchy_rsync_users[i].username) == 0) {
            user_roots = annarchy_rsync_users[i].allowed_roots;
            status = 1;
	    break;
	}
    }

    if (status == 1) {
        newcmd = malloc((strlen(line) + strlen("/usr/bin/") + 1) *
                        sizeof(char));
        if (!newcmd)
            panic("failed to allocate space for arg parsing\n");
        sprintf(newcmd, "/usr/bin/%s", line);

        if (strchr(newcmd, '`') || strchr(newcmd, '<') ||
            strchr(newcmd, '>') || strchr(newcmd, '(') ||
            strchr(newcmd, ')') || strstr(newcmd, ".."))
            panic("treachery in command '%s'\n", newcmd);

        lastarg = strrchr(newcmd, ' ');
        if (!lastarg)
            panic("missing argument to rsync ('%s')\n", newcmd);

        /* elide the space */
        lastarg++;

        for (status = 0, i = 0; user_roots[i]; i++) {
            if (strncmp(lastarg, user_roots[i],
                        strlen(user_roots[i])) == 0) {
                status = 1;
                break;
            }
        }

	if (status == 0)
            panic("not permitted to sync to %s\n", lastarg);

        if (verbose)
            printf("rsync '%s'\n", newcmd);
        status = system(newcmd);
        free(newcmd);

        if (status != 0)
            panic("system /usr/bin/rsync failed: \"%s\"\n", strerror (errno));
        else
            exit(0);
    }
    else {
        if (verbose)
            printf("user %s not authorised to rsync\n", passwd->pw_name);
        return 0;
    }
}

struct {
    char *name;
    int (*command) (char *line);
} commands[] = {
    { "cvs server", cvs_server },
    { "svn -t", svn_server },
    { "git-receive-pack", git_receive_pack },
    { "git-upload-pack", git_upload_pack },
    { "rsync", rsync },
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
