#include <linux/input.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <signal.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include "ini.h"

#define RUNNING_DIR	"/tmp"
#define LOCK_FILE	"scanserver.lock"
#define LOG_FILE	"scanserver.log"

void log_message(filename,message)
char *filename;
char *message;
{
	FILE *logfile;
	logfile=fopen(filename,"a");
	if(!logfile) return;
	fprintf(logfile,"%s\n",message);
	fclose(logfile);
}

void signal_handler(sig)
int sig;
{
	switch(sig) 
	{
		case SIGHUP:
			log_message(LOG_FILE,"hangup signal catched");
			break;
		case SIGTERM:
			log_message(LOG_FILE,"terminate signal catched");
			exit(0);
			break;
	}
}

int addStoreData(const char *filepath, const char *data)
{
  int rc = 0;
  FILE *fOut = fopen (filepath, "ab+");
  if (fOut != NULL) {
    if(data == "||"){
     fprintf(fOut, "\n");
    }else if(data == "|"){
     fprintf(fOut, " ");
   }else{
      fprintf(fOut, "%d", data);
    }
    if (fclose (fOut) == EOF) rc = 0;
  }
  return rc;
}

int writelockfile(const char *filepath, const char *openfile)
{
  int rc = 0;
  FILE *fOut = fopen (filepath, "w+");
  fprintf(fOut, "%s", openfile);
  if (fclose (fOut) == EOF) rc = 0;
  return rc;
}

void daemonize()
{
	int i,lfp;
	char str[10];
	if(getppid()==1) return; /* already a daemon */
	i=fork();
	if (i<0) exit(1); /* fork error */
	if (i>0) exit(0); /* parent exits */
	/* child (daemon) continues */
	setsid(); /* obtain a new process group */
	for (i=getdtablesize();i>=0;--i) close(i); /* close all descriptors */
	i=open("/dev/null",O_RDWR); dup(i); dup(i); /* handle standart I/O */
	umask(027); /* set newly created file permissions */
	chdir(RUNNING_DIR); /* change running directory */
	lfp=open(LOCK_FILE,O_RDWR|O_CREAT,0640);
	if (lfp<0) exit(1); /* can not open */
	if (lockf(lfp,F_TLOCK,0)<0) exit(0); /* can not lock */
	/* first instance continues */
	sprintf(str,"%d\n",getpid());
	write(lfp,str,strlen(str)); /* record pid to lockfile */
	signal(SIGCHLD,SIG_IGN); /* ignore child */
	signal(SIGTSTP,SIG_IGN); /* ignore tty signals */
	signal(SIGTTOU,SIG_IGN);
	signal(SIGTTIN,SIG_IGN);
	signal(SIGHUP,signal_handler); /* catch hangup signal */
	signal(SIGTERM,signal_handler); /* catch kill signal */
}

int main(int argc, char **argv)
{
	daemonize();
	endlesscheck();
}

int endlesscheck()
{
	log_message(LOG_FILE,"in endlesscheck for device");
	int endlessloop = 0;
	FILE *fOut;
	while(endlessloop == 0)
	{
		fOut = fopen ("/dev/input/by-id/usb-Metrologic_Metrologic_Scanner___________-event-kbd", "r");
		if(fOut == NULL){
			log_message(LOG_FILE,"device not ready");
			sleep(1); /* run */
		}else{
			endlessloop = 1;
			log_message(LOG_FILE,"device ready");
		}
	}
	fclose (fOut);
	initscanneranddosomework();
}


int initscanneranddosomework()
{
	int endlessloop = 0;
	FILE *fOut;
	while(1){
		fOut = fopen ("/dev/input/by-id/usb-Metrologic_Metrologic_Scanner___________-event-kbd", "r");
		if(fOut == NULL){
			log_message(LOG_FILE,"device brutaly exjected NOT ready");
			endlesscheck();
			return 0;
		}else{
			log_message(LOG_FILE,"working...");
		}
		sleep(1);
	}
	fclose (fOut);
}

