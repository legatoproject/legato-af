/*******************************************************************************
 * Copyright (c) 2012 Sierra Wireless and others.
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the Eclipse Public License v1.0
 * which accompanies this distribution, and is available at
 * http://www.eclipse.org/legal/epl-v10.html
 *
 * Contributors:
 *     Laurent Barthelemy for Sierra Wireless - initial API and implementation
 *******************************************************************************/
/*
 * appmon_daemon.c
 *
 *  Created on: Feb 15, 2010
 *      Author: lbarthelemy
 *
 * (Simple) Application Monitoring Daemon
 *
 * This program can launch applications, then stop started applications on demand.
 * Each application will be automatically restarted if it ends with an error (i.e. system return code != 0).
 * When asking an application to stop, it won't restart automatically.
 *
 *
 * This program acts as a daemon: i.e. it returns synchronously success,
 * then the program keeps running in background, waiting for request to execute.
 * To stop it, you have to send it the appropriate command ("destroy").
 *
 *
 * How to interact with this program:
 *  In you own program:
 *  -> the only way is to open a client socket on port (default to 4242, given by command argument on daemon start)
 *  -> then send the command on the socket
 *  -> then receive each command result from the socket (a line per result, separator is '\n')
 *  -> at this point, the daemon has already closed the socket, you can close yours too.
 *  Using appmon_client:
 *   - This tool doesn't exit yet!
 *
 *
 * Commands list:
 * - setup {working directory} {command_line_to_execute}
 -> the daemon checks path validity
 -> the daemon adds the new app
 -> the daemon returns the app_id
 * - start {app_id}
 *   -> the daemon first chdir to {working directory}
 *   -> the daemon forks
 *   -> child process is getting its own process group id, and becomes this process group leader
 *   -> then the command line is executed in child process like running it from a regular shell
 *      this command has to start the application you want to launch
 * - stop {app_id}
 *    -> send TERM signal to the process group of the corresponding child spawned before
 *       (i.e. all childs of this command will receive this signal)
 *    -> wait 5 seconds for SIGCHLD
 *    -> if SIGCHLD is received then "ok" is send on the socket
 *    -> if not, send KILL signal to the process group
 *    -> wait 5 seconds for SIGCHLD
 *    -> exit
 * - status {app_id}
 *    -> daemon sends application status (started or not), configurations (provided paths), and last exit info.
 *       (internal state is used to answer this request, no use of system tool/command (like ps).
 * - list
 *    -> daemon sends application status for all applications
 * - destroy
 *    -> stop all started commands, then stop the daemon.
 *
 *
 * Remarks:
 * - NO thread will be created to treat with the communication
 *   with the new client socket.
 *   So the appmon_daemon can be blocked if the connected client sends nothing.
 *   One possible evolution is to add timeout configuration on socket.
 *
 * - The previous remark also means that only one client can connect at the same time.
 *
 * - It is forbidden to provide shell command line with multiple command like "cmd1 & cmd2"
 *   The behavior of the  daemon is not defined in such case.
 *
 * - error reporting in socket result can be slightly improved.
 *
 * General working:
 *  while !stop
 *    client_skt = accept()
 *    while client_skt
 *      read(client_skt)
 *      execute_command()
 *    end
 *    close(client_skt)
 *  end
 *
 * Command execution monitoring:
 *  Each command is run in a child process created using fork.
 *  No thread is used to wait for the child process to die.
 *  SIGCHLD signal is used to be informed of a child death, then waitpid
 *  is used to get child status / exit code.
 *
 *
 *
 */

#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <sys/resource.h>
#include <netinet/in.h>
#include <errno.h>
#include <signal.h>
#include <pthread.h>
#include <sys/time.h>
#include <string.h>
#include <stdio.h>
#include <stdarg.h>
#include <pwd.h>
#include <grp.h>
#include <limits.h>
#include <poll.h>
#include <grp.h>
//internal includes
#include "swi_log.h"
#include "pointer_list.h"

// If the SVN Revision is known
#ifndef GIT_REV
#define GIT_REV "Unknown"
#endif //not GIT_REV

//commands definition
#define STOP_DAEMON "destroy"
#define STOP_APP "stop "
#define START_APP "start "
#define SETUP_APP "setup "
#define REMOVE_APP "remove "
#define STATUS_APP "status "
#define LIST_APPS "list"
#define SETENV "setenv "
#define PCONFIG "printconfig"

// exit code: 0-> success;  APPMON_ERR_EXIT_CODE-> internal error, else-> errno value that provoked exit.
#define APPMON_ERR_EXIT_CODE 1000
#define DEFAULT_LISTENING_PORT 4242 //default tcp port
#define RESTART_DELAY 5 //seconds
#define DEFAULT_ID 65534 // default uid,gid
//apps status, enum and string array to print enum...
typedef enum app_status_e
{
  STARTED, TO_BE_KILLED, TO_BE_RESTARTED, KILLED
} app_status;
static const char* app_status_str[] = { "STARTED", "STOPPING", "STARTING", "STOPPED" };

typedef struct app_struct
{
  pid_t pid;
  uint32_t id; //command id to be returned on start answer
  int32_t last_exit_code; //as filled by waitpid
  uint32_t start_count; // number of times that the app have been started (including auto restarts)
  app_status status;
  char* prog; // program command
  char* wd; // working directory
  char* last_exit_status; //string to describe app exit/death to external user
  int privileged; //application to be run with same user as appmon
} app_t;

