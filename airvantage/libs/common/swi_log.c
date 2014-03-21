/*******************************************************************************
 * Copyright (c) 2012 Sierra Wireless and others.
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the Eclipse Public License v1.0
 * which accompanies this distribution, and is available at
 * http://www.eclipse.org/legal/epl-v10.html
 *
 * Contributors:
 *     Romain Perier for Sierra Wireless - initial API and implementation
 *******************************************************************************/

#include "swi_log.h"

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdarg.h>
#include <string.h>
#include <time.h>

#define WARNING 2
#define DEBUG 5
#define ALL 6
#define OUT_OF_BOUNDS 7

#define unlikely(x) __builtin_expect(!!(x), 0)

typedef struct md_list
{
  char *module;
  swi_log_level_t level;
  struct md_list *next;
} md_list_t;


static void default_displaylogger(const char *module, swi_log_level_t severity, const char *message);

swi_log_displaylogger_t swi_log_displaylogger = default_displaylogger;
swi_log_storelogger_t swi_log_storelogger;
static const char *swi_log_format = "%t %m-%s: %l";
static swi_log_level_t default_level = WARNING;
static md_list_t *md_list;
static char format_cached[64];
static const char *levels[] = {
  "NONE",
  "ERROR",
  "WARNING",
  "INFO",
  "DETAIL",
  "DEBUG",
  "ALL"
};

static swi_log_level_t str2Severity(const char *str)
{
  int i;

  for (i = 0; i < 7; i++)
    if (!strcasecmp(levels[i], str))
      return (swi_log_level_t)i;
  return NONE;
}

static void changeVerbosity(const char *module, swi_log_level_t *level)
{
  char *var, *str, *ptr, *tmp, *sep;

  var = getenv("SWI_LOG_VERBOSITY");

  if (var == NULL)
    return;

  ptr = strdup(var);
  str = ptr;
  while((tmp = strtok(str, ",")))
  {
    sep = strchr(tmp, ':');
    if (sep == NULL)
    {
      str = NULL;
      continue;
    }
    *sep = '\0';
    if (!strcasecmp(tmp, module))
    {
      *level = str2Severity(sep+1);
      goto end;
    }
    else if (!strcasecmp(tmp, "default"))
    {
      default_level = str2Severity(sep+1);
      goto end;
    }
    str = NULL;
  }
end:
  free(ptr);
}

static void default_displaylogger(const char *module, swi_log_level_t severity, const char *message)
{
  const char *f = "\e[0m";
  char *var;

  var = getenv("SWI_LOG_COLOR");

  if (var && !strcmp(var, "0"))
    goto display;

  switch(severity)
  {
    case ERROR:
      f = "\e[31;1m";
      break;
    case WARNING:
      f = "\e[33;1m";
      break;
    case DEBUG:
      f = "\e[37;1m";
      break;
    case INFO:
      f = "\e[32;2m";
      break;
    case DETAIL:
       f = "\e[36;2m";
      break;
    default:
      break;
  }
display:
  fprintf(stderr, "%s%s\e[0m", f, message);
}

static void loggers(const char *module, swi_log_level_t severity, const char *message)
{
  if (swi_log_displaylogger)
    swi_log_displaylogger(module, severity, message);
  if (swi_log_storelogger)
    swi_log_storelogger(module, severity, message);
}

static void destroy()
{
  md_list_t *entry, *tmp;

  entry = md_list;
  while(entry)
  {
    tmp = entry->next;
    free(entry->module);
    free(entry);
    entry = tmp;
  }
}

static void newlevel(const char *module, swi_log_level_t level)
{
  md_list_t *entry;
  int len;

  if (md_list == NULL)
  {
    md_list = malloc(sizeof(md_list_t));
    entry = md_list;
    atexit(destroy);
  }
  else
  {
    for (entry = md_list; entry->next; entry = entry->next)
      ;
    entry->next = malloc(sizeof(md_list_t));
    entry = entry->next;
  }
  len = strlen(module);
  entry->module = malloc(len + 1);
  memcpy(entry->module, module, len);
  entry->module[len] = '\0';
  entry->level = level;
  entry->next = NULL;
}

