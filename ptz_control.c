// ptz_control.c
// Un seul binaire, deux modes :
//   ./ptz_control serve <ip> <port> <user> <pass>   -> démarre le service (login SDK, socket UNIX)
//   ./ptz_control <CMD> <START|STOP>               -> envoie la commande au service

#include "HCNetSDK.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/un.h>
#include <signal.h>

#define SOCK_PATH "/tmp/ptzd.socket"
#define BUFSIZE   64

static int getCmd(const char* s) {
  if (!strcmp(s,"LEFT"))     return PAN_LEFT;
  if (!strcmp(s,"RIGHT"))    return PAN_RIGHT;
  if (!strcmp(s,"UP"))       return TILT_UP;
  if (!strcmp(s,"DOWN"))     return TILT_DOWN;
  if (!strcmp(s,"ZOOM_IN"))  return ZOOM_IN;
  if (!strcmp(s,"ZOOM_OUT")) return ZOOM_OUT;
  return -1;
}

static void run_service(char* ip, int port, char* user, char* pass) {
  // 1) init + login
  NET_DVR_Init();
  NET_DVR_DEVICEINFO_V30 di;
  LONG uid = NET_DVR_Login_V30(ip,port,user,pass,&di);
  if (uid<0) {
    fprintf(stderr,"[ptz] Login failed %u\n",NET_DVR_GetLastError());
    exit(1);
  }
  DWORD chan = di.byStartChan;

  // 2) préparatifs socket UNIX
  unlink(SOCK_PATH);
  int ls = socket(AF_UNIX, SOCK_STREAM, 0);
  struct sockaddr_un sa = { .sun_family=AF_UNIX };
  strcpy(sa.sun_path, SOCK_PATH);
  bind(ls,(struct sockaddr*)&sa,sizeof(sa));
  listen(ls,5);
  chmod(SOCK_PATH,0666);
  fprintf(stderr,"[ptz] service up on %s\n",SOCK_PATH);

  // 3) boucle de service
  for (;;) {
    int cl = accept(ls, NULL, NULL);
    if (cl<0) continue;
    char buf[BUFSIZE];
    int n = read(cl,buf,BUFSIZE-1);
    if (n>0) {
      buf[n]=0;
      char cmdstr[16], actstr[16];
      if (sscanf(buf,"%15s %15s", cmdstr, actstr)==2) {
        int  cmd   = getCmd(cmdstr);
        DWORD stop = (strcmp(actstr,"STOP")==0);
        if (cmd>=0) {
          if (!NET_DVR_PTZControl_Other(uid, chan, cmd, stop))
            dprintf(cl,"ERR %u\n", NET_DVR_GetLastError());
          else
            dprintf(cl,"OK\n");
        }
        else
          dprintf(cl,"ERR_CMD\n");
      }
    }
    close(cl);
  }
}

static void usage(char* prog) {
  printf("Usage:\n"
         "  %s serve <ip> <port> <user> <pass>\n"
         "     -> démarre le service PTZ (login SDK, socket)\n"
         "  %s <CMD> <START|STOP>\n"
         "     -> envoie la commande au service\n"
         "CMD = LEFT | RIGHT | UP | DOWN | ZOOM_IN | ZOOM_OUT\n",
         prog, prog);
  exit(1);
}

int main(int argc, char** argv) {
  if (argc==6 && !strcmp(argv[1],"serve")) {
    // mode service
    signal(SIGPIPE, SIG_IGN);
    run_service(argv[2], atoi(argv[3]), argv[4], argv[5]);
    return 0;
  }
  if (argc==3) {
    // mode client
    char* cmd  = argv[1];
    char* act  = argv[2];
    struct sockaddr_un sa = { .sun_family=AF_UNIX };
    int s = socket(AF_UNIX, SOCK_STREAM, 0);
    strcpy(sa.sun_path, SOCK_PATH);
    if (connect(s,(struct sockaddr*)&sa,sizeof(sa))<0) {
      perror("connect");
      return 1;
    }
    char msg[BUFSIZE];
    snprintf(msg,BUFSIZE,"%s %s\n",cmd,act);
    write(s,msg,strlen(msg));
    char resp[16];
    int r = read(s,resp,sizeof(resp)-1);
    if (r>0) {
      resp[r]=0;
      printf("%s",resp);
    }
    close(s);
    return 0;
  }
  usage(argv[0]);
  return 0;
}