//to store app_t's
static PointerList* apps = NULL;
//app id will from 1 to MAX_UINT32
static uint32_t next_app_id = 0;
//socket vars
static int srv_skt = 0, client_skt = 0;
//signals vars
static sigset_t block_sigs;
// output buffer to send results
static char* output_buf = NULL;
static size_t output_bufsize = 128;
// input buffer to read clients requests
static char* input_buf = NULL;
static size_t input_bufsize = 128;
//uid and gid to use to start unprivileged apps
static uid_t uid = 0;
static gid_t gid = 0;
//uid and gid to use to start privileged apps
static uid_t puid = 0;
static gid_t pgid = 0;
//cwd to be stored at init
static char* cwd = NULL;
//nice priority of unprivileged apps
//special value: INT_MAX, to define not to change the app priority.
static int app_priority = INT_MAX;

//forward declarations...
void SIGCHLD_handler(int s);
void SIGALRM_handler(int s);
static int clean_all();
static void err_exit(char* str);

/*utility tools*/

static void daemonize(void)
{
  pid_t pid;

  /* already a daemon */
  if (getppid() == 1)
    return;

  /* Fork off the parent process */
  pid = fork();
  if (pid < 0) //last sync error, afterwards errors will be reported using socket API.
    err_exit("daemonize:fork() failed");

  /* If we got a good PID, then we can exit the parent process. */
  if (pid > 0)
  {
    clean_all();
    exit(EXIT_SUCCESS);
  }
  //the child is the daemon, keep running
}

static char* readline(int fd)
{
  int i = 0;
  char c = 0;
  if (input_buf == NULL)
  {
    input_buf = malloc(input_bufsize);
    if (NULL == input_buf)
    {
      SWI_LOG("APPMON", ERROR, "readline: malloc error\n");
      return NULL;
    }
  }

  for (i = 0, c = 0; c != '\n'; i++)
  {
    if (i == input_bufsize)
    {
      input_bufsize += 128;
      input_buf = realloc(input_buf, input_bufsize);
      if (NULL == input_buf)
      {
        SWI_LOG("APPMON", ERROR, "readline: realloc error\n");
        return NULL;
      }
    }
    if (1 != read(fd, &c, 1))
    {
      //free(buf);
      return NULL;
    }
    input_buf[i] = c;
  }

  if (i > 1 && input_buf[i - 2] == '\r')
    i--;

  input_buf[i - 1] = '\0';
  return input_buf;
}

static char* fill_output_buf(char* fmt, ...)
{
  if (NULL == output_buf)
  {
    output_buf = malloc(output_bufsize);
    if (NULL == output_buf)
    {
      SWI_LOG("APPMON", ERROR, "fill_output_buf: malloc error\n");
      return NULL;
    }
  }
  va_list argp;
  va_start(argp, fmt);
  size_t towrite = vsnprintf(output_buf, output_bufsize, fmt, argp);
  va_end(argp);
  if (towrite > output_bufsize)
  {
    output_bufsize = towrite + 128;
    output_buf = realloc(output_buf, output_bufsize);
    if (NULL == output_buf)
    {
      SWI_LOG("APPMON", ERROR, "fill_output_buf: realloc error\n");
      return NULL;
    }
    va_start(argp, fmt);
    vsnprintf(output_buf, output_bufsize, fmt, argp);
    va_end(argp);
  }
  return output_buf;
}

static void send_result(char * res)
{
  SWI_LOG("APPMON", DEBUG, "send_result, res=%s\n", res);
  if (strlen(res) != write(client_skt, res, strlen(res)))
  {
    SWI_LOG("APPMON", ERROR, "cannot write res to socket\n");
  }
  char* ressep = "\n";
  if (strlen(ressep) != write(client_skt, ressep, strlen(ressep)))
  {
    SWI_LOG("APPMON", ERROR, "cannot write result separator to socket\n");
  }

}

//to be called within SIGCHLD handler
//set string to describe app exit/death to external user
//no consequence to internal workflow
static void app_exit_status(app_t* app)
{
  int32_t ecode = app->last_exit_code;
  app_status to_be_killed = app->status == TO_BE_KILLED;
  int exited = WIFEXITED(ecode);
  int signaled = WIFSIGNALED(ecode);
  int estatus = exited ? WEXITSTATUS(ecode) : -1;
  int signal = signaled ? WTERMSIG(ecode) : -1;
  char* res = "UNKNOWN"; //default val to ini

  if (!to_be_killed && exited && estatus)
    res = "EXIT_ERROR";
  if (!to_be_killed && exited && !estatus)
    res = "EXIT_REGULAR";
  if (to_be_killed && signaled && signal == SIGKILL)
    res = "STOP_KILL";
  if (to_be_killed && ((signaled && signal == SIGTERM) || (exited /*match any exit code*/)))
    res = "STOP_REGULAR";
  if (signaled && ((to_be_killed && signal != SIGTERM && signal != SIGKILL) || (!to_be_killed /*match any signal */)))
    res = "SIG_UNCAUGHT";

  app->last_exit_status = res;
}

static char* create_app_status(app_t* app)
{
  return fill_output_buf(
      "appname=[%d] privileged=[%d] prog=[%s] wd=[%s] status=[%s] pid=[%d] startcount=[%d] lastexittype=[%s] lastexitcode=[%d]",
      app->id, app->privileged, app->prog, app->wd, app_status_str[app->status], app->pid, app->start_count,
      app->last_exit_status, app->last_exit_code);
}

static char* check_params(char *wd, char* prog)
{
  struct stat st;
  int res = stat(wd, &st);
  char *res_str;
  if (res)
  {
    perror("stat on wd folder error");
    res_str = fill_output_buf("wd (%s) cannot be stat!", wd);
    SWI_LOG("APPMON", ERROR, "%s\n", res_str);
    return res_str;
  }

  if (!S_ISDIR(st.st_mode))
  {
    res_str = fill_output_buf("wd (%s) is not a directory!", wd);
    SWI_LOG("APPMON", ERROR, "%s\n", res_str);
    return res_str;
  }

  res = stat(prog, &st);
  if (res)
  {
    perror("stat on prog file error");
    res_str = fill_output_buf("prog (%s) cannot be stat!", prog);
    SWI_LOG("APPMON", ERROR, "%s\n", res_str);
    return res_str;
  }

  if (!S_ISREG(st.st_mode) || access(prog, X_OK))
  {
    perror("access on prog");
    res_str = fill_output_buf("prog (%s) is not an executable file!", prog);
    SWI_LOG("APPMON", ERROR, "%s\n", res_str);
    return res_str;
  }

  return NULL;
}

