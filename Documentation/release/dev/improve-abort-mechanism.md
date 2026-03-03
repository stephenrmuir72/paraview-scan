## Improve abort mechanism

 - ParaView _abort_ button is now only enabled whenever apply is pressed instead of whenever an progress
event is received, which could lead to instability.
 - ParaView _abort_ button now works in client/server and client/server/render mode

 Please note distributed server abort is currently non-fonctionnal.