static char * replace(const char * src, const char *occur, const char *by)
{
  const char *str;
  char *token, *result;
  int src_len, occur_len, by_len, result_len, len = 0;

  result_len = strlen(src) - strlen(occur) + strlen(by);
  src_len = strlen(src);
  occur_len = strlen(occur);
  by_len = strlen(by);

  result = malloc(result_len + 1);
  str = src;
  while ((token = strstr(str, occur)))
  {
    memcpy(result + len, src, token - src);
    len += (token - src);

    memcpy(result + len, by, by_len);
    len += by_len;

    memcpy(result + len, token+occur_len, (src + src_len) - (token + occur_len));
    len += (src + src_len) - (token + occur_len);

    str += (token - str) + occur_len;
  }
  result[result_len] = '\0';
  return result;
}

static char *format_message(const char *module, swi_log_level_t severity, const char *message)
{
  const char *format;
  char *pass1 = NULL, *pass2 = NULL, *pass3 = NULL, buf[32], *formatted_message = NULL;
  static time_t s_t;
  time_t t;
  struct tm *tm;

  format = swi_log_format ? swi_log_format : "%t %m-%s: %l";

  t = time(NULL);
  if (s_t == t && strstr(format_cached, module) && strstr(format_cached, levels[severity]))
  {
    pass3 = format_cached;
    goto pass_3;
  }

  s_t = t;
  tm = localtime(&t);
  memset(buf, 0, sizeof(buf));
  strftime(buf, 32, "%F %H:%M:%S", tm);
  pass1 = replace(format, "%t", buf);
  pass2 = replace(pass1, "%m", module);
  pass3 = replace(pass2, "%s", levels[severity]);
  memcpy(format_cached, pass3, strlen(pass3) + 1);
pass_3:
  formatted_message = replace(pass3, "%l", message);

  free(pass1);
  free(pass2);
  if (pass3 != format_cached)
    free(pass3);
  return formatted_message;
}

void swi_log_setlevel(swi_log_level_t level, const char *module, ...)
{
  va_list ap;
  const char *mod = NULL;

  va_start(ap, module);

  if (module == NULL)
  {
    default_level = level;
    return;
  }

  newlevel(module, level);
  while((mod = va_arg(ap, const char *)))
    newlevel(mod, level);
  va_end(ap);
}

int swi_log_musttrace(const char *module, swi_log_level_t severity)
{
  md_list_t *entry;
  swi_log_level_t lev;
  static __thread char cache[64];

  if (unlikely(module == NULL))
    return 0;

  if(!strcmp(cache, module))
    return severity <= (cache[ (uint8_t)cache[63] ] - '0');

  for (entry = md_list; entry; entry = entry->next)
  {
    if (!strcmp(entry->module, module))
    {
      lev = entry->level;
      break;
    }
  }
  if (entry == NULL)
    lev = default_level;

  changeVerbosity(module, &lev);

  memset(cache, 0, 64);
  memcpy(cache, module, strlen(module));
  cache[63] = strlen(module);
  cache[ (uint8_t)cache[63] ] = lev + '0';

  return severity <= lev;
}

void swi_log_trace(const char *module, swi_log_level_t severity, const char *format, ...)
{
  char *user_message = NULL, *formatted_message = NULL;
  va_list ap;
  int ret;

  if (!swi_log_musttrace(module, severity))
    return;

  va_start(ap, format);

  ret = vasprintf(&user_message, format, ap);
  (void)ret;
  formatted_message = format_message(module, severity, user_message);

  loggers(module, severity, formatted_message);

  free(formatted_message);
  free(user_message);
  va_end(ap);
}

void swi_log_setformat(const char *format)
{
  swi_log_format = format;
  memset(format_cached, 0, sizeof(format_cached));
}