//call error if gid/uid operation failed, so no return value needed
//@param uid user id to set to current process, 0 means keep current(daemon) value
//@param gid group id to set to current process, 0 means keep current(daemon) value
//@param id internal id of the app (nothing related to posix call)
//Note: this function also sets supplementary group(s) when uid is provided
static void set_uid_gids(uid_t uid, gid_t gid, uint32_t id)
{
  SWI_LOG("APPMON", DEBUG, "set_uid_gids: uid=%d, gid%d\n", uid, gid);
  if (gid && setgid(gid))
  {
    SWI_LOG("APPMON", ERROR, "Child: id= %d, setgid failed :%s\n", id, strerror(errno));
    exit(EXIT_FAILURE);
  }
  if (uid)
  {

    //supplementary groups
    const struct passwd *user = NULL;
    user = getpwuid(uid);
    if (NULL == user)
    {
      SWI_LOG("APPMON", ERROR, "Child: id= %d, getpwuid failed :%s\n", id, strerror(errno));
      exit(EXIT_FAILURE);
    }
    //initgroup second arg is one more group to add (in addition to all supplementary groups found):
    //use provided gid or user default group.
    if (-1 == initgroups(user->pw_name, gid ? gid : user->pw_gid))
    {
      SWI_LOG("APPMON", ERROR, "Child: initgroups failed: %s\n", strerror(errno));
      exit(EXIT_FAILURE);
    }

    if (setuid(uid))
    {
      SWI_LOG("APPMON", ERROR, "Child: id= %d, setuid failed :%s\n", id, strerror(errno));
      exit(EXIT_FAILURE);
    }
  }
}

static char* start_app(app_t* app)
{
  char* wd = app->wd, *prog = app->prog;
  int id = app->id;
  SWI_LOG("APPMON", DEBUG, "start_app, id=%d, wd=%s; prog=%s\n", id, wd, prog);
  char * res = check_params(wd, prog);
  if (res) //check param errors
    return res;

  pid_t child_pid = fork();
  if (-1 == child_pid)
  {
    perror("start_app");
    res = "Fork error, cannot create new process";
    SWI_LOG("APPMON", ERROR, "%s\n", res);
    return res;
  }
  if (0 == child_pid)
  { //in child
    // close inherited stuff
    // note: sig handlers are reset
    close(srv_skt);
    close(client_skt);
    //child will actually run the application
    if (-1 == chdir(wd))
    {
      perror("cannot change working dir: chdir error");
      exit(EXIT_FAILURE);
    }

    setpgrp(); //equivalent to setpgrp(0, 0).
    //the point is that the child process is getting its own process group id, and becomes this process group leader
    //then on stop() daemon will send TERM signal to the process group of the child so that all children of this command will receive this signal.
    //so that all children of the application are killed too.
    //TODO: check if using setsid() can do the same thing, setsid might be more POSIX friendly (setpgrp() is indicated System V)
    SWI_LOG("APPMON", DEBUG, "Child: id= %d, pid=%d,  process group id set to = %d\n", id, getpid(), getpgrp());

    //change uid/gid/priority if necessary
    if (!app->privileged)
    {
      umask(S_IWOTH); //umask man reports that umask always succeeds...
      //SWI_LOG("APPMON", ERROR,"Failed to set umask")

      //may call error
      set_uid_gids(uid, gid, id);

      if (INT_MAX != app_priority)
      {
        //compute nice increment to give to nice
        int current_priority = getpriority(PRIO_PROCESS, 0);
        int nice_inc = app_priority - current_priority;
        //see nice man page, need to test errno manually
        errno = 0;
        int res = nice(nice_inc);
        if (errno)
        {
          SWI_LOG("APPMON", ERROR, "Child: id= %d, error while doing nice failed :%s, target priority was: %d, starting app with priority =%d\n",
                   id, strerror(errno), app_priority, getpriority(PRIO_PROCESS, 0));
        }
        else
        {
          if (res != app_priority)
            SWI_LOG("APPMON", ERROR, "Child: id= %d, nice failed : new priority=%d\n", id, res);
          else
            SWI_LOG("APPMON", DEBUG, "Child: id= %d, new priority=%d\n", id, res);
        }
      }
    }
    else //privileged app
    {
      set_uid_gids(puid, pgid, id);
      //no process priority change for privileged app
    }
    SWI_LOG("APPMON", DEBUG, "Child: id= %d, running with uid=%d gid=%d, eff uid=%d\n", id, getuid(), getgid(), geteuid());

    char* const argv[] = { prog, NULL };
    execvp(prog, argv);
    perror("");
    SWI_LOG("APPMON", ERROR, "Child: execvp has returned, error must have occurred\n");
    exit(EXIT_FAILURE);
  }
  //in daemon: everything is ok, update app infos.
  app->status = STARTED;
  app->pid = child_pid;
  app->start_count++;
  return "ok";
}

