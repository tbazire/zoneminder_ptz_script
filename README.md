# zoneminder_ptz_script
Programme utilisant le SDK Hikvision pour piloter les camera EZVIZ (tester sur EZVIZ C8C)

./ptz_control
Usage:
  ./ptz_control serve <ip> <port> <user> <pass>
     -> démarre le service PTZ (login SDK, socket)
  ./ptz_control <CMD> <START|STOP>
     -> envoie la commande au service
CMD = LEFT | RIGHT | UP | DOWN | ZOOM_IN | ZOOM_OUT
