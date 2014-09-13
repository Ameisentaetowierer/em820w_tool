#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <signal.h>
#include <sys/inotify.h>
#include <cutils/properties.h>
#include <cutils/logprint.h>

// global Options
int g_verbose = 0;
int g_daemon  = 0;
int g_loop    = 0;
int g_kill    = 0;
char *g_svc   = '\0';
char g_curval[1024];
char g_vers[16] = "V1.6";
char g_date[16] = "30.03.2013";

int g_ifd;
int g_wfd;



/*
  ANDROID_LOG_VERBOSE;
  ANDROID_LOG_DEBUG;
  ANDROID_LOG_INFO;
  ANDROID_LOG_WARN;
  ANDROID_LOG_ERROR;
  ANDROID_LOG_FATAL;
*/

void logging(int prio, char *logtxt) {
  const char *tag = "em820w_tool";
  if(g_daemon == 0) {
    if (g_verbose == 1 || prio >= ANDROID_LOG_WARN || prio < 0) {
      write(STDERR_FILENO, logtxt, strlen(logtxt));
      fsync(STDERR_FILENO);
      return;
    }
  } else {
    if(g_verbose == 1 || prio >= ANDROID_LOG_WARN) {
      (void) __android_log_buf_print(LOG_ID_RADIO, prio, tag, "%s", logtxt);
      // __android_log_print(prio, tag, "%s", logtxt);
    }
  }
}


static void sighdl(int signo) {
  char v_buf[512];

  sprintf(v_buf, "Signal %d received\n", signo);
  logging(ANDROID_LOG_DEBUG, v_buf);
  g_loop = 0;
  return;
}


int get_mode(void) {
  char buf[2049];
  int fd;
  int ret;
  char *spos;
  long bpos=0;
  int v_flg;
  int v_max = 0;

  while(v_max < 2) {
    fd=open("/proc/tty/driver/usbserial", O_RDONLY);
    if(fd < 0) {
      logging(ANDROID_LOG_ERROR, "can't open /proc/tty/driver/usbserial for reading\n");
      exit(1);
    }

    v_flg = fcntl(fd, F_GETFL);
    (void) fcntl(fd, F_SETFL, v_flg | O_NONBLOCK);
    ret = read(fd, buf, 2048);
    close(fd);
    buf[ret] = '\0';
    // logging(ANDROID_LOG_DEBUG, buf);
    spos = strstr(buf, ":1003 ");
    if(spos != NULL) {
      return(1003);
    }
    spos = strstr(buf, ":1404 ");
    if(spos != NULL) {
      return(1404);
    }
    sleep(2);
    v_max++;
  }
  return(-1);
}

void getport(char *pname) {
  int pn=0;
  int hit=0;
  int ret;
  int v_sys;
  struct stat s_stat;

  while(hit < 2 &&  pn < 20) {
    sprintf(pname, "/dev/ttyUSB%d", pn);
    ret = stat(pname, &s_stat);
    if(ret == 0) {
      hit++;
    }
    pn++;
  }
  if(hit < 2) {
    pname = NULL;
  }
  return;
}


int write_val(char *filename, char *value) {
  int v_fd;
  int v_ret;
  char v_buf[1024];
  v_fd = open(filename, O_WRONLY);
  if(v_fd < 0) {
    sprintf(v_buf, "can't open %s for writing!\n", filename);
    logging(ANDROID_LOG_ERROR, v_buf);
    return(-1);
  } else {
    v_ret = write(v_fd, value, strlen(value));
    close(v_fd);
    sprintf(v_buf, "%d out of %d bytes written to %s\n", v_ret, strlen(value), filename);
    logging(ANDROID_LOG_DEBUG, v_buf);
    return(v_ret == strlen(value));
  }
}

void read_val(char *filename, char *p_buf) {
  int v_fd;
  int v_ret;
  int v_flg;
  char v_buf[1024];
  v_fd = open(filename, O_RDONLY);
  if(v_fd < 0) {
    sprintf(v_buf, "can't open %s for reading!\n", filename);
    p_buf='\0';
    logging(ANDROID_LOG_ERROR, v_buf);
    return;
  } else {
   
    v_flg = fcntl(v_fd, F_GETFL);
    (void) fcntl(v_fd, F_SETFL, v_flg | O_NONBLOCK);
    v_ret = read(v_fd, p_buf, 1023);
    p_buf[v_ret]='\0';
    close(v_fd);
    return;
  }
}
    