static char* stop_app(app_t* app)
{
  SWI_LOG("APPMON", DEBUG, "stop_app id=%d, pid %d\n", app->id, app->pid);
  sigset_t enable_chld_set;
  char* resstr = "ok";
  sigemptyset(&enable_chld_set);
  sigaddset(&enable_chld_set, SIGCHLD);

  // stopping an monitored app: trying to stop the process and any of his childs
  // using process group id, and spawned process have been set as process group id leader
  // i.e. pid=gpid
  app->status = TO_BE_KILLED;
  int signal = SIGTERM;
  int res = killpg(app->pid, SIGTERM);

  long time_to_wait = 5;
  struct timeval end, now;
  gettimeofday(&end, NULL);
  end.tv_sec += time_to_wait;
  gettimeofday(&now, NULL);
  struct timespec timeout;
  timeout.tv_sec = time_to_wait;
  timeout.tv_nsec = 0;
  do
  {
    SWI_LOG("APPMON", DEBUG, "stop_app: id=%d sigtimedwait: .... app->status=%d\n", app->id, app->status);
    errno = 0;
    res = sigtimedwait(&enable_chld_set, NULL, &timeout);
    if (res < 0)
    {
      if (errno == EINTR)
      {
        /* Interrupted by a signal other than SIGCHLD. */
        SWI_LOG("APPMON", DEBUG, "stop_app: id=%d Interrupted by a signal other than SIGCHLD, %d\n", app->id, app->status);
      }
      else if (errno == EAGAIN)
      {
        if (SIGKILL != signal)
        {
          //wait 5 more seconds for SIGCHLD after SIGKILL
          gettimeofday(&end, NULL);
          end.tv_sec += time_to_wait;
          signal = SIGKILL;
          SWI_LOG("APPMON", DEBUG, "stop_app: id=%d Timeout, killing child with SIGKILL\n", app->id);
          killpg(app->pid, SIGKILL);
        }
        else
        {
          //2nd and last timeout, exit anyway
          resstr = fill_output_buf("stop_app: id=%d Timeout, did not get SIGCHLD even after SIGKILL, pid=%d, name=%s",
              app->id, app->pid, app->prog);
          SWI_LOG("APPMON", ERROR, "%s\n", resstr);
          return resstr;
        }
      }
      else
      {
        perror("sigtimedwait");
        resstr = fill_output_buf("stop_app: sigtimedwait error: errno!=EINTR and errno!=EAGAIN errno=[%s]",
            strerror(errno));
        return resstr;
      }
    }
    if (res == SIGCHLD)
    {
      SWI_LOG("APPMON", DEBUG, "stop_app: res == SIGCHLD, app->status=%d\n", app->status);
      SWI_LOG("APPMON", DEBUG, "stop_app: manually calling SIGCHLD_handler\n");
      SIGCHLD_handler(0);
      SWI_LOG("APPMON", DEBUG, "stop_app: app->status=%d\n", app->status);
    }

    //update timeout
    gettimeofday(&now, NULL);
    timeout.tv_sec = end.tv_sec - now.tv_sec;
    timeout.tv_nsec = 0;
  } while (app->status != KILLED && timeout.tv_sec > 0);

  SWI_LOG("APPMON", DEBUG, "stop_app: exiting with status: %d[%d]\n", app->status, KILLED);

  return resstr;
}

static app_t* add_app(char* wd, char* prog, int privileged)
{
  app_t* app = malloc(sizeof(app_t) + strlen(prog) + 1 + strlen(wd) + 1);
  if (NULL == app)
  {
    SWI_LOG("APPMON", ERROR, "add_app error: malloc error=[%s]\n", strerror(errno));
    return NULL;
  }

  app->pid = -1;
  app->wd = (char*) app + sizeof(app_t);
  app->prog = (char*) app + sizeof(app_t) + strlen(wd) + 1;
  strcpy(app->prog, prog);
  strcpy(app->wd, wd);
  app->status = KILLED; //application is not running and can be started by default
  app->last_exit_code = -1; //never died yet!
  app->last_exit_status = "App has never died yet"; //never died yet!
  app->start_count = 0; //never started yet!
  app->id = ++next_app_id;
  app->privileged = privileged;
  rc_ReturnCode_t res = 0;
  if (RC_OK != (res = PointerList_PushLast(apps, app)))
  {
    SWI_LOG("APPMON", ERROR, "add_app error: PointerList_PushLast failed[AwtStatus=%d]\n", res);
    free(app);
    return NULL;
  }
  return app;
}

static app_t* find_by_pid(pid_t pid)
{
  app_t* app = NULL;
  unsigned int size, i = 0;
  PointerList_GetSize(apps, &size, NULL);
  for (i = 0; i < size; i++)
  {
    PointerList_Peek(apps, i, (void**) &app);
    if (app->pid == pid)
    {
      return app;
    }
  }
  return NULL; //not found !
}

static app_t* find_by_id(uint32_t id)
{
  app_t* app = NULL;
  unsigned int size, i = 0;
  PointerList_GetSize(apps, &size, NULL);
  for (i = 0; i < size; i++)
  {
    PointerList_Peek(apps, i, (void**) &app);
    if (app->id == id)
    {
      return app;
    }
  }
  return NULL; //not found !
}

static int clean_all()
{
  app_t* tmp = NULL;
  rc_ReturnCode_t res = RC_OK;
  if (apps != NULL)
  {

    unsigned int size, i = 0;
    //first apps list traversal: stopping apps
    //(cannot free them now as stop_app and SIGCHLD_handler functions
    //will traverse apps list too.
    PointerList_GetSize(apps, &size, NULL);
    for (i = 0; i < size; i++)
    {
      PointerList_Peek(apps, i, (void**) &tmp);
      if (tmp->status == STARTED || tmp->status == TO_BE_KILLED)
      {
        stop_app(tmp);
        //collect zombies, non blocking
        waitpid(tmp->pid, NULL, 0);
      }
    }
    //last apps list traversal: free-ing apps
    PointerList_GetSize(apps, &size, NULL);
    for (i = 0; i < size; i++)
    {
      PointerList_Peek(apps, i, (void**) &tmp);
      free(tmp);
    }

    res = PointerList_Destroy(apps);
  }

  //close sockets
  if (srv_skt)
    close(srv_skt);
  if (client_skt)
    close(client_skt);

  //no more socket to write to, free buffers
  if (input_buf)
  {
    free(input_buf);
    input_buf = NULL;
  }

  if (output_buf)
  {
    free(output_buf);
    output_buf = NULL;
  }

  if (cwd)
  {
    free(cwd);
    cwd = NULL;
  }
  SWI_LOG("APPMON", DEBUG, "clean_all end\n");
  return res;
}

