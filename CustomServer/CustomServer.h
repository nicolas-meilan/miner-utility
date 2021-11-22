/*
  CustomServer.h - Server manager.
*/
#ifndef CustomServer_h
#define CustomServer_h

#include <Arduino.h>
#include <WebServer.h>

class CustomServer : public WebServer
{
  public:
    CustomServer(int port);
    String getBody();
  
  private:
    static const String DEFAULT_POST_ARG;
    static const String VOID_BODY;
};

#endif