void set_1404(void) {
  char v_rilsvc[] = "ril-daemon";
  char *p_rilsvc = v_rilsvc;
  char p_name[64];
  char v_buf[1024];
  int v_ret;
  int v_max = 0;

  if(get_mode() == 1404) {
    logging(ANDROID_LOG_DEBUG, "modem already has USB-PID 1404!\n");
    return;
  }

  if(g_svc != '\0') {
    p_rilsvc = g_svc;
  }

  logging(ANDROID_LOG_WARN, "setting modem to USB-PID 1404 ...\n");

  // v_ret = write_val("/sys/bus/usb/devices/usb1/power/control", "on\n");
  if(g_kill == 0) {
    sprintf(v_buf, "stopping service %s ...\n", p_rilsvc);
    logging(ANDROID_LOG_DEBUG, v_buf);
    property_set("ctl.stop", p_rilsvc);
  }
  v_ret = write_val("/sys/EcControl/ThreeGPower", "0\n");
  v_max = 10;
  getport(p_name);
  while(p_name != NULL && v_max > 0) {
    getport(p_name);
    v_max--;
    sleep(1);
  }
  v_ret = write_val("/sys/EcControl/ThreeGPower", "1\n");
  v_max = 10;
  getport(p_name);
  while(p_name == NULL && v_max > 0) {
    getport(p_name);
    v_max--;
    sleep(1);
  }
  v_max=0;
  while(get_mode() != 1404 && v_max < 5) {
    v_ret = write_val("/sys/bus/usb/devices/usb1/power/control", "on\n");
    sleep(1 + v_max);
    v_max++;
  }
  v_ret = write_val("/sys/bus/usb/devices/1-1/power/autosuspend", "2\n");
  v_ret = write_val("/sys/bus/usb/devices/1-1/power/control", "on\n");
  v_ret = write_val("/sys/bus/usb/devices/1-1/power/wakeup", "enabled\n");
  v_ret = write_val("/sys/bus/usb/devices/usb1/power/control", "auto\n");
  if(strlen(g_curval) > 0) {
    v_ret = write_val("/sys/bus/usb/devices/1-1/power/autosuspend", g_curval);
  } else {
    v_ret = write_val("/sys/bus/usb/devices/1-1/power/autosuspend", "2\n");
  }
   

/*
  sleep(5);
  v_max=0;
  while(get_mode() != 1404 && v_max < 5) {
    v_ret = write_val("/sys/bus/usb/devices/usb1/power/control", "on\n");
    sleep(1 + v_max);
    v_max++;
  }
*/
  if(g_kill == 0) {
    sprintf(v_buf, "starting service %s ...\n", p_rilsvc);
    logging(ANDROID_LOG_DEBUG, v_buf);
    property_set("ctl.start", p_rilsvc);
  } else {
    logging(ANDROID_LOG_DEBUG, "killing rild...\n");
    system("killall rild");
  }
  if(get_mode() == 1404) {
    logging(ANDROID_LOG_WARN, "Modem was set to USB-PID 1404 successfuly\n");
  } else {
    logging(ANDROID_LOG_ERROR, "ERROR! Modem was not set to USB-PID 1404\n");
  }
}



void set_1003(void) {
  char v_tty[128];
  char v_buf[512];
  int v_ret;
  char msg1[] = { 'A','T','$','Q','C','D','M','G',0x0d,0x0a,0x00 };
  char msg2[] = { 'A','T','^','D','I','S','L','O','G','=','2','5','5',0x0d,0x0a,0x00 };
  char msg3[] = { 0x7e,0x3a,0xa1,0x6e,0x7e,0x00 };

  if(get_mode() != 1404) {
    logging(ANDROID_LOG_ERROR, "modem doesn't have USB-PID 1404!\n");
    exit(1);
  }
  logging(ANDROID_LOG_WARN, "setting modem to USB-PID 1003 ...\n");
  
  getport(v_tty);
  if(v_tty == NULL) {
    logging(ANDROID_LOG_ERROR, "can't find valid tty-port\n");
    exit(1);
  }

  v_ret = write_val(v_tty, msg1);
  v_ret = write_val(v_tty, msg2);
  v_ret = write_val(v_tty, msg3);
  sleep(7);

  v_ret = get_mode();
  if(v_ret != 1003) {
    sprintf(v_buf, "ERROR! Modem was not set to USB-PID 1003, >%d< was reported instead\n", v_ret);
    logging(ANDROID_LOG_ERROR, v_buf);
    exit(1);
  }
  logging(ANDROID_LOG_WARN, "Modem was set to USB-PID 1003 successfuly\n");
  exit(0);
}

void show_mode(void) {
  printf("current mode >%d<\n", get_mode());
}