void SIGALRM_handler(int s)
{
  SWI_LOG("APPMON", DEBUG, "SIGALRM_handler: pid=%d ======>\n", getpid());
  app_t* app = NULL;
  unsigned int size = 0, i = 0;
  PointerList_GetSize(apps, &size, NULL);
  SWI_LOG("APPMON", DEBUG, "SIGALRM_handler: started_apps size = %d\n", size);
  for (i = 0; i < size; i++)
  {
    PointerList_Peek(apps, i, (void**) &app);
    if (NULL != app && app->status == TO_BE_RESTARTED)
    {
      SWI_LOG("APPMON", DEBUG, "SIGALRM_handler: needs to restart %s\n", app->prog);
      char * res = start_app(app);
      if (!strcmp(res, "ok"))
      {
        SWI_LOG("APPMON", DEBUG, "SIGALRM_handler: %s restarted, new pid=%d\n", app->prog, app->pid);
      }
      else
      {
        SWI_LOG("APPMON", ERROR, "SIGALRM_handler: Cannot restart app id=%d, prog=%s, err=%s\n", app->id, app->prog, res);
        PointerList_Remove(apps, i, (void**) &app);
      }
    }
  }
  SWI_LOG("APPMON", DEBUG, "SIGALRM_handler: pid=%d ======<\n", getpid());
  fflush(stdout);
}

void SIGCHLD_handler(int s)
{
  int old_errno = errno;
  int child_status = 0, child_pid = 0;

  pid_t appmon_pid = getpid();
  SWI_LOG("APPMON", DEBUG, "SIGCHLD_handler: appmon_pid=%d =============>\n", appmon_pid);

  while (1)
  {
    do
    {
      errno = 0;
      //call waitpid trying to avoid suspend-related state changes.(no WUNTRACED or WCONTINUED options to waitpid)
      child_pid = waitpid(WAIT_ANY, &child_status, WNOHANG);
    } while (child_pid <= 0 && errno == EINTR);

    if (child_pid <= 0)
    {
      /* A real failure means there are no more
       stopped or terminated child processes, so return.  */
      errno = old_errno;
      SWI_LOG("APPMON", DEBUG, "SIGCHLD_handler: pid=%d =============< quit\n", appmon_pid);
      fflush(stdout);
      return;
    }

    app_t* app = find_by_pid(child_pid);
    if (NULL != app)
    {
      int exited = WIFEXITED(child_status);
      int exited_code = exited ? WEXITSTATUS(child_status) : -1;
      int signaled = WIFSIGNALED(child_status);

      //update exit status only if process is terminated.
      //if stopped/continued event on process is caught, don't update status
      if (!exited && !signaled)
      {
        SWI_LOG("APPMON", ERROR, "SIGCHLD_handler: status change looks like suspend events (STOP/CONT), ignored\n");
        continue; //go to next waipid call
      }

      //real child termination, update values
      app->last_exit_code = child_status;
      app_exit_status(app);

      SWI_LOG("APPMON", DEBUG, "SIGCHLD_handler: app terminated: id=%d, prog=%s was pid %d, calculated status =%s\n",
          app->id, app->prog, child_pid, app->last_exit_status);
      if (TO_BE_KILLED == app->status || (STARTED == app->status && exited && !exited_code))
      {
        //put flag to be used it stop_app
        app->status = KILLED;
        SWI_LOG("APPMON", DEBUG, "SIGCHLD_handler: status => KILLED\n");
      }
      else
      {
        if ((exited && exited_code > 0) | signaled)
        {
          SWI_LOG("APPMON", DEBUG, "SIGCHLD_handler: Child status error %d, application is set to  TO_BE_RESTARTED, %s\n",
              child_status, app->prog);
          //error has occur, restart app after delay
          // app will not be found if it has been stopped voluntarily, so it won't be restarted.
          app->status = TO_BE_RESTARTED;
          alarm(RESTART_DELAY);
        }
      }
    }
    else //app == NULL, pid not found in monitored apps
    {
      SWI_LOG("APPMON", DEBUG, "SIGCHLD_handler: unknown dead app, pid=%d\n", child_pid);
    }
  }
}

static void err_exit(char* str)
{
  int err = errno;
  if (err)
  {
    SWI_LOG("APPMON", ERROR, "err_exit:  strerror(errno)=[%s], ctx=[%s]\n", strerror(errno), str);
  }
  else
  {
    err = APPMON_ERR_EXIT_CODE;
    SWI_LOG("APPMON", ERROR, "err_exit: ctx=[%s]\n", str);
  }
  SWI_LOG("APPMON", ERROR, "cleaning and exiting\n");
  clean_all();
  exit(err);
}

/* if an integer was found in str: returns 0, the integer value is put in res
 * if an integer was not found in str:
 *    - if errmsg is not nul, then call *error* with that message
 *    - if errmsg is null, then return 1
 */
static int parse_arg_integer(char* str, int* res, char* errmsg)
{
  //code from strtol man page
  char *endptr;
  long val = 0;

  errno = 0; /* To distinguish success/failure after call */
  val = strtol(str, &endptr, 0);

  /* Check for various possible errors */

  if ((errno == ERANGE && (val == LONG_MAX || val == LONG_MIN)) || (errno != 0 && val == 0))
  {
    err_exit("parse_arg_integer error, strtol failed");
  }

  if (endptr == str)
  {
    if (errmsg)
    {
      err_exit(errmsg);
    }
    else
      return 1; //not found
  }
  /* If we got here, strtol() successfully parsed a number */

  if (*endptr != '\0') /* Not necessarily an error... */
    SWI_LOG("APPMON", DEBUG, "Further characters after number arg: %s\n", endptr);

  *res = (int) val;
  return 0;
}

