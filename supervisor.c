#include <getopt.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <sys/stat.h>
#include <signal.h>
#include <sched.h>
#include <getopt.h>

#define MAX_BUF 255

#define PWM_PREFIX "/sys/devices/platform/soc/fe20c000.pwm/pwm/pwmchip0/"
#define READ_TEMP "/sys/class/thermal/thermal_zone0/temp"
#define GPIO_PREFIX "/sys/class/gpio/"

static char last_boot[MAX_BUF + 1] = "/lastboot";
static int min_boot_sec = 60*10;
static int real_reboot = 1;
static float min_temp = 35.0;
static float start_temp = 40.0;
static float delta_t = 25.0;
static int min_duty = 50;
static int debug = 0;
static int shutdown_pin = 454 + 5;
static int boot_pin = 454 + 12;

int read_int(char *fname)
{
  char buf[MAX_BUF + 1];
  int fd = open(fname, O_RDONLY);
  if (fd == -1) {
    fprintf(stderr, "Cannot open %s: %s(%d)", fname, strerror(errno), errno);
    exit(1);
  }
  int n = read(fd, buf, MAX_BUF);
  if (n == -1) {
    fprintf(stderr, "Cannot read %s: %s(%d)", fname, strerror(errno), errno);
    exit(1);
  }
  buf[n] = '\0';
  int ret = atoi(buf);
  close(fd);
  return ret;
}

void write_int(char *fname, int val)
{
  char buf[MAX_BUF + 1];
  int n = snprintf(buf, MAX_BUF, "%d", val);
  buf[n] = '\0';
  int fd = open(fname, O_WRONLY|O_CREAT);
  if (fd == -1) {
    fprintf(stderr, "Cannot open %s: %s(%d)\n", fname, strerror(errno), errno);
    exit(1);
  }
  int wr = write(fd, buf, strlen(buf));
  if (wr == -1) {
    fprintf(stderr, "Cannot write %s: %s(%d)\n", fname, strerror(errno), errno);
    exit(1);
  }
  close(fd);
}

void write_string(char *bname, int pin, char *buf)
{
  char fname[MAX_BUF + 1];
  int n = snprintf(fname, MAX_BUF, bname, pin);
  fname[n] = '\0';
  int fd = open(fname, O_WRONLY);
  if (fd == -1) {
    fprintf(stderr, "Cannot open %s: %s(%d)\n", fname, strerror(errno), errno);
    exit(1);
  }
  int wr = write(fd, buf, strlen(buf));
  if (wr == -1) {
    fprintf(stderr, "Cannot write %s: %s(%d)\n", fname, strerror(errno), errno);
    exit(1);
  }
  close(fd);
}

int running = 1;

void termination_handler(int signum)
{
  running = 0;
}

int main(int argc, char *argv[])
{
  static struct option long_options[] = {
        {"debug",     no_argument,       0,  'd' },
        {0,           0,                 0,  0   },
  };
  int long_index = 0;
  int opt = 0;
  while ((opt = getopt_long_only(argc, argv, "",
				 long_options, &long_index )) != -1) {
    switch (opt) {
    case 'd':
      debug = 1;
      real_reboot = 0;
      break;
    }
  }
  struct sigaction new_action;
  new_action.sa_handler = termination_handler;
  sigemptyset (&new_action.sa_mask);
  new_action.sa_flags = 0;
  sigaction(SIGINT, &new_action, NULL);
  sigaction(SIGTERM, &new_action, NULL);
  signal(SIGHUP, SIG_IGN);
  struct sched_param sp = {
    .sched_priority = sched_get_priority_max(SCHED_FIFO),
  };
  if (sched_setscheduler(0, SCHED_FIFO, &sp) == -1) {
    fprintf(stderr, "Cannot set RT priority: %s(%d)\n", strerror(errno), errno);
    return 1;
  }
  /* Cooling init */
  int cooling = 0;
  int period = 40000; /* in ns, so 25 kHz */
  if (access(PWM_PREFIX "pwm1/period", F_OK))
    write_int(PWM_PREFIX "export", 1);
  write_int(PWM_PREFIX "pwm1/period", period);
  write_int(PWM_PREFIX "pwm1/enable", 1);
  /* Reset button init */
  char shutdown[MAX_BUF + 1];
  int n = snprintf(shutdown, MAX_BUF, GPIO_PREFIX "gpio%d/value", shutdown_pin);
  shutdown[n] = '\0';
  if (access(shutdown, F_OK))
    write_int(GPIO_PREFIX "export", shutdown_pin);
  char boot[MAX_BUF + 1];
  n = snprintf(boot, MAX_BUF, GPIO_PREFIX "gpio%d/value", boot_pin);
  boot[n] = '\0';
  if (access(boot, F_OK))
    write_int(GPIO_PREFIX "export", boot_pin);
  write_string(GPIO_PREFIX "gpio%d/direction", shutdown_pin, "in");
  write_string(GPIO_PREFIX "gpio%d/direction", boot_pin, "out");
  write_int(boot, 1);
  /* Main loop */
  while (running) {
    /* Cooling Handling */
    float temp = read_int(READ_TEMP) / 1000.0;
    if (cooling) {
      if (temp < min_temp)
	cooling = 0;
    } else {
      if (temp > start_temp)
	cooling = 1;
    }
    if (cooling) {
      float duty = min_duty;
      float excess_temp = temp - min_temp;
      if (excess_temp > delta_t) {
        duty = 100;
      } else {
        duty = excess_temp / delta_t * 100.0;
	if (duty < min_duty)
	  duty = min_duty;
      }
      int dur = (int) (duty / 100.0 * period);
      if (debug)
        fprintf(stderr, "Temp: %f Duty: %f Duration: %d\n", temp, duty, dur);
      write_int(PWM_PREFIX "pwm1/duty_cycle", dur);
    } else {
      if (debug)
        fprintf(stderr, "Temp: %f\n", temp);
    }
    /* Reset button handling */
    if (read_int(shutdown)) {
      struct stat st;
      int do_reboot = 0;
      time_t now = time(NULL);
      if (stat(last_boot, &st) == -1) {
        do_reboot = 1;
      } else {
        if (now < st.st_mtim.tv_sec + min_boot_sec)
          fprintf(stderr, "Reboot requested, but too early since last one\n");
        else
          do_reboot = 1;
      }
      if (do_reboot) {
        write_int(last_boot, now);
        if (real_reboot) {
          if (execl("/usr/bin/reboot", "/usr/bin/reboot", NULL) == -1) {
            fprintf(stderr, "Reboot failed: %s(%d)\n", strerror(errno), errno);
          }
        } else {
          fprintf(stderr, "REBOOT!\n");
        }
      }
    }
    /* Reloop */
    sleep(1);
  }
  return 0;
}