void daemonize(void) {
  int v_ret;
  pid_t v_pid;
  char v_name[128];
  char v_buf[512];
  struct inotify_event iev;
  struct sigaction    actions;

  v_pid = fork();

  if(v_pid > 0) {
    exit(0);
  }

  sprintf(v_buf,"emw820w_tool (%s) daemonizing ...\n", g_vers);
  logging(ANDROID_LOG_ERROR, v_buf);
  g_loop=1;
  
  close(STDIN_FILENO);
  close(STDOUT_FILENO);
  close(STDERR_FILENO);


  memset(&actions, 0, sizeof(actions));
  sigemptyset(&actions.sa_mask);
  actions.sa_flags = 0;
  actions.sa_handler = sighdl;
  sigaction(SIGTERM,& actions, NULL);

  g_ifd = inotify_init();
  if(g_ifd < 0) {
    sprintf(v_buf,"inotify_init() error %d\n", errno);
    logging(ANDROID_LOG_ERROR, v_buf);
    exit(1);
  }

  logging(ANDROID_LOG_DEBUG, "entering watchdog mode ...\n");

  if(get_mode() != 1404) {
    set_1404();
    sleep(2);
  }
  read_val("/sys/bus/usb/devices/1-1/power/autosuspend", g_curval);

  while(g_loop == 1) {
    getport(v_name);
    
    g_wfd = inotify_add_watch(g_ifd, v_name, IN_DELETE_SELF);
    if(g_wfd < 0) {
      sprintf(v_buf,"watchdog: inotify_add_watch() error %d\n", errno);
      logging(ANDROID_LOG_ERROR, v_buf);
      sleep(2);
    } else {
      sprintf(v_buf,"watchdog for %s initialized\n", v_name);
      logging(ANDROID_LOG_DEBUG, v_buf);
  
      iev.mask = 0;
      while(iev.mask != IN_DELETE_SELF) {
        v_ret=read(g_ifd, &iev, sizeof(iev));
        if(g_loop == 0) {
          break;
        }
      } 
      v_ret = inotify_rm_watch(g_ifd, g_wfd);
      if(g_loop == 1) {
        sleep(5);
        if((v_ret = get_mode()) != 1404) {
          logging(ANDROID_LOG_DEBUG,"3G crash detected!\n");
          logging(ANDROID_LOG_DEBUG,"activating wakelock\n");
          v_ret = write_val("/sys/power/wake_lock", "em820w_tool\n");
          while (v_ret != 1404) {
            set_1404();
            sleep(10);
            v_ret = get_mode();
          }
          logging(ANDROID_LOG_DEBUG,"deactivating wakelock\n");
          v_ret = write_val("/sys/power/wake_unlock", "em820w_tool\n");
        }
        sleep(2);
      }
    }
  }

  logging(ANDROID_LOG_DEBUG,"exiting...\n");
  close(g_wfd);
  close(g_ifd);
  exit(0);
}

void helpme(void) {
  char v_buf[512];
  logging(-1, "\n");
  logging(-1, "Usage:\n");
  logging(-1, "em820w_tool [OPTION]\n");
  logging(-1, " -v     be verbose (logcat)\n");
  logging(-1, " -s     show current mode\n");
  logging(-1, " -d     disable (crash) modem (1003)\n");
  logging(-1, " -e     enable modem after crash (1404)\n");
  logging(-1, " -b     run in background and monitor modem\n");
  logging(-1, " -k     kill rild instead of using service \"ril-daemon\"\n");
  logging(-1, " -n svc use service svc instead of \"ril-daemon\"\n");
  logging(-1, "\n");
  logging(-1, "This tool fixes the 3g crashes on Lenovo K1 and Medion P9516\n");
  logging(-1, "This is done by disabling and reenabling the modem and the\n");
  logging(-1, "ril-daemon.\n");
  logging(-1, "When the modem is operating normally, you can see the\n");
  logging(-1, "USB-ID 12d1:1404 in lsusb.\n");
  logging(-1, "When the modem has crashed, the USB-ID is 12d1:1003.\n");
  logging(-1, "\n");
  sprintf(v_buf, "%s %s by Ameisentaetowierer\n", g_vers, g_date);
  logging(-1, v_buf);
  logging(-1, "\n");
  exit(1);
}

int main(int argc, char **argv) {
  int v_opt;
  int v_set1404 = 0;
  int v_set1003 = 0;
  int v_show = 0;
  extern int optind, optopt;
  extern char *optarg;

  while((v_opt = getopt(argc, argv, "edbvskn:")) != -1) { 
    switch(v_opt) {
      case 'e':
        v_set1404 = 1;
        break;
      case 'd':
        v_set1003 = 1;
        break;
      case 'b':
        g_daemon = 1;
        break;
      case 'v':
        g_verbose = 1;
        break;
      case 's':
        v_show = 1;
        break;
      case 'k':
        g_kill = 1;
        break;
      case 'n':
        g_svc = optarg;
        break;
      case ':':
      case '?':
        helpme();
    }
  }

  if((v_set1404 + v_set1003 + g_daemon + v_show > 1) ||
     (v_set1404 + v_set1003 + g_daemon + v_show == 0)) {
    helpme();
  }
  if(g_kill == 1 && g_svc != '\0') {
    helpme();
  }

  if(v_set1404 == 1) {
    set_1404();
    exit(0);
  }
  if(v_set1003 == 1) {
    set_1003();
    exit(0);
  }
  if(v_show == 1) {
    show_mode();
    exit(0);
  }
  if(g_daemon == 1) {
    daemonize();
  }
  helpme();
}