static void get_uid_option(uint32_t* uid)
{
  if (parse_arg_integer(optarg, (int*) uid, NULL))
  {
    //cannot get uid as the number, trying to use command arg a user name.
    struct passwd * user = NULL;
    user = getpwnam(optarg);
    if (NULL == user)
    {
      err_exit("Command line arguments parsing: invalid user given.");
    }
    else
      *uid = user->pw_uid;
  }
  SWI_LOG("APPMON", DEBUG, "Command line arguments parsing: user uid=%d\n", *uid);
}

static void get_gid_option(uint32_t* gid)
{
  if (parse_arg_integer(optarg, (int*) gid, NULL))
  //cannot get gid as the number, trying to use command arg a group name.
  {
    struct group * grp = NULL;
    grp = getgrnam(optarg);
    if (NULL == grp)
    {
      err_exit("Command line arguments parsing: invalid group given");
    }
    else
      *gid = grp->gr_gid;
  }
  SWI_LOG("APPMON", DEBUG, "Command line arguments parsing: group gid=%d\n", *gid);
}

int main(int argc, char** argv)
{
  char *buffer = NULL;
  int portno = 0, stop, optval;
  socklen_t clilen;
  struct sockaddr_in serv_addr, cli_addr;
  struct sigaction action_CHLD;
  struct sigaction action_ALRM;

  /* Preliminary signal configuration:
   * SIGCHLD and SIGALRM are used with different handler
   * configure each handler to mask the other signal during it's process
   * Note: within a handler called upon a signal SIGX, the SIGX signal is
   * automatically de-activated.
   */

  sigemptyset(&block_sigs);
  sigaddset(&block_sigs, SIGCHLD);
  sigaddset(&block_sigs, SIGALRM);

  action_CHLD.sa_flags = SA_RESTART;
  action_CHLD.sa_handler = SIGCHLD_handler;
  sigemptyset(&(action_CHLD.sa_mask));
  sigaddset(&(action_CHLD.sa_mask), SIGALRM);

  action_ALRM.sa_flags = SA_RESTART;
  action_ALRM.sa_handler = SIGALRM_handler;
  sigemptyset(&(action_ALRM.sa_mask));
  sigaddset(&(action_ALRM.sa_mask), SIGCHLD);

  PointerList_Create(&apps, 0);
  if (NULL == apps)
    err_exit("PointerList_Create");

  /* Command line arguments Parsing
   * Options
   * a : privileged app path
   * w : privileged app working directory
   * v : user id to use to start privileged app
   * h : group id to use to start privileged app
   * p : TCP port to receive commands
   * u : user id to use to start regular apps
   * g : group id to use to start regular apps
   * n : nice value (process priority) for regular apps
   */
  char* init_app = NULL;
  char* init_app_wd = NULL;
  app_t* privileged_app = NULL;
  int opt;
  char useropt_given = 0;
  char grpopt_given = 0;
  //optarg is set by getopt
  while ((opt = getopt(argc, argv, "a:w:v:h:p:u:g:n:")) != -1)
  {
    switch (opt)
    {
      case 'a':
        init_app = optarg;
        SWI_LOG("APPMON", DEBUG, "Command line arguments parsing: init_app %s\n", init_app);
        break;
      case 'w':
        init_app_wd = optarg;
        SWI_LOG("APPMON", DEBUG, "Command line arguments parsing: init_app_wd %s\n", init_app_wd);
        break;
      case 'p':
        parse_arg_integer(optarg, &portno, "Command line arguments parsing: bad format for port argument");
        SWI_LOG("APPMON", DEBUG, "Command line arguments parsing: port =%d\n", portno);
        if (portno > UINT16_MAX)
        {
          err_exit("Command line arguments parsing: bad value for port, range=[0, 65535]");
        }
        break;
      case 'u':
        useropt_given = 1; //used to set default value after cmd line option parsing
        get_uid_option(&uid);
        break;
      case 'v':
        get_uid_option(&puid);
        break;
      case 'g':
        grpopt_given = 1; //used to set default value after cmd line option parsing
        get_gid_option(&gid);
        break;
      case 'h':
        get_gid_option(&pgid);
        break;
      case 'n':
        parse_arg_integer(optarg, &app_priority,
            "Command line arguments parsing: app process priority must be an integer");
        if (19 < app_priority || -20 > app_priority)
        {
          err_exit("Command line arguments parsing: app process priority must be between -20 and 19");
        }
        SWI_LOG("APPMON", DEBUG, "Command line arguments parsing: nice increment =%d\n", app_priority);
        break;
      default: /* '?' */
        SWI_LOG("APPMON", ERROR, "Command line arguments parsing: unknown argument\n");
        break;
    }

  }
  if (NULL != init_app)
  {

    if (NULL == init_app_wd)
    { //using current working directory as privileged app wd.
      cwd = malloc(PATH_MAX);
      if (NULL == cwd)
      {
        err_exit("Cannot malloc init_app_wd");
      }
      cwd = getcwd(cwd, PATH_MAX);
      if (NULL == cwd)
      {
        err_exit("getcwd failed to guess privileged app default wd");
      }
      init_app_wd = cwd;
    }
    char * res = check_params(init_app_wd, init_app);
    if (NULL != res)
    {
      SWI_LOG("APPMON", ERROR, "check_params on privileged app failed: %s\n", res);
      err_exit("check_params on privileged app failed");
    }
    privileged_app = add_app(init_app_wd, init_app, 1);
    if (NULL == privileged_app)
    {
      err_exit("add_app on privileged app failed");
    }
  }

  if (!uid && !useropt_given)
  { //get default "nobody" user.
    uid = 65534;
  }

  if (!gid && !grpopt_given)
  { //get default "nogroup" group.
    gid = 65534;
  }

  SWI_LOG("APPMON", DEBUG, "Command line arguments parsing: will use uid=%d and gid=%d to run unprivileged apps\n",
      uid, gid);

  /* configuring signals handling */
  if (sigaction(SIGCHLD, &action_CHLD, NULL))
    err_exit("configuring signals handling: sigaction SIGCHLD call error");

  if (sigaction(SIGALRM, &action_ALRM, NULL))
    err_exit("configuring signals handling: sigaction SIGCHLD call error");

  srv_skt = socket(AF_INET, SOCK_STREAM, 0);
  if (srv_skt < 0)
    err_exit("socket configuration: opening socket error");

  // set SO_REUSEADDR on socket:
  optval = 1;
  if (setsockopt(srv_skt, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof optval))
    err_exit("socket configuration: setting SO_REUSEADDR on socket failed");

  bzero((char *) &serv_addr, sizeof(serv_addr));
  portno = portno ? portno : DEFAULT_LISTENING_PORT;

  serv_addr.sin_family = AF_INET;
  serv_addr.sin_addr.s_addr = INADDR_ANY;
  serv_addr.sin_port = htons(portno);

  if (bind(srv_skt, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0)
    err_exit("socket configuration: error on binding");

  if (listen(srv_skt, 5))
    err_exit("socket configuration: error on listen");

  clilen = sizeof(cli_addr);
  stop = 0;

  SWI_LOG("APPMON", DEBUG, "Init successful, now running as daemon.\n");
  /* daemonize the later possible to enhance sync error reporting*/
  daemonize();
  /* Now we are a simple daemon */
  SWI_LOG("APPMON", DEBUG, "Daemon pid=%d, Listening port = %d\n", getpid(), portno);

  if (privileged_app)
  {
    SWI_LOG("APPMON", DEBUG, "Autostarting privileged app\n");
    start_app(privileged_app);
  }

  while (!stop)
  {
    fflush(stdout);
    client_skt = accept(srv_skt, (struct sockaddr *) &cli_addr, &clilen);
    if (client_skt < 0)
    {
      SWI_LOG("APPMON", ERROR, "socket configuration: error on accept: %s\n", strerror(errno));
      SWI_LOG("APPMON", ERROR, "Now going to crippled mode: cannot use socket API anymore!\n");
      if (client_skt)
        close(client_skt);
      if (srv_skt)
        close(srv_skt);
      // Sleep for 1.5 sec
      // sleep() function not used here, as it may disrupt the use of SIGALRM made in this program.
      struct timeval tv;
      while (1)
      {
        tv.tv_sec = 1;
        tv.tv_usec = 0;
        int res = select(0, NULL, NULL, NULL, &tv);
        SWI_LOG("APPMON", DEBUG, "crippled mode: select, res = %d\n", res);
      }
      //never returning from here, need to kill the daemon
      // but apps should still be managed.
    }

    SWI_LOG("APPMON", DEBUG, "new client ...\n");
    buffer = readline(client_skt);

    //deal with all the requests coming from the new client
    while (NULL != buffer && !stop)
    {
      SWI_LOG("APPMON", DEBUG, "NEW cmd=[%s]\n", buffer);
      do
      {
        if (!strncmp(buffer, STOP_DAEMON, strlen(STOP_DAEMON)))
        {
          stop = 1;
          send_result("ok, destroy is in progress, stopping aps, closing sockets.");
          break;
        }
        if (!strncmp(buffer, PCONFIG, strlen(PCONFIG)))
        {
          send_result(
              fill_output_buf(
                  "appmon_daemon: version[%s], uid=[%d], gid=[%d], puid=[%d], pgid=[%d], app_priority=[%d]",
                  GIT_REV, uid, gid, puid, pgid, app_priority));
          break;
        }
        if (!strncmp(buffer, SETUP_APP, strlen(SETUP_APP)))
        {
          char* buf = buffer;
          strsep(&buf, " ");
          char* wd = strsep(&buf, " ");
          char* prog = strsep(&buf, " ");

          SWI_LOG("APPMON", DEBUG, "SETUP wd =%s, prog = %s\n", wd, prog);
          if (NULL == wd || NULL == prog)
          {
            send_result("Bad command format, must have wd and prog params");
            break;
          }
          char *res = check_params(wd, prog);
          if (res)
          {
            send_result(res);
            break;
          }
          sigprocmask(SIG_BLOCK, &block_sigs, NULL);
          app_t* app = add_app(wd, prog, 0);
          if (NULL == app)
            send_result("Cannot add app");
          else
            send_result(fill_output_buf("%d", app->id));

          sigprocmask(SIG_UNBLOCK, &block_sigs, NULL);

          break;
        }
        if (!strncmp(buffer, START_APP, strlen(START_APP)))
        {
          char* str_id = buffer + strlen(START_APP);
          int id = atoi(str_id);
          SWI_LOG("APPMON", DEBUG, "START_APP, id =%d\n", id);
          if (id == 0)
          {
            send_result("Bad command format, start called with invalid app id");
            break;
          }
          sigprocmask(SIG_BLOCK, &block_sigs, NULL);
          app_t* app = find_by_id(id);
          if (app == NULL)
          {
            send_result("Unknown app");
            sigprocmask(SIG_UNBLOCK, &block_sigs, NULL);
            break;
          }
          if (app->privileged)
          {
            send_result("Privileged App, cannot act on it through socket.");
            sigprocmask(SIG_UNBLOCK, &block_sigs, NULL);
            break;
          }
          if (app->status != KILLED)
          {
            send_result("App already running (or set to be restarted), start command discarded");
            sigprocmask(SIG_UNBLOCK, &block_sigs, NULL);
            break;
          }
          send_result(start_app(app));
          sigprocmask(SIG_UNBLOCK, &block_sigs, NULL);
          break;
        }

        if (!strncmp(buffer, STOP_APP, strlen(STOP_APP)))
        {
          char* str_id = buffer + strlen(STOP_APP);
          int id = atoi(str_id);
          if (id == 0)
          {
            send_result("Bad command format, stop called with invalid app id");
            break;
          }
          sigprocmask(SIG_BLOCK, &block_sigs, NULL);
          app_t* app = find_by_id(id);
          if (NULL == app)
            send_result("Unknown app");
          else
          {
            if (app->privileged)
            {
              send_result("Privileged App, cannot act on it through socket.");
              sigprocmask(SIG_UNBLOCK, &block_sigs, NULL);
              break;
            }

            //stop command has effect only if application is running.
            if (app->status == STARTED || app->status == TO_BE_KILLED)
            {
              send_result(stop_app(app));
            }
            else
            { //application is already stopped (app->status could be KILLED or TO_BE_RESTARTED)
              app->status = KILLED; //force app->status =  KILLED, prevent app to be restarted if restart was scheduled. (see SIG ALRM handler)
              send_result("ok, already stopped, won't be automatically restarted anymore");
            }

          }
          sigprocmask(SIG_UNBLOCK, &block_sigs, NULL);
          break;
        }

        if (!strncmp(buffer, REMOVE_APP, strlen(REMOVE_APP)))
        {
          char* str_id = buffer + strlen(REMOVE_APP);
          int id = atoi(str_id);
          if (id == 0)
          {
            send_result("Bad command format, remove called with invalid app id");
            break;
          }
          sigprocmask(SIG_BLOCK, &block_sigs, NULL);
          app_t* app = find_by_id(id);
          if (NULL == app)
            send_result("Unknown app");
          else
          {
            if (app->privileged)
            {
              send_result("Privileged App, cannot act on it through socket.");
              sigprocmask(SIG_UNBLOCK, &block_sigs, NULL);
              break;
            }
            //stop command has effect only if application is running.
            if (app->status == STARTED || app->status == TO_BE_KILLED)
            {
              stop_app(app); //trying to stop, no big deal with it fails
            }

            app_t* app = NULL;
            unsigned int size, i = 0;
            PointerList_GetSize(apps, &size, NULL);
            for (i = 0; i < size; i++)
            {
              PointerList_Peek(apps, i, (void**) &app);
              if (app->id == id)
              {
                rc_ReturnCode_t res = 0;
                if (RC_OK != (res = PointerList_Remove(apps, i, (void**) &app)))
                {
                  send_result(fill_output_buf("Remove: PointerList_Remove failed, AwtStatus =%d", res));
                }
                else
                {
                  free(app);
                  send_result("ok");
                }
                break;
              }
            }
          }
          sigprocmask(SIG_UNBLOCK, &block_sigs, NULL);
          break;
        }

        if (!strncmp(buffer, STATUS_APP, strlen(STATUS_APP)))
        {
          char* str_id = buffer + strlen(STATUS_APP);
          int id = atoi(str_id);
          if (id == 0)
          {
            send_result("Bad command format, status called with invalid app id");
            break;
          }

          sigprocmask(SIG_BLOCK, &block_sigs, NULL);
          app_t* app = find_by_id(id);
          if (NULL == app)
            send_result("Unknown app");
          else
          {
            SWI_LOG("APPMON", DEBUG, "sending app status...\n");
            send_result(create_app_status(app));
          }
          sigprocmask(SIG_UNBLOCK, &block_sigs, NULL);
          break;
        }
        if (!strncmp(buffer, LIST_APPS, strlen(LIST_APPS)))
        {
          SWI_LOG("APPMON", DEBUG, "sending app list ...\n");
          sigprocmask(SIG_BLOCK, &block_sigs, NULL);
          app_t* app = NULL;
          unsigned int size, i = 0;
          PointerList_GetSize(apps, &size, NULL);
          for (i = 0; i < size; i++)
          {
            PointerList_Peek(apps, i, (void**) &app);
            char* app_status_tmp = create_app_status(app);
            if (strlen(app_status_tmp) != write(client_skt, app_status_tmp, strlen(app_status_tmp)))
            {
              SWI_LOG("APPMON", ERROR, "list: cannot write res to socket\n");
            }
            SWI_LOG("APPMON", DEBUG, "list: send status, app_status_tmp=%s\n", app_status_tmp);
            char* statussep = "\t";
            if (strlen(statussep) != write(client_skt, statussep, strlen(statussep)))
            {
              SWI_LOG("APPMON", ERROR, "list: cannot write statussep: %s\n", statussep);
            }
          }
          send_result("");
          sigprocmask(SIG_UNBLOCK, &block_sigs, NULL);
          break;
        }
        if (!strncmp(buffer, SETENV, strlen(SETENV)))
        {
          char *arg, *varname, *tmp;

          arg = buffer + strlen(SETENV);
          varname = arg;
          tmp = strchr(arg, '=');
          *tmp++ = '\0';

          SWI_LOG("APPMON", DEBUG, "Setting Application framework environment variable %s = %s...\n", varname, tmp);
          setenv(varname, tmp, 1);

          send_result("");
          break;
        }

        //command not found
        send_result("command not found");
        SWI_LOG("APPMON", DEBUG, "Command not found\n");
      } while (0);

      if (stop)
        break;

      //read some data again to allow to send several commands with the same socket
      buffer = readline(client_skt);

    } //end while buffer not NULL: current client has no more data to send

    //current client exited, let's close client skt, wait for another connexion
    close(client_skt);
  }

  sigprocmask(SIG_BLOCK, &block_sigs, NULL);
  int exit_status_daemon = clean_all();
  SWI_LOG("APPMON", DEBUG, "appmon daemon end, exit_status_daemon: %d\n", exit_status_daemon);
  return exit_status_daemon;
}
