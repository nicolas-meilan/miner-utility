/*
  CustomServer.h - Server manager.
*/
#include <Arduino.h>

#include "CustomServer.h"

const String CustomServer::DEFAULT_POST_ARG = "plain";
const String CustomServer::VOID_BODY = "NULL";

CustomServer::CustomServer(int port) : WebServer(port){};

String CustomServer::getBody()
{
  String body = this->arg(CustomServer::DEFAULT_POST_ARG);

  if (body == VOID_BODY)
    return "";

  return body;
}
